#include "EnergyLinkSubsystem.h"

#include "CLCDOBPFLib.h"

DEFINE_LOG_CATEGORY(ApEnergyLink);

// Sets default values
AEnergyLinkSubsystem::AEnergyLinkSubsystem() : Super()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEnergyLinkSubsystem::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(ApSubsystem, Display, TEXT("AEnergyLinkSubsystem:BeginPlay()"));

	/*FString changeBaseClassJson = TEXT(R"({
		"$schema": "https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_CDO.json",
		"Class" : "/Game/FactoryGame/Buildable/Factory/PowerStorage/Build_PowerStorageMk1.Build_PowerStorageMk1",
		"Edits" : [
			{
				"Property": "Parent",
				"Value" : "/Archipelago/Buildables/EnergyLinkBuildable.EnergyLinkBuildable"
			}
		]
	})");

	if (UCLCDOBPFLib::GenerateCLCDOFromString(changeBaseClassJson, true)) {
		UE_LOG(ApSubsystem, Display, TEXT("AEnergyLinkSubsystem:BeginPlay(), Patched BuildEnergyLink baseclass"));
	}
	else
	{
		UE_LOG(ApSubsystem, Error, TEXT("AEnergyLinkSubsystem:BeginPlay(), Failed to patch BuildEnergyLink baseclass"));
	}*/

	AFGBuildablePowerStorage* bpscdo = GetMutableDefault<AFGBuildablePowerStorage>();
	SUBSCRIBE_METHOD_VIRTUAL(AFGBuildablePowerStorage::BeginPlay, bpscdo, [this](auto& scope, AFGBuildablePowerStorage* self) {
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::BeginPlay()"));

		if (!PowerStorages.Contains(self)) {
			PowerStorages.Add(self);
		}
	});
	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::EndPlay, [this](auto& scope, AFGBuildablePowerStorage* self, const EEndPlayReason::Type EndPlayReason) {
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::EndPlay()"));

		if (PowerStorages.Contains(self)) {
			PowerStorages.Remove(self);
		}
	});*/

	//Called for UI, percentage from 0.0f to 100.0f of how full specific power storage is
	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetPowerStore, [](auto& scope, const AFGBuildablePowerStorage* self) {
		float f = scope(self);
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetPowerStore(): %f"), f);
	});*/
	
	SUBSCRIBE_METHOD(AFGBuildablePowerStorage::IndicatorLevelChanged, [](auto& scope, AFGBuildablePowerStorage* self, uint8 indicatorLevel) {
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::IndicatorLevelChanged(indicatorLevel: %d)"), indicatorLevel);
	});

	//Called when status of individual power storage changes, idle / charging / draining etc
	SUBSCRIBE_METHOD(AFGBuildablePowerStorage::StatusChanged, [](auto& scope, AFGBuildablePowerStorage* self, EBatteryStatus newStatus) {
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::StatusChanged(newStatus: %d)"), newStatus);
	});

	/*SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetPowerStoreCapacity, [](auto& scope, const AFGBuildablePowerStorage* self) {
		float f = scope(self);
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetPowerStoreCapacity(): %f"), f);
	});*/

	SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetPowerStorePercent, [](auto& scope, const AFGBuildablePowerStorage* self) {
		float f = scope(self);
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetPowerStorePercent(): %f"), f);
	});

	//Called for UI, current charge added or drained from the grid for specific power storage
	SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetNetPowerInput, [](auto& scope, const AFGBuildablePowerStorage* self) {
		float f = scope(self);
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetNetPowerInput(): %f"), f);
	});

	//Called repeatingly for indicator at the outside
	SUBSCRIBE_METHOD(AFGBuildablePowerStorage::CalculateIndicatorLevel, [](auto& scope, const AFGBuildablePowerStorage* self) {
		//float f = scope(self);
		//UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::CalculateIndicatorLevel(): %f"), f);
		scope.Override(5.0f);
	});

	GetWorldTimerManager().SetTimer(timerHandle, this, &AEnergyLinkSubsystem::SecondThick, 1.0f, true);
}

void AEnergyLinkSubsystem::SecondThick() {
	//UE_LOG(ApSubsystem, Display, TEXT("AEnergyLinkSubsystem::SecondThick()"));

	int numberOfDrainingStorages = 0;
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

		if (chargePerSecond < 0.0f)
			numberOfDrainingStorages++;

		totalChargePerSecond += chargePerSecond;

		powerStorage->mPowerStoreCapacity = (float)max_capacity;
		powerStorage->mPowerStore = (float)currentServerStorage / (float)PowerStorages.Num();

		UFGBatteryInfo* info = powerStorage->mBatteryInfo;
		TSubclassOf<class UFGPowerInfoComponent> powerInfo = powerStorage->mPowerInfoClass;
	}

	for (AFGBuildablePowerStorage* powerStorageToRemove : scheduledForRemoval) {
		if (powerStorageToRemove)
			PowerStorages.Remove(powerStorageToRemove);
	}

	UE_LOG(ApSubsystem, Display, TEXT("AEnergyLinkSubsystem::SecondThick() total power balance: %f, (max float %f)"), totalChargePerSecond, FLT_MAX);
}

// Called every frame
void AEnergyLinkSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

