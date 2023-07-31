#pragma once

#include <functional>
#include <map>

#include "CoreMinimal.h"

#include "FGSchematicManager.h"
#include "FGResearchManager.h"

#include "Patching/BlueprintHookHelper.h"
#include "Patching/BlueprintHookManager.h"
#include "Registry/ModContentRegistry.h"
#include "Kismet/BlueprintLoggingLibrary.h"
#include "Subsystem/ModSubsystem.h"
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

#include "ApConfigurationStruct.h"
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

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void DispatchLifecycleEvent(ELifecyclePhase phase);

	void MonitorDataStoreValue(std::string key, AP_DataType dataType, std::string defaultValue, std::function<void(AP_SetReply)> callback);
	void SetServerData(AP_SetServerDataRequest* setDataRequest);

	static FApConfigurationStruct GetActiveConfig();

private:
	static std::map<std::string, std::function<void(AP_SetReply)>> callbacks;
	static TMap<int64_t, std::string> ItemIdToSchematicName;
	static std::vector<AP_NetworkItem>* lastLocationScout;

	static void SetReplyCallback(AP_SetReply setReply);
	static void ItemClearCallback();
	static void ItemReceivedCallback(int64_t id, bool notify);
	static void LocationCheckedCallback(int64_t id);
	static void LocationScoutedCallback(std::vector<AP_NetworkItem>);

	FTimerHandle connectionTimeoutHandler;
	void ConnectToArchipelago(FApConfigurationStruct config);
	void TimeoutConnectionIfNotConnected();

	void SendChatMessage(const FString& Message, const FLinearColor& Color);
	void HintUnlockedHubRecipies();

	UFUNCTION()
	void OnResearchCompleted(TSubclassOf<class UFGSchematic> schematic);
};
