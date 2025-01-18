#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Subsystem/ApSubsystem.h"
#include "Patching/NativeHookManager.h"
#include "FGSchematicManager.h"
#include "FGResearchManager.h"

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
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	static AApHardDriveGachaSubsystem* Get(class UWorld* world);

	const int bucketSize = 4;

	void Initialize(const TArray<TSubclassOf<UFGSchematic>>& apHardDriveSchematics);

private:
	bool hooksInitialized = false;

	FDelegateHandle hookHandler;

	TArray<TSubclassOf<class UFGSchematic>> apHardDriveSchematics;

	UModContentRegistry* contentRegistry;
	AFGResearchManager* RManager;
	AApSchematicPatcherSubsystem* schematicPatcherSubsystem;

	bool GetAvailableAlternateSchematics(TCallScope<bool(*)(const AFGResearchManager* self, TArray<TSubclassOf<UFGSchematic>>, int32, TArray<TSubclassOf<UFGSchematic>>&)>& Scope, const AFGResearchManager* self, TArray<TSubclassOf<UFGSchematic>> excludedSchematics, int32 numSchematics, TArray<TSubclassOf<UFGSchematic>>& out_schematics);

	bool IsExternalModSchematic(TSubclassOf<class UFGSchematic> schematic);

	TSubclassOf<class UFGSchematic> GetRandomSchematic(TSet<TSubclassOf<UFGSchematic>> excludedSchematics);

	UFUNCTION() //required for event binding
	void OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic);
};

#pragma optimize("", on)
