#include "Subsystem/ApEnergyLinkSubsystem.h"

DEFINE_LOG_CATEGORY(LogApEnergyLink);

AApEnergyLinkSubsystem::AApEnergyLinkSubsystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f;
}

void AApEnergyLinkSubsystem::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogApSubsystem, Display, TEXT("AEnergyLinkSubsystem:BeginPlay()"));

	if (!hooksInitialized) {
		AFGBuildablePowerStorage* bpscdo = GetMutableDefault<AFGBuildablePowerStorage>();
		SUBSCRIBE_METHOD_VIRTUAL(AFGBuildablePowerStorage::BeginPlay, bpscdo, [this](auto& scope, AFGBuildablePowerStorage* self) {
			UE_LOG(LogApSubsystem, Display, TEXT("AFGBuildablePowerStorage::BeginPlay()"));

			if (!PowerStorages.Contains(self)) {
				PowerStorages.Add(self);
			}
		});

		hooksInitialized = true;
	}
}

void AApEnergyLinkSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!apInitialized) {
		ap = AApSubsystem::Get(GetWorld());

		if (ap->ConnectionState == EApConnectionState::Connected)
		{
			ap->MonitorDataStoreValue("EnergyLink", AP_DataType::Raw, energyLinkDefault, [&](AP_SetReply setReply) {
				OnEnergyLinkValueChanged(setReply);
			});

			apInitialized = true;
		}
	}

	if (apInitialized)
		SecondThick();
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

void AApEnergyLinkSubsystem::SecondThick() {
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
	if (!apInitialized)
		return;

	AP_SetServerDataRequest sendEnergyLinkUpdate;
	sendEnergyLinkUpdate.key = "EnergyLink";

	std::string valueToAdd = std::to_string(amount);

	AP_DataStorageOperation add;
	add.operation = "add";
	add.value = &valueToAdd;

	AP_DataStorageOperation lowerBoundry;
	lowerBoundry.operation = "max";
	lowerBoundry.value = &energyLinkDefault;

	std::vector<AP_DataStorageOperation> operations;
	operations.push_back(add);
	operations.push_back(lowerBoundry);

	sendEnergyLinkUpdate.operations = operations;
	sendEnergyLinkUpdate.default_value = &energyLinkDefault;
	sendEnergyLinkUpdate.type = AP_DataType::Raw;
	sendEnergyLinkUpdate.want_reply = true;

	ap->SetServerData(&sendEnergyLinkUpdate);
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
