#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <numeric>

#include "CoreMinimal.h"

#include "FGSchematicManager.h"
#include "FGResearchManager.h"
#include "FGGamePhaseManager.h"
#include "FGResourceSinkSubsystem.h"

#include "GenericPlatform/GenericPlatformProcess.h"
#include "Patching/BlueprintHookHelper.h"
#include "Patching/BlueprintHookManager.h"
#include "Registry/ModContentRegistry.h"
#include "Kismet/BlueprintLoggingLibrary.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Subsystem/ApTrapSubsystem.h"
#include "Configuration/ModConfiguration.h"
#include "Configuration/ConfigProperty.h"
#include "Configuration/ConfigManager.h"
#include "Kismet/RuntimeBlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "Configuration/Properties/ConfigPropertySection.h"
#include "Templates/SubclassOf.h"
#include "FGChatManager.h"
#include "Module/ModModule.h"
#include "Reflection/ClassGenerator.h"
#include "Buildables/FGBuildable.h"
#include "Buildables/FGBuildableAutomatedWorkBench.h"

#include "ApConfigurationStruct.h"
#include "Data/ApSlotData.h"
#include "Data/ApTypes.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "Subsystem/ApGoalSubsystem.h"

#include "ContentLibSubsystem.h"
#include "CLSchematicBPFLib.h"
#include "CLItemBPFLib.h"
#include "CLRecipeBPFLib.h"
#include "BPFContentLib.h"

#include "Archipelago.h"
#include "Archipelago_Satisfactory.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApSubsystem, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogApChat, All, All);

#include "ApSubsystem.generated.h"

UENUM(BlueprintType)
enum EApConnectionState {
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
	EApConnectionState ConnectionState;
	UPROPERTY(BlueprintReadOnly)
	FText ConnectionStateDescription;

	UPROPERTY(BlueprintReadOnly)
	int currentPlayerTeam;

	UPROPERTY(BlueprintReadOnly)
	int currentPlayerSlot;

	// Get a copy of the subsystem
	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get ApSubsystem"))
	static AApSubsystem* Get();
	static AApSubsystem* Get(class UWorld* world);

	UFUNCTION(BlueprintCallable)
	void DispatchLifecycleEvent(ELifecyclePhase phase);

	void MonitorDataStoreValue(std::string key, AP_DataType dataType, std::string defaultValue, std::function<void(AP_SetReply)> callback);
	void SetServerData(AP_SetServerDataRequest* setDataRequest);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE TEnumAsByte<EApConnectionState> GetConnectionState() const { return TEnumAsByte<EApConnectionState>(ConnectionState); };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE FApSlotData GetSlotData() const { return slotData; };

	FString GetApItemName(int64 itemId);

	void SetGiftBoxState(bool open);
	bool SendGift(FApSendGift giftToSend);
	TArray<FApReceiveGift> GetGifts();
	void RejectGift(FString id);
	void AcceptGift(FString id);
	TMap<FApPlayer, FApGiftBoxMetaData> GetAcceptedTraitsPerPlayer();

	TArray<FApPlayer> GetAllApPlayers();

private:
	static std::map<std::string, std::function<void(AP_SetReply)>> callbacks;

	static void SetReplyCallback(AP_SetReply setReply);
	static void ItemClearCallback();
	static void ItemReceivedCallback(int64 id, bool notify, bool isFromServer);
	static void LocationCheckedCallback(int64 id);
	static void LocationScoutedCallback(std::vector<AP_NetworkItem>);
	static void ParseSlotData(std::string json);

	AFGSchematicManager* SManager;
	AFGResearchManager* RManager;

	UContentLibSubsystem* contentLibSubsystem;
	UModContentRegistry* contentRegistry;
	AApPortalSubsystem* portalSubsystem;
	AApMappingsSubsystem* mappingSubsystem;
	AApTrapSubsystem* trapSubsystem;
	AApGoalSubsystem* goalSubsystem;

	TMap<TSubclassOf<class UFGSchematic>, TArray<AP_NetworkItem>> locationsPerMilestone;
	TMap<TSubclassOf<class UFGSchematic>, TArray<AP_NetworkItem>> locationsPerMamNode;
	TMap<int64, TSubclassOf<class UFGSchematic>> ItemSchematics;
	TQueue<int64> ReceivedItems;
	TQueue<TPair<FString, FLinearColor>> ChatMessageQueue;

	TArray<AP_NetworkItem> scoutedLocations;
	bool hasScoutedLocations;
	bool areScoutedLocationsReadyToParse;
	bool areRecipiesAndSchematicsInitialized;

	FApSlotData slotData;

	UPROPERTY(SaveGame)
	bool hasSentGoal;

	// TODO
	// UPROPERTY(SaveGame)
	// int64 lastReceivedApNetworkItem

	bool InitializeTick(FApConfigurationStruct config, FDateTime connectingStartedTime);

	void ConnectToArchipelago(FApConfigurationStruct config);
	void TimeoutConnection();

	void CheckConnectionState(FApConfigurationStruct config);
	void ScoutArchipelagoItems();
	void ParseScoutedItemsAndCreateRecipiesAndSchematics();

	void HandleAPMessages();
	void SendChatMessage(const FString& Message, const FLinearColor& Color);

	void CreateSchematicBoundToItemId(int64 item, TSharedRef<FApRecipeItem> recipe);
	FContentLib_UnlockInfoOnly CreateUnlockInfoOnly(AP_NetworkItem item);
	void UpdateInfoOnlyUnlockWithBuildingInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, AP_NetworkItem* item, TSharedRef<FApBuildingItem> itemInfo);
	void UpdateInfoOnlyUnlockWithRecipeInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, AP_NetworkItem* item, TSharedRef<FApRecipeItem> itemInfo);
	void UpdateInfoOnlyUnlockWithItemBundleInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, AP_NetworkItem* item, TSharedRef<FApItem> itemInfo);
	void UpdateInfoOnlyUnlockWithSchematicInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, AP_NetworkItem* item, TSharedRef<FApSchematicItem> itemInfo);
	void UpdateInfoOnlyUnlockWithGenericApInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, AP_NetworkItem* item);
	void CreateHubSchematic(FString name, TSubclassOf<UFGSchematic> factorySchematic, TArray<AP_NetworkItem> apItems);
	void CreateMamSchematic(FString name, TSubclassOf<UFGSchematic> factorySchematic, TArray<AP_NetworkItem> apItems);
	
	UFUNCTION() //required for event binding
	void OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic);
	UFUNCTION() //required for event binding
	void OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic);
};
