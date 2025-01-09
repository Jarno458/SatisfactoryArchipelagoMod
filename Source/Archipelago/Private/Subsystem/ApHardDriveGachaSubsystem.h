#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Subsystem/ApSubsystem.h"
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

	static AApHardDriveGachaSubsystem* Get(class UWorld* world);

	const int bucketSize = 4;

	void Initialize(const TArray<TSubclassOf<UFGSchematic>>& apSchematics);

	UPROPERTY()
	UFGHardDriveSettings* HardDriveSettings;

private:
	bool isEnabled = false;
	bool hooksInitialized = false;

	TArray<TSubclassOf<class UFGSchematic>> apHardDriveSchematics;
	TArray<TSubclassOf<class UFGSchematic>> schematicsToOffer;

	AApSubsystem* ap;
	UModContentRegistry* contentRegistry;
	AFGSchematicManager* SManager;

	void GetValidSchematicRewardDrops(TCallScope<void(*)(const UFGHardDriveSettings*, class AFGSchematicManager*, TArray<TSubclassOf<class UFGSchematic>>&)>& Scope, const UFGHardDriveSettings* self, class AFGSchematicManager* schematicManager, TArray<TSubclassOf<class UFGSchematic>>& out_validSchematics);
	TArray<TSubclassOf<class UFGSchematic>> GetFinalSchematicRewards(TCallScope<TArray<TSubclassOf<class UFGSchematic>>(*)(const UFGHardDriveSettings*, const TArray<TSubclassOf<class UFGSchematic>>& allValidSchematicDrops)>& Scope, const UFGHardDriveSettings* self, const TArray<TSubclassOf<class UFGSchematic>>& allValidSchematicDrops);

	TSubclassOf<class UFGSchematic> GetRandomSchematic(int lowerBound, int upperBound);


	UFUNCTION() //required for event binding
	void OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic);
};

#pragma optimize("", on)
