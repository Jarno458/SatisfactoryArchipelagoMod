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

#include "FGPlayerController.h"
#include "FGCharacterPlayer.h"

#include "GenericPlatform/GenericPlatformProcess.h"
#include "Registry/ModContentRegistry.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Subsystem/ApTrapSubsystem.h"
#include "Configuration/ModConfiguration.h"
#include "Configuration/ConfigManager.h"
#include "Templates/SubclassOf.h"
#include "Module/ModModule.h"
#include "ApConfigurationStruct.h"
#include "Data/ApTypes.h"
#include "Data/ApTypes.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Subsystem/ApMessagingSubsystem.h"

#include "Archipelago.h"
#include "Archipelago_Satisfactory.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApSubsystem, Log, All);

#include "ApSubsystem.generated.h"

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
	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void GatherDependencies_Implementation(TArray<UObject*>& out_dependentObjects) override {};
	virtual bool NeedTransform_Implementation() override { return false; };
	virtual bool ShouldSave_Implementation() const override { return true; };
	// End IFSaveInterface

public:
	// Get subsystem. Server-side only, null on clients
	static AApSubsystem* Get(class UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get ApSubsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApSubsystem* Get(UObject* worldContext);

	void DispatchLifecycleEvent(ELifecyclePhase phase);

	void MonitorDataStoreValue(FString keyFString, TFunction<void()> callback);
	void MonitorDataStoreValue(FString keyFString, AP_DataType dataType, FString defaultValueFString, TFunction<void(AP_SetReply)> callback);
	void ModdifyEnergyLink(long amount, FString defaultValueFString);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FApConfigurationStruct GetConfig() const { return config; };

	FString GetApItemName(int64 itemId);

	void SetGiftBoxState(bool open, const TSet<FString>& acceptedTraits);
	bool SendGift(FApSendGift giftToSend);
	TArray<FApReceiveGift> GetGifts();
	void RejectGift(TSet<FString> ids);
	void AcceptGift(TSet<FString> ids);
	TMap<FApPlayer, FApTraitBits> GetAcceptedTraitsPerPlayer();
	TArray<FApPlayer> GetAllApPlayers();

	void MarkGameAsDone();
	void Say(FString message);

	FApNetworkItem ScoutLocation(int64 locationId);
	TMap<int64, FApNetworkItem> ScoutLocation(const TSet<int64>& locationIds);

	void CreateLocationHint(int64 locationId, bool spam = false);
	void CreateLocationHint(const TSet<int64>& locationIds, bool spam = false);

	void CheckLocation(int64 locationId);
	void CheckLocation(const TSet<int64>& locationIds);

	void SetItemReceivedCallback(TFunction<void(int64, bool)> onItemReceived);
	void SetLocationCheckedCallback(TFunction<void(int64)> onLocationChecked);
	void SetDeathLinkReceivedCallback(TFunction<void(FText)> onDeathLinkReceived);
	void SetReconnectCallback(TFunction<void(void)> onReconnect);

	void AddChatMessage(FText, FLinearColor);

	void TriggerDeathLink();
private:
	static AApSubsystem* callbackTarget;

	AApConnectionInfoSubsystem* connectionInfoSubsystem;

	TSharedPtr<TPromise<TMap<int64, FApNetworkItem>>> locationScoutingPromise = nullptr;

	TMap<FString, TFunction<void(AP_SetReply)>> dataStoreCallbacks;
	TArray<TFunction<void(int64, bool)>> itemReceivedCallbacks;
	TArray<TFunction<void(int64)>> locationCheckedCallbacks;
	TArray<TFunction<void(FText)>> deathLinkReceivedCallbacks;
	TArray<TFunction<void(void)>> onReconnectCallbacks;

	static void SetReplyCallback(AP_SetReply setReply);
	static void ItemClearCallback();
	static void ItemReceivedCallback(int64 id, bool notify, bool isFromServer);
	static void LocationCheckedCallback(int64 id);
	static void LocationScoutedCallback(std::vector<AP_NetworkItem>);
	static void ParseSlotData(std::string json);
	static void DeathLinkReceivedCallback(std::string source, std::string cause);
	static void LogFromAPCpp(std::string message);

	UPROPERTY(SaveGame)
	FApConfigurationStruct config;

	TQueue<TTuple<int64, bool>> ReceivedItems;
	TQueue<int64> CheckedLocations;
	TQueue<FText> PendingDeathlinks;
	TQueue<TPair<FString, FLinearColor>> ChatMessageQueue;
	std::atomic_bool isReconnect = false;

	bool canRecieveChat;

	bool InitializeTick(FDateTime connectingStartedTime);

	void ConnectToArchipelago();
	void TimeoutConnection();

	void CheckConnectionState();
	void ScoutArchipelagoItems();
	void ParseScoutedItemsAndCreateRecipiesAndSchematics();
	void LoadRoomInfo();

	void ProcessReceivedItems();
	void ProcessCheckedLocations();
	void ProcessDeadlinks();

	void HandleAPMessages();

	// TODO do we want to keep this around or call AApMessagingSubsystem::DisplayMessage directly?
	void SendChatMessage(const FString& Message, const FLinearColor& Color);

	static void AbortGame(FText reason);

	template<typename RetType>
	RetType CallOnGameThread(TFunction<RetType()> InFunction);
	template<>
	FORCEINLINE void CallOnGameThread(TFunction<void()> InFunction);
};
