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

	SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetPowerStore, [](auto& scope, const AFGBuildablePowerStorage* self) {
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetPowerStore"));
	});

	SUBSCRIBE_METHOD(AFGBuildablePowerStorage::IndicatorLevelChanged, [](auto& scope, AFGBuildablePowerStorage* self, uint8 indicatorLevel) {
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::IndicatorLevelChanged"));
	});

	SUBSCRIBE_METHOD(AFGBuildablePowerStorage::StatusChanged, [](auto& scope, AFGBuildablePowerStorage* self, EBatteryStatus newStatus) {
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::StatusChanged"));
	});

	SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetPowerStoreCapacity, [](auto& scope, const AFGBuildablePowerStorage* self) {
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetPowerStoreCapacity"));
	});

	SUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetPowerStorePercent, [](auto& scope, const AFGBuildablePowerStorage* self) {
		UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetPowerStorePercent"));
	})

	GetWorldTimerManager().SetTimer(timerHandle, this, &AEnergyLinkSubsystem::SecondThick, 1.0f, true, 2.0f);
}

void AEnergyLinkSubsystem::SecondThick() {
	//UE_LOG(ApSubsystem, Display, TEXT("AEnergyLinkSubsystem::SecondThick()"));


}

/*float AEnergyLinkSubsystem::GetPowerStore(auto& scope, AFGBuildablePowerStorage* self) {
	UE_LOG(ApSubsystem, Display, TEXT("AFGBuildablePowerStorage::GetPowerStore"));
	return 1.1f;
}*/


// Called every frame
void AEnergyLinkSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

