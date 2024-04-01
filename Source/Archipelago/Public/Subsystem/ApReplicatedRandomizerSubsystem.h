#pragma once

#include "CoreMinimal.h"

#include "FGSchematicManager.h"
#include "FGResearchManager.h"
#include "FGGamePhaseManager.h"
#include "FGPlayerController.h"
#include "FGCharacterPlayer.h"

#include "GenericPlatform/GenericPlatformProcess.h"
#include "Registry/ModContentRegistry.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Subsystem/ApTrapSubsystem.h"
#include "Configuration/ModConfiguration.h"
#include "Configuration/ConfigProperty.h"
#include "Configuration/Properties/ConfigPropertyInteger.h"
#include "Configuration/Properties/ConfigPropertyBool.h"
#include "Configuration/ConfigManager.h"
#include "Engine/Engine.h"
#include "Configuration/Properties/ConfigPropertySection.h"
#include "Templates/SubclassOf.h"
#include "FGChatManager.h"
#include "Module/ModModule.h"
#include "Reflection/ClassGenerator.h"
#include "Buildables/FGBuildable.h"
#include "Buildables/FGBuildableAutomatedWorkBench.h"
#include "Unlocks/FGUnlockInfoOnly.h"

#include "ApConfigurationStruct.h"
#include "Data/ApSlotData.h"
#include "Data/ApTypes.h"
#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"

#include "CLSchematicBPFLib.h"
#include "BPFContentLib.h"
#include "Configuration/FreeSamplesConfigurationStruct.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApReplicatedRandomizerSubsystem, Log, All);

#include "ApReplicatedRandomizerSubsystem.generated.h"

UCLASS()
class ARCHIPELAGO_API AApReplicatedRandomizerSubsystem : public AModSubsystem//, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AApReplicatedRandomizerSubsystem ();

protected:
	// Called when the game starts or when spawned
	//virtual void BeginPlay() override;
	//virtual void Tick(float DeltaTime) override;
	//virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	// Begin IFGSaveInterface
	/*virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void GatherDependencies_Implementation(TArray<UObject*>& out_dependentObjects) override {};
	virtual bool NeedTransform_Implementation() override { return false; };
	virtual bool ShouldSave_Implementation() const override { return true; };*/
	// End IFSaveInterface

public:
	static AApReplicatedRandomizerSubsystem* Get(class UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Ap Client Replicated Randomizer Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApReplicatedRandomizerSubsystem* Get(UObject* worldContext);

	void DispatchLifecycleEvent(ELifecyclePhase phase, TArray<TSubclassOf<UFGSchematic>> apHardcodedSchematics);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool IsInitialized() const { return isInitialized; };

private:
	//AFGSchematicManager* SManager;
	//AFGResearchManager* RManager;
	//AFGGamePhaseManager* phaseManager;
	//AFGUnlockSubsystem* unlockSubsystem;

	UContentLibSubsystem* contentLibSubsystem;
	UModContentRegistry* contentRegistry;

	AApSubsystem* ap;
	//AApPortalSubsystem* portalSubsystem;
	AApMappingsSubsystem* mappingSubsystem;
	//AApTrapSubsystem* trapSubsystem;


	TArray<TSubclassOf<UFGSchematic>> hardcodedSchematics;
	TMap<TSubclassOf<class UFGSchematic>, TArray<FApNetworkItem>> locationsPerMilestone;
	TMap<TSubclassOf<class UFGSchematic>, FApNetworkItem> locationPerMamNode;
	TMap<TSubclassOf<class UFGSchematic>, FApNetworkItem> locationPerShopNode;
	TMap<int64, TSubclassOf<class UFGSchematic>> ItemSchematics;
	TArray<TSubclassOf<class UFGSchematic>> inventorySlotRecipes;

	UTexture2D* collectedIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Archipelago/Assets/SourceArt/ArchipelagoAssetPack/AP-Black.AP-Black"));
	UClass* workshopComponent = LoadClass<UObject>(nullptr, TEXT("/Game/FactoryGame/Buildable/-Shared/WorkBench/BP_WorkshopComponent.BP_WorkshopComponent_C"));

	bool isInitialized = false;

	//std::atomic_bool hasScoutedLocations;
	//std::atomic_bool areScoutedLocationsReadyToParse;
	//std::atomic_bool areRecipiesAndSchematicsInitialized;
	//std::atomic_bool hasLoadedRoomInfo;

	/*void TimeoutConnection();

	void CheckConnectionState();
	void ScoutArchipelagoItems();
	void ParseScoutedItemsAndCreateRecipiesAndSchematics();
	void LoadRoomInfo();
	UConfigPropertySection* GetConfigurationRootSection(FConfigId configId);
	bool UpdateFreeSamplesConfiguration();
	void SetMamEnhancerConfigurationHooks();
	UFUNCTION() //required for event binding
	void LockMamEnhancerSpoilerConfiguration();*/

	//void ReceiveItems();
	//void AwardItem(int64 itemId, bool isFromServer);
	void HandleCheckedLocations();
	//AFGCharacterPlayer* GetLocalPlayer();
	bool IsCollected(UFGUnlock* unlock);
	void Collect(UFGUnlock* unlock, FApNetworkItem& networkItem);
	//void HandleDeathLink();
	//void HandleInstagib(AFGCharacterPlayer* player);

	//void HandleAPMessages();

	// TODO do we want to keep this around or call AApMessagingSubsystem::DisplayMessage directly?
	//void SendChatMessage(const FString& Message, const FLinearColor& Color);

	void CreateSchematicBoundToItemId(int64 item, TSharedRef<FApRecipeItem> recipe);
	FContentLib_UnlockInfoOnly CreateUnlockInfoOnly(FApNetworkItem item);
	void UpdateInfoOnlyUnlockWithBuildingInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApBuildingItem> itemInfo);
	void UpdateInfoOnlyUnlockWithRecipeInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApRecipeItem> itemInfo);
	void UpdateInfoOnlyUnlockWithItemBundleInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApItem> itemInfo);
	void UpdateInfoOnlyUnlockWithSchematicInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApSchematicItem> itemInfo);
	void UpdateInfoOnlyUnlockWithSpecialInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApSpecialItem> itemInfo);
	void UpdateInfoOnlyUnlockWithGenericApInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item);
	void InitializaHubSchematic(FString name, TSubclassOf<UFGSchematic> factorySchematic, TArray<FApNetworkItem> apItems);
	void InitializaSchematicForItem(TSubclassOf<UFGSchematic> factorySchematic, FApNetworkItem item, bool updateSchemaName);

	/*UFUNCTION() //required for event binding
	void OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic);
	FUNCTION() //required for event binding
	void OnMamResearchTreeUnlocked(TSubclassOf<class UFGResearchTree> researchTree);
	UFUNCTION() //required for event binding
	void OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic);

	void OnAvaiableSchematicsChanged();*/
};

