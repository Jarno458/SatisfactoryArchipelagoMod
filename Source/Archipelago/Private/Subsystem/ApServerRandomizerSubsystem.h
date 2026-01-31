#pragma once

#include "CoreMinimal.h"

#include "FGSchematicManager.h"
#include "FGResearchManager.h"
#include "FGGamePhaseManager.h"
#include "FGUnlockSubsystem.h"

#include "Subsystem/ModSubsystem.h"
#include "Templates/SubclassOf.h"

#include "ApTrapSubsystem.h"
#include "ApVaultSubsystem.h"
#include "Data/ApTypes.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "Subsystem/ApSchematicPatcherSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Subsystem/ApSlotDataSubsystem.h"
#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApHardDriveGachaSubsystem.h"
#include "Subsystem/ApMamTreeSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApServerRandomizerSubsystem, Log, All);

#include "ApServerRandomizerSubsystem.generated.h"

UCLASS()
class ARCHIPELAGO_API AApServerRandomizerSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AApServerRandomizerSubsystem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	// Begin IFGSaveInterface
	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void GatherDependencies_Implementation(TArray<UObject*>& out_dependentObjects) override {};
	virtual bool NeedTransform_Implementation() override { return false; };
	virtual bool ShouldSave_Implementation() const override { return true; };
	// End IFSaveInterface

public:
	static AApServerRandomizerSubsystem* Get(UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Ap Server Side Randomizer Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApServerRandomizerSubsystem* Get(UObject* worldContext);

	void DispatchLifecycleEvent(ELifecyclePhase phase, const TArray<TSubclassOf<UFGSchematic>>& schematics);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool IsInitialized() const { return areRecipiesAndSchematicsInitialized; };

	void ResetDuplicationCounter();
	void ResetCurrentItemCounter();

private:
	UPROPERTY(SaveGame)
	int lastProcessedItemIndex = 0;

	UPROPERTY(SaveGame)
	TArray<FApNetworkItem> scoutedLocations;

	int currentItemIndex = 0;
	int lastGamePhase = -1;

	AFGSchematicManager* SManager;
	AFGResearchManager* RManager;
	AFGGamePhaseManager* phaseManager;
	AFGUnlockSubsystem* unlockSubsystem;
	
	UModContentRegistry* contentRegistry;

	AApSubsystem* ap;
	AApConnectionInfoSubsystem* connectionInfo;
	AApSlotDataSubsystem* slotData;
	AApSchematicPatcherSubsystem* schematicPatcher;
	AApPortalSubsystem* portalSubsystem;
	AApVaultSubsystem* vaultSubsystem;
	AApMappingsSubsystem* mappingSubsystem;
	AApTrapSubsystem* trapSubsystem;
	AApHardDriveGachaSubsystem* hardDriveGachaSubsystem;
	AApMamTreeSubsystem* mamTreeSubsystem;

	TQueue<TTuple<int64, bool>> ReceivedItems;
	TQueue<int64> CheckedLocations;

	TArray<TSubclassOf<UFGSchematic>> schematics;

	TSet<int64> hintedLocations;

	TMap<TSubclassOf<UFGSchematic>, TArray<FApNetworkItem>> locationsPerMilestone;
	TMap<TSubclassOf<UFGSchematic>, FApNetworkItem> locationPerMamNode;
	TMap<TSubclassOf<UFGSchematic>, FApNetworkItem> locationPerHardDrive;
	TMap<TSubclassOf<UFGSchematic>, FApNetworkItem> locationPerShopNode;
	TMap<int64, TSubclassOf<UFGSchematic>> ItemSchematics;

	std::atomic_bool areRecipiesAndSchematicsInitialized;
	std::atomic_bool hasLoadedRoomInfo;

	bool harddriveRerollHookInitialized = false;
	FDelegateHandle harddriveRerollHookHandler;

	void ScoutArchipelagoItems();
	void ParseScoutedItemsAndCreateRecipiesAndSchematics();
	bool InitializeTick();
	void FinalizeInitialization();

	bool UpdateFreeSamplesConfiguration() const;

	void ReceiveItem(int64 itemId, bool isFromServer);
	void ProcessReceivedItems();
	void AwardItem(int64 itemId, bool isFromServer);

	AFGCharacterPlayer* GetLocalPlayer();

	void CollectLocation(int64 itemId);
	void HandleCheckedLocations();

	std::atomic_bool instagib;
	std::atomic_bool awaitingHealty;

	UFUNCTION() //required for event binding
	void OnMamResearchCompleted(TSubclassOf<UFGSchematic> schematic);
	UFUNCTION() //required for event binding
	void OnMamResearchTreeUnlocked(TSubclassOf<UFGResearchTree> researchTree);
	UFUNCTION() //required for event binding
	void OnSchematicCompleted(TSubclassOf<UFGSchematic> schematic);
	UFUNCTION() //required for event binding
	void OnUnclaimedHardDrivesUpdated();

	void OnAvaiableSchematicsChanged();
};

