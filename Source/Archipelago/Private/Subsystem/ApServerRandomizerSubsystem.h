#pragma once

#include "CoreMinimal.h"

#include "FGSchematicManager.h"
#include "FGResearchManager.h"
#include "FGGamePhaseManager.h"
#include "FGPlayerController.h"
#include "FGCharacterPlayer.h"
#include "FGUnlockSubsystem.h"

//#include "GenericPlatform/GenericPlatformProcess.h"
//#include "Registry/ModContentRegistry.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
//#include "Subsystem/ApTrapSubsystem.h"
#include "Configuration/ModConfiguration.h"
#include "Configuration/ConfigProperty.h"
#include "Configuration/Properties/ConfigPropertyInteger.h"
#include "Configuration/Properties/ConfigPropertyBool.h"
#include "Configuration/ConfigManager.h"
//#include "Engine/Engine.h"
#include "Configuration/Properties/ConfigPropertySection.h"
#include "Templates/SubclassOf.h"
#include "FGChatManager.h"
//#include "Module/ModModule.h"
//#include "Reflection/ClassGenerator.h"
//#include "Buildables/FGBuildable.h"
//#include "Buildables/FGBuildableAutomatedWorkBench.h"
#include "Unlocks/FGUnlockInfoOnly.h"

#include "ApConfigurationStruct.h"
#include "Data/ApSlotData.h"
#include "Data/ApTypes.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "Subsystem/ApSchematicPatcherSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Subsystem/ApSlotDataSubsystem.h"
#include "Subsystem/ApSubsystem.h"

#include "ApUtils.h"

//#include "CLSchematicBPFLib.h"
//#include "BPFContentLib.h"
#include "Configuration/FreeSamplesConfigurationStruct.h"

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
	static AApServerRandomizerSubsystem* Get(class UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Ap Server Side Randomizer Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApServerRandomizerSubsystem* Get(UObject* worldContext);

	void DispatchLifecycleEvent(ELifecyclePhase phase, TArray<TSubclassOf<UFGSchematic>> apHardcodedSchematics);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool IsInitialized() const { return areRecipiesAndSchematicsInitialized; };

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

	//UContentLibSubsystem* contentLibSubsystem;
	UModContentRegistry* contentRegistry;

	AApSubsystem* ap;
	AApConnectionInfoSubsystem* connectionInfo;
	AApSlotDataSubsystem* slotDataSubsystem;
	AApSchematicPatcherSubsystem* schematicPatcher;
	AApPortalSubsystem* portalSubsystem;
	AApMappingsSubsystem* mappingSubsystem;
	AApTrapSubsystem* trapSubsystem;

	TQueue<TTuple<int64, bool>> ReceivedItems;
	TQueue<int64> CheckedLocations;

	TArray<TSubclassOf<UFGSchematic>> hardcodedSchematics;

	TMap<TSubclassOf<class UFGSchematic>, TArray<FApNetworkItem>> locationsPerMilestone;
	TMap<TSubclassOf<class UFGSchematic>, FApNetworkItem> locationPerMamNode;
	TMap<TSubclassOf<class UFGSchematic>, FApNetworkItem> locationPerShopNode;
	TMap<int64, TSubclassOf<class UFGSchematic>> ItemSchematics;

	//TArray<TSubclassOf<class UFGSchematic>> inventorySlotRecipes;

	//std::atomic_bool hasScoutedLocations;
	//std::atomic_bool areScoutedLocationsReadyToParse;
	std::atomic_bool areRecipiesAndSchematicsInitialized;
	std::atomic_bool hasLoadedRoomInfo;

	void ScoutArchipelagoItems();
	void ParseScoutedItemsAndCreateRecipiesAndSchematics();
	//void LoadRoomInfo();
	bool InitializeTick();

	//UConfigPropertySection* GetConfigurationRootSection(FConfigId configId);

	bool UpdateFreeSamplesConfiguration();
	void SetMamEnhancerConfigurationHooks();
	UFUNCTION() //required for event binding
	void LockMamEnhancerSpoilerConfiguration();

	void ReceiveItem(int64 itemId, bool isFromServer);
	void ProcessReceivedItems();
	void AwardItem(int64 itemId, bool isFromServer);

	AFGCharacterPlayer* GetLocalPlayer();

	void CollectLocation(int64 itemId);
	void HandleCheckedLocations();
	//bool IsCollected(UFGUnlock* unlock);
	//void Collect(UFGUnlock* unlock, FApNetworkItem& networkItem);

	//void HandleDeathLink();
	//void HandleInstagib(AFGCharacterPlayer* player);

	//void HandleAPMessages();

	// TODO do we want to keep this around or call AApMessagingSubsystem::DisplayMessage directly?
	//void SendChatMessage(const FString& Message, const FLinearColor& Color);

	UFUNCTION() //required for event binding
	void OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic);
	UFUNCTION() //required for event binding
	void OnMamResearchTreeUnlocked(TSubclassOf<class UFGResearchTree> researchTree);
	UFUNCTION() //required for event binding
	void OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic);

	void OnAvaiableSchematicsChanged();
};

