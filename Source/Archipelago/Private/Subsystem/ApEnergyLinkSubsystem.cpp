#include "Subsystem/ApEnergyLinkSubsystem.h"
#include "ApUtils.h"

DEFINE_LOG_CATEGORY(LogApEnergyLink);

#define LOCTEXT_NAMESPACE "Archipelago"

AApEnergyLinkSubsystem::AApEnergyLinkSubsystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

void AApEnergyLinkSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AApEnergyLinkSubsystem, currentServerStorage);
}

void AApEnergyLinkSubsystem::BeginPlay() {
	Super::BeginPlay();

	if (!HasAuthority())
		return;
		
	UE_LOG(LogApEnergyLink, Display, TEXT("AEnergyLinkSubsystem:BeginPlay()"));

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
		UE_LOG(LogApEnergyLink, Display, TEXT("Initializing hooks"));
		AFGBuildablePowerStorage* bpscdo = GetMutableDefault<AFGBuildablePowerStorage>();
		hookHandler = SUBSCRIBE_METHOD_VIRTUAL(AFGBuildablePowerStorage::BeginPlay, bpscdo, [this](auto& scope, AFGBuildablePowerStorage* self) {
			UE_LOG(LogApEnergyLink, Display, TEXT("AFGBuildablePowerStorage::BeginPlay()"));

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
			UE_LOG(LogApEnergyLink, Display, TEXT("Setting MonitorDataStoreValue OnEnergyLinkValueChanged"));
			ap->MonitorDataStoreValue(FString("EnergyLink") + UApUtils::FStr(apConnectionInfo->GetCurrentPlayerTeam()), AP_DataType::Raw, energyLinkDefault, [&](AP_SetReply setReply) {
				OnEnergyLinkValueChanged(setReply);
			});
		}
	}

	if (isInitialized && energyLinkEnabled)
		EnergyLinkTick();
}

void AApEnergyLinkSubsystem::OnEnergyLinkValueChanged(AP_SetReply setReply) {
	std::string valueStr = (*(std::string*)setReply.value).c_str();

	if (valueStr.length() > 6 || valueStr.length() == 0)
		currentServerStorage = maxPowerStorage;
	else
	{
		long l = stol(valueStr);
		currentServerStorage = l;
	}
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
			chargePerSecond /= 60 * 60;

		totalChargePerSecond += chargePerSecond;
	}

	localStorage += totalChargePerSecond;

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

	for (AFGBuildablePowerStorage* powerStorage : PowerStorages) {
		powerStorage->mPowerStoreCapacity = maxPowerStorage;
		powerStorage->mBatteryInfo->mPowerStoreCapacity = maxPowerStorage * 2; //make sure it keeps charging even if at max
		powerStorage->mPowerStore = (float)currentServerStorage + localStorage;
		powerStorage->mBatteryInfo->mPowerStore = (float)currentServerStorage + localStorage;
	}
}

void AApEnergyLinkSubsystem::SendEnergyToServer(long amount) {
	if (!isInitialized)
		return;

	ap->ModdifyEnergyLink(amount, energyLinkDefault);
}

	//Called for UI, percentage from 0.0f to 100.0f of how full specific power storage is
	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetPowerStore, [](auto& scope, const AFGBuildablePowerStorage* self) {
		float f = scope(self);
		UE_LOG(LogApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetPowerStore(): %f"), f);
	});*/

	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::IndicatorLevelChanged, [](auto& scope, AFGBuildablePowerStorage* self, uint8 indicatorLevel) {
		UE_LOG(LogApSubsystem, Display, TEXT("AFGBuildablePowerStorage::IndicatorLevelChanged(indicatorLevel: %d)"), indicatorLevel);
	});*/

	//Called when status of individual power storage changes, idle / charging / draining etc
	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::StatusChanged, [](auto& scope, AFGBuildablePowerStorage* self, EBatteryStatus newStatus) {
		UE_LOG(LogApSubsystem, Display, TEXT("AFGBuildablePowerStorage::StatusChanged(newStatus: %d)"), newStatus);
	});*/

	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetPowerStoreCapacity, [](auto& scope, const AFGBuildablePowerStorage* self) {
		float f = scope(self);
		UE_LOG(LogApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetPowerStoreCapacity(): %f"), f);
	});*/

	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetPowerStorePercent, [](auto& scope, const AFGBuildablePowerStorage* self) {
		//float f = scope(self);
		//UE_LOG(LogApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetPowerStorePercent(): %f"), f);
	});*/

	//Called for UI, current charge added or drained from the grid for specific power storage
	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetNetPowerInput, [](auto& scope, const AFGBuildablePowerStorage* self) {
		//float f = scope(self);
		//UE_LOG(LogApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetNetPowerInput(): %f"), f);
	});*/

	//Called repeatingly for indicator at the outside
	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::CalculateIndicatorLevel, [](auto& scope, const AFGBuildablePowerStorage* self) {
		//float f = scope(self);
		//UE_LOG(LogApSubsystem, Display, TEXT("AFGBuildablePowerStorage::CalculateIndicatorLevel(): %f"), f);
		scope.Override(5.0f);
	});*/

#undef LOCTEXT_NAMESPACE