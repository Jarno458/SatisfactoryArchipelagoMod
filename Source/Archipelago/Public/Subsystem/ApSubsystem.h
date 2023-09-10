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
#include "Data/ApMappings.h"
#include "ApPortalSubsystem.h"

#include "ContentLibSubsystem.h"
#include "CLSchematicBPFLib.h"
#include "CLItemBPFLib.h"
#include "CLRecipeBPFLib.h"
#include "BPFContentLib.h"

#include "Archipelago.h"

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
	virtual void GatherDependencies_Implementation(TArray< UObject* >& out_dependentObjects) override {};
	virtual bool NeedTransform_Implementation() override { return false; };
	virtual bool ShouldSave_Implementation() const override { return true; };
	// End IFSaveInterface

public:
	EApConnectionState ConnectionState;
	UPROPERTY(BlueprintReadOnly)
	FText ConnectionStateDescription;

	// Get a copy of the subsystem
	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get ApSubsystem"))
	static AApSubsystem* Get();
	static AApSubsystem* Get(class UWorld* world);

	static FApConfigurationStruct GetActiveConfig();

	// Called every frame
	bool InitializeTick(FApConfigurationStruct config, FDateTime connectingStartedTime);

	UFUNCTION(BlueprintCallable)
	void DispatchLifecycleEvent(ELifecyclePhase phase);

	void MonitorDataStoreValue(std::string key, AP_DataType dataType, std::string defaultValue, std::function<void(AP_SetReply)> callback);
	void SetServerData(AP_SetServerDataRequest* setDataRequest);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TEnumAsByte<EApConnectionState> GetConnectionState();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE FApSlotData GetSlotData() const { return slotData; };

	FString GetItemName(int64_t);
	FString GetLocationName(int64_t);

private:
	static std::map<std::string, std::function<void(AP_SetReply)>> callbacks;

	static void SetReplyCallback(AP_SetReply setReply);
	static void ItemClearCallback();
	static void ItemReceivedCallback(int64_t id, bool notify);
	static void LocationCheckedCallback(int64_t id);
	static void LocationScoutedCallback(std::vector<AP_NetworkItem>);
	static void ParseSlotData(std::string json);

	AFGSchematicManager* SManager;
	AFGResearchManager* RManager;
	AFGGamePhaseManager* PManager;

	UContentLibSubsystem* contentLibSubsystem;
	AModContentRegistry* contentRegistry;
	AFGResourceSinkSubsystem* resourceSinkSubsystem;
	AApPortalSubsystem* portalSubsystem;

	TMap<TSubclassOf<class UFGSchematic>, TArray<AP_NetworkItem>> locationsPerMilestone;
	TMap<TSubclassOf<class UFGSchematic>, TArray<AP_NetworkItem>> locationsPerMamNode;
	TMap<int64_t, TSubclassOf<class UFGSchematic>> ItemSchematics;
	TQueue<int64_t> ReceivedItems;
	TQueue<TPair<FString, FLinearColor>> ChatMessageQueue;

	TArray<AP_NetworkItem> scoutedLocations;
	bool shouldParseItemsToScout;

	FApSlotData slotData;

	UPROPERTY(SaveGame)
	bool hasSentGoal;

	// TODO
	// UPROPERTY(SaveGame)
	// int64_t lastReceivedApNetworkItem

	void ConnectToArchipelago(FApConfigurationStruct config);
	void TimeoutConnection();

	void CheckConnectionState(FApConfigurationStruct config);
	void ParseScoutedItems();
	void HandleAPMessages();
	void HintUnlockedHubRecipies();

	void SendChatMessage(const FString& Message, const FLinearColor& Color);

	void CreateSchematicBoundToItemId(int64_t item);
	FContentLib_UnlockInfoOnly CreateUnlockInfoOnly(TMap<FName, FAssetData> recipeAssets, TMap<FName, FAssetData> itemDescriptorAssets, AP_NetworkItem item);
	void UpdateInfoOnlyUnlockWithBuildingInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, TMap<FName, FAssetData> buildingRecipyAssets, AP_NetworkItem* item);
	void UpdateInfoOnlyUnlockWithRecipeInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, TMap<FName, FAssetData> recipeAssets, AP_NetworkItem* item);
	void UpdateInfoOnlyUnlockWithItemInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, TMap<FName, FAssetData> itemDescriptorAssets, AP_NetworkItem* item);
	void UpdateInfoOnlyUnlockWithGenericApInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, AP_NetworkItem* item);
	void CreateRecipe(AP_NetworkItem item);
	void CreateDescriptor(AP_NetworkItem item);
	void CreateHubSchematic(TMap<FName, FAssetData> recipeAssets, TMap<FName, FAssetData> itemDescriptorAssets, FString name, TSubclassOf<UFGSchematic> factorySchematic, TArray<AP_NetworkItem> apItems);
	void CreateMamSchematic(TMap<FName, FAssetData> recipeAssets, TMap<FName, FAssetData> itemDescriptorAssets, FString name, TSubclassOf<UFGSchematic> factorySchematic, TArray<AP_NetworkItem> apItems);
	
	UFGRecipe* GetRecipeByName(TMap<FName, FAssetData> recipeAssets, FString name);
	UFGItemDescriptor* GetItemDescriptorByName(TMap<FName, FAssetData> itemDescriptorAssets, FString name);

	UFUNCTION() //required for event binding
	void OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic);
	UFUNCTION() //required for event binding
	void OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic);
};
