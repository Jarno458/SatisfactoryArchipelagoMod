#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Subsystem/ApSubsystem.h"
#include "Data/ApSlotData.h"
#include "FGHardDriveSettings.h"
#include "Patching/NativeHookManager.h"
#include "FGGlobalSettings.h"

#include "ApHardDriveGachaSubsystem.generated.h"

//TODO REMOVE
#pragma optimize("", off)

DECLARE_LOG_CATEGORY_EXTERN(LogApHardDriveGachaSubsystem, Log, All);

UCLASS()
class ARCHIPELAGO_API AApHardDriveGachaSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApHardDriveGachaSubsystem();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	static AApHardDriveGachaSubsystem* Get(class UWorld* world);

	void DispatchLifecycleEvent(ELifecyclePhase phase, TArray<TSubclassOf<UFGSchematic>> apHardcodedSchematics);

	UPROPERTY()
	UFGHardDriveSettings* HardDriveSettings;

private:
	bool hooksInitialized = false;

	TArray<TSubclassOf<class UFGSchematic>> apUnlocks;
	TArray<TSubclassOf<class UFGSchematic>> schematicsToOffer;

	AApSubsystem* ap;
	UModContentRegistry* contentRegistry;

	void GetValidSchematicRewardDrops(TCallScope<void(*)(const UFGHardDriveSettings*, class AFGSchematicManager*, TArray<TSubclassOf<class UFGSchematic>>&)>& Scope, const UFGHardDriveSettings* self, class AFGSchematicManager* schematicManager, TArray<TSubclassOf<class UFGSchematic>>& out_validSchematics);
	TArray<TSubclassOf<class UFGSchematic>> GetFinalSchematicRewards(TCallScope<TArray<TSubclassOf<class UFGSchematic>>(*)(const UFGHardDriveSettings*, const TArray<TSubclassOf<class UFGSchematic>>& allValidSchematicDrops)>& Scope, const UFGHardDriveSettings* self, const TArray<TSubclassOf<class UFGSchematic>>& allValidSchematicDrops);

};

#pragma optimize("", on)
