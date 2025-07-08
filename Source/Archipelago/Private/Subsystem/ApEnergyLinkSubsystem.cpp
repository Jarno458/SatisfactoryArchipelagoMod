#include "Subsystem/ApEnergyLinkSubsystem.h"
#include "ApUtils.h"
#include "Misc/Char.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(LogApEnergyLink);

#define LOCTEXT_NAMESPACE "Archipelago"

//TODO REMOVE
#pragma optimize("", off)

AApEnergyLinkSubsystem::AApEnergyLinkSubsystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

void AApEnergyLinkSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AApEnergyLinkSubsystem, replicatedServerStorage);
	DOREPLIFETIME(AApEnergyLinkSubsystem, replicatedServerStorageSuffix);
	DOREPLIFETIME(AApEnergyLinkSubsystem, replicatedGlobalChargeRatePerSecond);
}

void AApEnergyLinkSubsystem::BeginPlay() {
	Super::BeginPlay();

	if (!HasAuthority())
		return;
		
	UE_LOGFMT(LogApEnergyLink, Display, "AEnergyLinkSubsystem:BeginPlay()");

	UWorld* world = GetWorld();
	ap = AApSubsystem::Get(world);
	fgcheck(ap);
	slotDataSubsystem = AApSlotDataSubsystem::Get(world);
	fgcheck(slotDataSubsystem);
	randomizerSubsystem = AApServerRandomizerSubsystem::Get(world);
	fgcheck(randomizerSubsystem);
	apConnectionInfo = AApConnectionInfoSubsystem::Get(world);
	fgcheck(apConnectionInfo);

	if (!hooksInitialized && !WITH_EDITOR) {
		AFGBuildablePowerStorage* bpscdo = GetMutableDefault<AFGBuildablePowerStorage>();
		hookHandler = SUBSCRIBE_METHOD_VIRTUAL(AFGBuildablePowerStorage::BeginPlay, bpscdo, [this](auto& scope, AFGBuildablePowerStorage* self) {
			UE_LOGFMT(LogApEnergyLink, Display, "AFGBuildablePowerStorage::BeginPlay()");

			if (!PowerStorages.Contains(self)) {
				PowerStorages.Add(self);
			}
		});

		hooksInitialized = true;
	}
}

void AApEnergyLinkSubsystem::EndPlay(const EEndPlayReason::Type endPlayReason) {
	if (hookHandler.IsValid())
		UNSUBSCRIBE_METHOD(AFGBuildablePowerStorage::BeginPlay, hookHandler);
}

void AApEnergyLinkSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (!HasAuthority())
		return;

	if (!isInitialized && randomizerSubsystem->IsInitialized()) {
		isInitialized = true;

		if (!slotDataSubsystem->HasLoadedSlotData())
			return;

		energyLinkEnabled = slotDataSubsystem->EnergyLink;

		if (apConnectionInfo->GetConnectionState() == EApConnectionState::Connected && energyLinkEnabled) {
			ap->MonitorDataStoreValue(FString("EnergyLink") + UApUtils::FStr(apConnectionInfo->GetCurrentPlayerTeam()), AP_DataType::Raw, "0", [&](AP_SetReply setReply) {
				OnEnergyLinkValueChanged(setReply);
			});
		}
	}

	if (isInitialized && energyLinkEnabled)
		EnergyLinkTick();
}

void AApEnergyLinkSubsystem::OnEnergyLinkValueChanged(AP_SetReply setReply) {
	//example 54737402054455566
	std::string valueCstr = (*(std::string*)setReply.value);
	FString rawValue = UApUtils::FStr(valueCstr).TrimStartAndEnd();

	FString intergerValueString;
	if (rawValue.IsEmpty()) {
		intergerValueString = "0";
	}
	else {
		//cut off decimals, should not happen, but we cant trust other games

		if (rawValue.Contains(".")) {
			UE_LOGFMT(LogApEnergyLink, Error, "AApEnergyLinkSubsystem::OnEnergyLinkValueChanged() received non numeric input {0}", rawValue);
			intergerValueString = rawValue.Left(rawValue.Find(".", ESearchCase::IgnoreCase, ESearchDir::FromEnd));
		}
		else {
			intergerValueString = rawValue;
		}

		if (!intergerValueString.IsNumeric()) {
			UE_LOGFMT(LogApEnergyLink, Error, "AApEnergyLinkSubsystem::OnEnergyLinkValueChanged() received non numeric input {0}", intergerValueString);
			return;
		}
	}

	//int256 is max python can store
	int256 serverValue = Int256FromDecimal(intergerValueString);

	//should never happen, but we cant trust other games
	if (serverValue < 0)
		serverValue = 0;
	
	int256 remainder;
	serverValue.DivideWithRemainder(ENERGYLINK_MULTIPLIER, remainder);

	int256 valueForClient = serverValue;
	int devisions = 0;
	while (valueForClient > 10000) {
		valueForClient /= 1000;
		devisions++;
	}
	if (devisions >= 7) {
		replicatedServerStorage = 0;
		replicatedServerStorageSuffix = EApEnergyLinkSuffix::Overflow;
	}
	else {
		replicatedServerStorage = valueForClient.ToInt() + ((float)remainder.ToInt() / ENERGYLINK_MULTIPLIER);
		replicatedServerStorageSuffix = static_cast<EApEnergyLinkSuffix>(devisions);
	}

	// if there is enough energy to keep us going for an other minute then we dont need to know the exact value
	if (serverValue > 99900)
		currentServerStorage = 99900.0f;
}

void AApEnergyLinkSubsystem::EnergyLinkTick() {
	//TODO move to power update tick

	float totalChargePerSecond = 0.0f;

	TArray<AFGBuildablePowerStorage*> scheduledForRemoval;

	for (AFGBuildablePowerStorage* powerStorage : PowerStorages) {
		if (!IsValid(powerStorage))
		{
			scheduledForRemoval.Add(powerStorage);
			continue;
		}

		float chargePerSecond = powerStorage->GetNetPowerInput();
		if (chargePerSecond != 0.0f)
			chargePerSecond /= 60;

		totalChargePerSecond += chargePerSecond;
	}

	localStorage += totalChargePerSecond;
	replicatedGlobalChargeRatePerSecond = totalChargePerSecond * 60;

	for (AFGBuildablePowerStorage* powerStorageToRemove : scheduledForRemoval) {
		if (powerStorageToRemove)
			PowerStorages.Remove(powerStorageToRemove);
	}

	if (localStorage < 0 && currentServerStorage == 0)
		localStorage = 0.0f; //trip powah
	else if (localStorage > 1 || localStorage < -1) {
		float chargeToSend;

		localStorage = modff(localStorage, &chargeToSend);

		SendEnergyToServer((long)chargeToSend);
	}

	float individualStorage = currentServerStorage;
	if (individualStorage > 1000)
		individualStorage = 1000.0f;

	float maxStorage = 10000.0f;
	if (currentServerStorage + 100.0f >  maxStorage)
		maxStorage = currentServerStorage + 100;

	for (AFGBuildablePowerStorage* powerStorage : PowerStorages) {
		//current building stored power
		powerStorage->mPowerStore = currentServerStorage + localStorage;
		//current building capacity
		powerStorage->mPowerStoreCapacity = 9999.0f;
		//gird capacity
		powerStorage->mBatteryInfo->mPowerStoreCapacity = 100000.0f;
		//grid stored power
		powerStorage->mBatteryInfo->mPowerStore = currentServerStorage + localStorage;
	}
}

void AApEnergyLinkSubsystem::SendEnergyToServer(long amount) {
	if (!isInitialized)
		return;

	ap->ModdifyEnergyLink(amount * ENERGYLINK_MULTIPLIER);
}

int256 AApEnergyLinkSubsystem::Int256FromDecimal(FString decimal) {
	bool negative = false;
	if (decimal.StartsWith(TEXT("-"))) {
		negative = true;
		decimal = decimal.RightChop(1);
	}

	FString reverse = decimal.Reverse();

	int256 total = 0;
	int multiplier = 0;

	for (TCHAR c : reverse) {
		if (!FChar::IsDigit(c)) {
			UE_LOGFMT(LogApEnergyLink, Error, "AApEnergyLinkSubsystem::Int256FromDecimal() received non numeric input {0} in string {1}", c, decimal);
			total = 0;
		}

		int256 multiplierValue = int256::One;
		for (int i = 0; i < multiplier; i++) {
			multiplierValue *= 10;
		}
				
		total += (multiplierValue * FChar::ConvertCharDigitToInt(c));
		multiplier++;
	}

	if (negative)
		total *= -1;

	UE_LOGFMT(LogApEnergyLink, VeryVerbose, "AApEnergyLinkSubsystem::Int256FromDecimal() validation, input: {0} > hex: {1} > validation: {2}", decimal, total.ToString(), FParse::HexNumber(*total.ToString()));

	return total;
}

#pragma optimize("", on)

#undef LOCTEXT_NAMESPACE