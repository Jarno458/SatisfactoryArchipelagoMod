#include "EnergyLinkSubsystem.h"

DEFINE_LOG_CATEGORY(ApEnergyLink);

// Sets default values
AEnergyLinkSubsystem::AEnergyLinkSubsystem()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f;
}

// Called when the game starts or when spawned
void AEnergyLinkSubsystem::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(ApSubsystem, Display, TEXT("AEnergyLinkSubsystem:BeginPlay()"));

	AFGBuildablePowerStorage* bpscdo = GetMutableDefault<AFGBuildablePowerStorage>();
	SUBSCRIBE_METHOD_VIRTUAL(AFGBuildablePowerStorage::BeginPlay, bpscdo, [this](auto& scope, AFGBuildablePowerStorage* self) {
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::BeginPlay()"));

		if (!PowerStorages.Contains(self)) {
			PowerStorages.Add(self);
		}
	});

	//Called for UI, percentage from 0.0f to 100.0f of how full specific power storage is
	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetPowerStore, [](auto& scope, const AFGBuildablePowerStorage* self) {
		float f = scope(self);
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetPowerStore(): %f"), f);
	});*/
	
	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::IndicatorLevelChanged, [](auto& scope, AFGBuildablePowerStorage* self, uint8 indicatorLevel) {
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::IndicatorLevelChanged(indicatorLevel: %d)"), indicatorLevel);
	});*/

	//Called when status of individual power storage changes, idle / charging / draining etc
	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::StatusChanged, [](auto& scope, AFGBuildablePowerStorage* self, EBatteryStatus newStatus) {
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::StatusChanged(newStatus: %d)"), newStatus);
	});*/

	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetPowerStoreCapacity, [](auto& scope, const AFGBuildablePowerStorage* self) {
		float f = scope(self);
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetPowerStoreCapacity(): %f"), f);
	});*/

	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetPowerStorePercent, [](auto& scope, const AFGBuildablePowerStorage* self) {
		//float f = scope(self);
		//UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetPowerStorePercent(): %f"), f);
	});*/

	//Called for UI, current charge added or drained from the grid for specific power storage
	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetNetPowerInput, [](auto& scope, const AFGBuildablePowerStorage* self) {
		//float f = scope(self);
		//UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetNetPowerInput(): %f"), f);
	});*/

	//Called repeatingly for indicator at the outside
	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::CalculateIndicatorLevel, [](auto& scope, const AFGBuildablePowerStorage* self) {
		//float f = scope(self);
		//UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::CalculateIndicatorLevel(): %f"), f);
		scope.Override(5.0f);
	});*/
}

// Called every frame
void AEnergyLinkSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!apInitialized) {
		USubsystemActorManager* SubsystemActorManager = GetWorld()->GetSubsystem<USubsystemActorManager>();
		check(SubsystemActorManager);

		ap = SubsystemActorManager->GetSubsystemActor<AApSubsystem>();

		if (ap->isInitialized)
		{
			ap->MonitorDataStoreValue("EnergyLink", AP_DataType::Raw, [&](AP_SetReply setReply) {
				std::string valueStr = (*(std::string*)setReply.value).c_str();

				//TODO parse numeric value to update serverCharge
				if (valueStr.length() > 6)
					currentServerStorage = 999999;
				else
					currentServerStorage = stol(valueStr);
			});

			apInitialized = true;
		}
	}

	if (apInitialized)
		SecondThick();
}


void AEnergyLinkSubsystem::SecondThick() {
	//TODO move to power update tick

	float totalChargePerSecond = 0.0f;

	TArray<AFGBuildablePowerStorage*> scheduledForRemoval;

	for (AFGBuildablePowerStorage* powerStorage : PowerStorages) {
		if (!powerStorage || powerStorage->IsPendingKill())
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

	for (AFGBuildablePowerStorage* powerStorage : PowerStorages) {
		powerStorage->mPowerStoreCapacity = (float)currentServerStorage + localStorage;
		powerStorage->mBatteryInfo->mPowerStoreCapacity = (float)currentServerStorage + localStorage + 100; //make sure it keeps charging even if at max
		powerStorage->mPowerStore = (float)currentServerStorage + localStorage;
		powerStorage->mBatteryInfo->mPowerStore = (float)currentServerStorage + localStorage;
	}

	if (localStorage > 1 || localStorage < -1) {
		float chargeToSend;

		localStorage = modff(localStorage, &chargeToSend);

		SendEnergyToServer((long)chargeToSend);
	}

	UE_LOG(ApSubsystem, Display, TEXT("AEnergyLinkSubsystem::SecondThick() local storage: %f"), localStorage);
}


void AEnergyLinkSubsystem::SendEnergyToServer(long amount) {
	UE_LOG(ApSubsystem, Display, TEXT("AEnergyLinkSubsystem::SendEnergyToServer(%i)"), amount);

	if (!apInitialized)
		return;

	AP_SetServerDataRequest sendEnergyLinkUpdate;
	sendEnergyLinkUpdate.key = "EnergyLink";

	std::string valueToAdd = std::to_string(amount);
	std::string minimalValue = "0";

	AP_DataStorageOperation add;
	add.operation = "add";
	add.value = &valueToAdd;

	AP_DataStorageOperation lowerBoundry;
	add.operation = "max";
	add.value = &minimalValue;

	std::vector<AP_DataStorageOperation> operations;
	operations.push_back(add);
	operations.push_back(lowerBoundry);

	sendEnergyLinkUpdate.operations = operations;
	sendEnergyLinkUpdate.default_value = &minimalValue;
	sendEnergyLinkUpdate.type = AP_DataType::Raw;
	sendEnergyLinkUpdate.want_reply = true;

	ap->SetServerData(&sendEnergyLinkUpdate);
}

