#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <numeric>

#include "CoreMinimal.h"

#include "FGSchematicManager.h"
#include "FGResearchManager.h"

#include "GenericPlatform/GenericPlatformProcess.h"
#include "Patching/BlueprintHookHelper.h"
#include "Patching/BlueprintHookManager.h"
#include "Registry/ModContentRegistry.h"
#include "Kismet/BlueprintLoggingLibrary.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
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

#include "ApConfigurationStruct.h"

#include "ContentLibSubsystem.h"
#include "CLSchematicBPFLib.h"
#include "CLItemBPFLib.h"
#include "CLRecipeBPFLib.h"

#include "Archipelago.h"

DECLARE_LOG_CATEGORY_EXTERN(ApSubsystem, Log, All);

#include "ApSubsystem.generated.h"

UCLASS(Blueprintable)
class ARCHIPELAGO_API AApSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AApSubsystem();

	UPROPERTY()
		AFGSchematicManager* SManager;
	UPROPERTY()
		AFGResearchManager* RManager;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	bool isInitialized = false;
	bool isConnecting = false;
	TMap<TSubclassOf<class UFGSchematic>, TArray<int64_t>> locationsPerMileStone;

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	bool InitializeTick(FApConfigurationStruct config);

	void DispatchLifecycleEvent(ELifecyclePhase phase);

	void MonitorDataStoreValue(std::string key, AP_DataType dataType, std::string defaultValue, std::function<void(AP_SetReply)> callback);
	void SetServerData(AP_SetServerDataRequest* setDataRequest);

	static FApConfigurationStruct GetActiveConfig();

private:
	static std::map<std::string, std::function<void(AP_SetReply)>> callbacks;
	static TMap<int64_t, std::string> ItemIdToSchematicName;
	static UContentLibSubsystem* ContentLibSubsystem;
	static std::vector<AP_NetworkItem> ScoutedLocations;
	static bool ShouldParseItemsToScout;
	static int FirstHubLocation;
	static int LastHubLocation;

	static void SetReplyCallback(AP_SetReply setReply);
	static void ItemClearCallback();
	static void ItemReceivedCallback(int64_t id, bool notify);
	static void LocationCheckedCallback(int64_t id);
	static void LocationScoutedCallback(std::vector<AP_NetworkItem>);
	static void SlotDataFirstHubLocation(int locationId);
	static void SlotDataLastHubLocation(int locationId);

	FTimerHandle connectionTimeoutHandler;
	void ConnectToArchipelago(FApConfigurationStruct config);
	void TimeoutConnectionIfNotConnected();

	void CheckConnectionState(FApConfigurationStruct config);
	void ParseScoutedItems();
	void HandleAPMessages();
	void SendChatMessage(const FString& Message, const FLinearColor& Color);
	void HintUnlockedHubRecipies();

	void CreateRecipe(AModContentRegistry* contentRegistry, AP_NetworkItem item);
	void CreateItem(AModContentRegistry* contentRegistry, AP_NetworkItem item);
	void CreateHubSchematic(AModContentRegistry* contentRegistry, std::string milestoneName, std::vector<AP_NetworkItem> items);
	
	UFUNCTION() //required for event
	void OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic);
	UFUNCTION() //required for event
	void OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic);
};
