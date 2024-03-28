#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <numeric>
#include <set>
#include <atomic>

#include "CoreMinimal.h"
#include "Templates/Function.h"

#include "FGSchematicManager.h"
#include "FGResearchManager.h"
#include "FGGamePhaseManager.h"
#include "FGPlayerController.h"
#include "FGCharacterPlayer.h"
#include "FGWorkBench.h"

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
#include "Module/ModModule.h"
#include "Reflection/ClassGenerator.h"
#include "Buildables/FGBuildable.h"
#include "Buildables/FGBuildableAutomatedWorkBench.h"
#include "Unlocks/FGUnlockInfoOnly.h"
#include "FGUnlockSubsystem.h"

#include "ApConfigurationStruct.h"
#include "Data/ApSlotData.h"
#include "Data/ApTypes.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "Subsystem/ApMessagingSubsystem.h"

#include "CLSchematicBPFLib.h"
#include "BPFContentLib.h"
#include "Configuration/FreeSamplesConfigurationStruct.h"

#include "Archipelago.h"
#include "Archipelago_Satisfactory.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApSubsystem, Log, All);

#include "ApSubsystem.generated.h"

UENUM(BlueprintType)
enum class EApConnectionState : uint8 {
	NotYetAttempted UMETA(DisplayName = "Not Yet Attempted"),
	Connecting UMETA(DisplayName = "Connecting"),
	Connected UMETA(DisplayName = "Connection Successful"),
	ConnectionFailed UMETA(DisplayName = "Connection Failed")
};

UCLASS()
class ARCHIPELAGO_API AApSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AApSubsystem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	// Begin IFGSaveInterface
	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void GatherDependencies_Implementation(TArray<UObject*>& out_dependentObjects) override {};
	virtual bool NeedTransform_Implementation() override { return false; };
	virtual bool ShouldSave_Implementation() const override { return true; };
	// End IFSaveInterface

public:
	EApConnectionState ConnectionState;
	UPROPERTY(BlueprintReadOnly)
	FText ConnectionStateDescription;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	int currentPlayerTeam = 0;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	int currentPlayerSlot = 0;

	static AApSubsystem* Get(class UWorld* world);

	// Get subsystem. Server-side only, null on clients
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get ApSubsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApSubsystem* Get(UObject* worldContext);

	UFUNCTION(BlueprintCallable)
	void DispatchLifecycleEvent(ELifecyclePhase phase, TArray<TSubclassOf<UFGSchematic>> apHardcodedSchematics);

	void MonitorDataStoreValue(FString keyFString, TFunction<void()> callback);
	void MonitorDataStoreValue(FString keyFString, AP_DataType dataType, FString defaultValueFString, TFunction<void(AP_SetReply)> callback);
	void ModdifyEnergyLink(long amount, FString defaultValueFString);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE EApConnectionState GetConnectionState() const { return ConnectionState; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE FApSlotData GetSlotData() const { return slotData; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FApConfigurationStruct GetConfig() const { return config; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool IsInitialized() const { return areRecipiesAndSchematicsInitialized; };

	FString GetApItemName(int64 itemId);

	void SetGiftBoxState(bool open);
	bool SendGift(FApSendGift giftToSend);
	TArray<FApReceiveGift> GetGifts();
	void RejectGift(TSet<FString> ids);
	void AcceptGift(TSet<FString> ids);
	TMap<FApPlayer, FApGiftBoxMetaData> GetAcceptedTraitsPerPlayer();
	TArray<FApPlayer> GetAllApPlayers();

	void MarkGameAsDone();
	void Say(FString message);

private:
	static AApSubsystem* callbackTarget;
	TMap<FString, TFunction<void(AP_SetReply)>> dataStoreCallbacks;

	static void SetReplyCallback(AP_SetReply setReply);
	static void ItemClearCallback();
	static void ItemReceivedCallback(int64 id, bool notify, bool isFromServer);
	static void LocationCheckedCallback(int64 id);
	static void LocationScoutedCallback(std::vector<AP_NetworkItem>);
	static void ParseSlotData(std::string json);
	static void DeathLinkReceivedCallback(std::string source, std::string cause);
	static void LogFromAPCpp(std::string message);

	AFGSchematicManager* SManager;
	AFGResearchManager* RManager;
	AFGGamePhaseManager* phaseManager;
	AFGUnlockSubsystem* unlockSubsystem;

	UContentLibSubsystem* contentLibSubsystem;
	UModContentRegistry* contentRegistry;

	AApPortalSubsystem* portalSubsystem;
	AApMappingsSubsystem* mappingSubsystem;
	AApTrapSubsystem* trapSubsystem;

	AFGCharacterPlayer* currentPlayer;

	UPROPERTY(SaveGame)
	FString roomSeed;

	UPROPERTY(SaveGame)
	FApSlotData slotData;
	UPROPERTY(SaveGame)
	TArray<FApSaveableHubLayout> saveSlotDataHubLayout;

	UPROPERTY(SaveGame)
	TArray<FApNetworkItem> scoutedLocations;

	int currentItemIndex = 0;
	UPROPERTY(SaveGame)
	int lastProcessedItemIndex = 0;
	int lastGamePhase = -1;

	UPROPERTY(SaveGame)
	FApConfigurationStruct config;

	TArray<TSubclassOf<UFGSchematic>> hardcodedSchematics;
	TMap<TSubclassOf<class UFGSchematic>, TArray<FApNetworkItem>> locationsPerMilestone;
	TMap<TSubclassOf<class UFGSchematic>, FApNetworkItem> locationPerMamNode;
	TMap<TSubclassOf<class UFGSchematic>, FApNetworkItem> locationPerShopNode;
	TMap<int64, TSubclassOf<class UFGSchematic>> ItemSchematics;
	TArray<TSubclassOf<class UFGSchematic>> inventorySlotRecipes;
	TQueue<TTuple<int64, bool>> ReceivedItems;
	TQueue<int64> CheckedLocations;
	TQueue<TPair<FString, FLinearColor>> ChatMessageQueue;

	UTexture2D* collectedIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Archipelago/Assets/SourceArt/ArchipelagoAssetPack/AP-Black.AP-Black"));
	UClass* workshopComponent = LoadClass<UObject>(nullptr, TEXT("/Game/FactoryGame/Buildable/-Shared/WorkBench/BP_WorkshopComponent.BP_WorkshopComponent_C"));

	std::atomic_bool hasScoutedLocations;
	std::atomic_bool areScoutedLocationsReadyToParse;
	std::atomic_bool areRecipiesAndSchematicsInitialized;
	std::atomic_bool hasLoadedRoomInfo;

	bool instagib;
	bool awaitingHealty;
	bool canRecieveChat;

	bool InitializeTick(FDateTime connectingStartedTime);

	void ConnectToArchipelago();
	void TimeoutConnection();

	void CheckConnectionState();
	void ScoutArchipelagoItems();
	void ParseScoutedItemsAndCreateRecipiesAndSchematics();
	void LoadRoomInfo();
	UConfigPropertySection* GetConfigurationRootSection(FConfigId configId);
	bool UpdateFreeSamplesConfiguration();
	void SetMamEnhancerConfigurationHooks();
	UFUNCTION() //required for event binding
	void LockMamEnhancerSpoilerConfiguration();

	void ReceiveItems();
	void AwardItem(int64 itemId, bool isFromServer);
	void HandleCheckedLocations();
	AFGCharacterPlayer* GetLocalPlayer();
	bool IsCollected(UFGUnlock* unlock);
	void Collect(UFGUnlock* unlock, FApNetworkItem& networkItem);
	void HandleDeathLink();
	void HandleInstagib(AFGCharacterPlayer* player);

	void HandleAPMessages();

	// TODO do we want to keep this around or call AApMessagingSubsystem::DisplayMessage directly?
	void SendChatMessage(const FString& Message, const FLinearColor& Color);

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
	
	UFUNCTION() //required for event binding
	void OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic);
	UFUNCTION() //required for event binding
	void OnMamResearchTreeUnlocked(TSubclassOf<class UFGResearchTree> researchTree);
	UFUNCTION() //required for event binding
	void OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic);

	void OnAvaiableSchematicsChanged();

	static void AbortGame(FText reason);

	template<typename RetType>
	RetType CallOnGameThread(TFunction<RetType()> InFunction);
	template<>
	FORCEINLINE void CallOnGameThread(TFunction<void()> InFunction);
};
