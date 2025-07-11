#include "Subsystem/ApEnergyLinkSubsystem.h"
#include "ApUtils.h"
#include "Misc/Char.h"
#include "Logging/StructuredLog.h"
#include "EngineUtils.h"
#include "Misc/ScopeLock.h"

DEFINE_LOG_CATEGORY(LogApEnergyLink);

#define LOCTEXT_NAMESPACE "Archipelago"

AApEnergyLinkSubsystem::AApEnergyLinkSubsystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

AApEnergyLinkSubsystem* AApEnergyLinkSubsystem::Get(UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApEnergyLinkSubsystem>();
}

AApEnergyLinkSubsystem* AApEnergyLinkSubsystem::Get(class UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

void AApEnergyLinkSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AApEnergyLinkSubsystem, replicatedServerStorageJoules);
	DOREPLIFETIME(AApEnergyLinkSubsystem, replicatedServerStorageSuffix);
	DOREPLIFETIME(AApEnergyLinkSubsystem, replicatedGlobalChargeRateMegaWattHour);
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
		hookHandlerBatteryTick = SUBSCRIBE_METHOD_AFTER(UFGPowerCircuitGroup::TickPowerCircuitGroup, [this](UFGPowerCircuitGroup* self, float deltaTime) {
			TickPowerCircuits(self, deltaTime);
		});

		hooksInitialized = true;
	}
}

void AApEnergyLinkSubsystem::EndPlay(const EEndPlayReason::Type endPlayReason) {
	if (hookHandlerBatteryTick.IsValid())
		UNSUBSCRIBE_METHOD(UFGPowerCircuitGroup::TickPowerCircuitGroup, hookHandlerBatteryTick);
}

void AApEnergyLinkSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (!HasAuthority())
		return;

	if (!isInitialized && randomizerSubsystem->IsInitialized()) {
		if (!slotDataSubsystem->HasLoadedSlotData())
			return;

		isInitialized = true;

		energyLinkEnabled = slotDataSubsystem->EnergyLink;

		if (apConnectionInfo->GetConnectionState() == EApConnectionState::Connected && energyLinkEnabled) {
			ap->MonitorDataStoreValue(FString("EnergyLink") + UApUtils::FStr(apConnectionInfo->GetCurrentPlayerTeam()), AP_DataType::Raw, "0", [&](AP_SetReply setReply) {
				OnEnergyLinkValueChanged(setReply);
			});
		}
	}

	if (isInitialized && energyLinkEnabled)
		EnergyLinkTick(DeltaTime);
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
	int256 serverValueJoules = Int256FromDecimal(intergerValueString);

	//should never happen, but we cant trust other games
	if (serverValueJoules < 0)
		serverValueJoules = 0;
	
	int256 valueForClientJoules = serverValueJoules;
	int devisions = 0;
	while (valueForClientJoules > 10000 && devisions < 7) {
		valueForClientJoules /= 1000;
		devisions++;
	}
	if (devisions >= 7) {
		replicatedServerStorageJoules = 0;
		replicatedServerStorageSuffix = EApUnitSuffix::Overflow;
	}
	else {
		// since this is interger division, it wont show decimals ever
		replicatedServerStorageJoules = valueForClientJoules.ToInt();
		replicatedServerStorageSuffix = static_cast<EApUnitSuffix>(devisions);
	}

	int256 serverValueMegaWattHour = serverValueJoules / (ENERGYLINK_MULTIPLIER * 3600);

	// if there is enough energy to keep us going for an other minute then we dont need to know the exact value
	if (serverValueMegaWattHour > 99900)
		serverAvailableMegaWattHour = 99900;
	else
		serverAvailableMegaWattHour = serverValueMegaWattHour.ToInt();
}

void AApEnergyLinkSubsystem::TickPowerCircuits(UFGPowerCircuitGroup* instance, float deltaTime) {
	double netMegaJoules = 0.0f;

	if (!isInitialized
		|| !energyLinkEnabled
		|| !IsValid(instance)) //still, gets called for cirquits without batteries
		return;

	for (UFGPowerCircuit* cicuit : instance->mCircuits) {
		if (!IsValid(cicuit)
			|| cicuit->IsFuseTriggered()
			|| !cicuit->HasBatteries())
			continue;

		netMegaJoules += deltaTime * cicuit->GetmBatterySumPowerInput();
	}

	UE::TScopeLock<FCriticalSection> lock(localStorageLock);
	localAvailableMegaJoule += netMegaJoules;
}

void AApEnergyLinkSubsystem::EnergyLinkTick(float deltaTime) {
	float individualStorage = serverAvailableMegaWattHour;
	if (individualStorage > 1000)
		individualStorage = 1000.0f;

	float maxStorage = 10000.0f;
	if (serverAvailableMegaWattHour + 100.0f >  maxStorage)
		maxStorage = serverAvailableMegaWattHour + 100;

	for (TActorIterator<AFGBuildablePowerStorage> actorItterator(GetWorld()); actorItterator; ++actorItterator) {
		AFGBuildablePowerStorage* powerStorage = *actorItterator;
		if (!IsValid(powerStorage))
			continue;

		//current building stored power
		powerStorage->mPowerStore = serverAvailableMegaWattHour + localAvailableMegaJoule;
		//current building capacity
		powerStorage->mPowerStoreCapacity = 9999.0f;
		//gird capacity
		powerStorage->mBatteryInfo->mPowerStoreCapacity = 100000.0f;
		//grid stored power
		powerStorage->mBatteryInfo->mPowerStore = serverAvailableMegaWattHour + localAvailableMegaJoule;
	}
}

void AApEnergyLinkSubsystem::HandleExcessEnergy() {
	UE::TScopeLock<FCriticalSection> lock(localStorageLock);

	if (localAvailableMegaJoule <= 0 && serverAvailableMegaWattHour == 0) {
		localAvailableMegaJoule = 0.0f; //trip powah
	}
	else if (localAvailableMegaJoule > 4 || localAvailableMegaJoule < -4) {
		//apply 25% cut by deviding by 4 and only sending 3 times that
		float intergerPart;
		long remainder = modff(localAvailableMegaJoule / 4, &intergerPart);

		localAvailableMegaJoule = (remainder * 4);

		//we can only send full intergers
		SendEnergyToServer(intergerPart * 3);
	}
}

void AApEnergyLinkSubsystem::SendEnergyToServer(long amount) {
	if (!isInitialized)
		return;

	ap->ModdifyEnergyLink(amount * ENERGYLINK_MULTIPLIER);
}

TArray<FApGraphInfo> AApEnergyLinkSubsystem::GetEnergyLinkGraphs(UFGPowerCircuit* circuit) {
	const FLinearColor circuitChargeColor(0.2f, 0.5f, 0.1f);
	const FLinearColor circuitDrainColor(0.5f, 0.2f, 0.1f);

	TArray<FApGraphInfo> graphs;

	if (!isInitialized || !energyLinkEnabled)
		return graphs;
	
	FApGraphInfo cirquitEnergyLinkGraph;
	FString remaining;

	cirquitEnergyLinkGraph.Id = FString(TEXT("EL"));
	cirquitEnergyLinkGraph.DisplayName = FText::FromString(TEXT("EnergyLink "));
	cirquitEnergyLinkGraph.Suffix = FText::FromString(TEXT("MW"));
	cirquitEnergyLinkGraph.FullName = FText::FromString(TEXT("Enerylink Charge/Drain"));
	cirquitEnergyLinkGraph.Description = FText::FromString(TEXT("How much power is added or drained from the EnergyLink"));

	float charge = circuit->GetmBatterySumPowerInput();

	if (charge > 0)
		cirquitEnergyLinkGraph.Color = circuitChargeColor;
	else
		cirquitEnergyLinkGraph.Color = circuitDrainColor;

	cirquitEnergyLinkGraph.DataPoints.SetNum(10);
	for (int i = 0; i < cirquitEnergyLinkGraph.DataPoints.Num(); i++) {
		cirquitEnergyLinkGraph.DataPoints[i] = charge;
	}

	graphs.Add(cirquitEnergyLinkGraph);
	
	return graphs;
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

#undef LOCTEXT_NAMESPACE