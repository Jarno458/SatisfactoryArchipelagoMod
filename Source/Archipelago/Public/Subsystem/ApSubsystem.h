#pragma once

#include <string>
#include <vector>
#include <atomic>

#include "CoreMinimal.h"
#include "Templates/Function.h"

#include "FGPlayerController.h"

#include "GenericPlatform/GenericPlatformProcess.h"
#include "Subsystem/ModSubsystem.h"
#include "Templates/SubclassOf.h"
#include "Module/ModModule.h"
#include "ApConfigurationStruct.h"
#include "Data/ApTypes.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"

#include "Archipelago.h"
#include "Archipelago_Satisfactory.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApSubsystem, Log, All);

#include "ApSubsystem.generated.h"

USTRUCT()
struct ARCHIPELAGO_API FBounceDayo
{
	GENERATED_BODY()

	UPROPERTY()
	FString Source;
	UPROPERTY()
	FString Cause;
	UPROPERTY()
	double Time;
	UPROPERTY()
	FString Reference;
};

UCLASS()
class ARCHIPELAGO_API AApSubsystem : public AModSubsystem
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

public:
	// Get subsystem. Server-side only, null on clients
	static AApSubsystem* Get(class UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get ApSubsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApSubsystem* Get(UObject* worldContext);

	void DispatchLifecycleEvent(ELifecyclePhase phase);

	void MonitorDataStoreValue(FString keyFString, TFunction<void()> callback);
	void MonitorDataStoreValue(TArray<FString> keys, AP_DataType datatype, TFunction<void(AP_SetReply)> callback);
	void MonitorUnboundedIntergerDataStoreValue(FString keyFString, TFunction<void(AP_SetReply)> callback);
	void ModifyEnergyLink(int64 amount);
	void ModifyDataStorageInt64(FString key, int64 amount);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FApConfigurationStruct GetConfig() const { return config; };

	FString GetApItemName(int64 itemId) const;

	void SetGiftBoxState(bool open, const TSet<FString>& acceptedTraits) const;
	bool SendGift(FApSendGift giftToSend) const;
	TArray<FApReceiveGift> GetGifts() const;
	void RejectGift(TSet<FString> ids) const;
	void AcceptGift(TSet<FString> ids) const;
	TMap<FApPlayer, FApTraitBits> GetAcceptedTraitsPerPlayer() const;
	TMap<FApPlayer, TTuple<FString, FString>> GetAllApPlayers() const;
	static TSet<int64> GetAllLocations();

	void MarkGameAsDone() const;
	static void Say(FString message);

	FApNetworkItem ScoutLocation(int64 locationId);
	TMap<int64, FApNetworkItem> ScoutLocation(const TSet<int64>& locationIds);

	FString GetSlotDataJson() const;

	void CreateLocationHint(int64 locationId, bool spam = false);
	void CreateLocationHint(const TSet<int64>& locationIds, bool spam = false) const;

	void CheckLocation(int64 locationId);
	void CheckLocation(const TSet<int64>& locationIds) const;

	void SetItemReceivedCallback(TFunction<void(int64, bool)> onItemReceived);
	void SetLocationCheckedCallback(TFunction<void(int64)> onLocationChecked);
	void SetDeathLinkReceivedCallback(TFunction<void(FString, FString)> onDeathLinkReceived);
	void SetReconnectCallback(TFunction<void(void)> onReconnect);
	void BounceReceivedCallback(AP_Bounce bounce);

	void AddChatMessage(FText, FLinearColor);

	void EnableDeathLink();
	void TriggerDeathLink(FString source, FString cause);

private:
	AApConnectionInfoSubsystem* connectionInfoSubsystem;

	TSharedPtr<TPromise<TMap<int64, FApNetworkItem>>> locationScoutingPromise = nullptr;

	TMap<FString, TFunction<void(AP_SetReply)>> dataStoreCallbacks;
	TArray<TFunction<void(int64, bool)>> itemReceivedCallbacks;
	TArray<TFunction<void(int64)>> locationCheckedCallbacks;
	TArray<TFunction<void(FString, FString)>> deathLinkReceivedCallbacks;
	TArray<TFunction<void(void)>> onReconnectCallbacks;

	TSet<FGuid> sendDeathLinkReferences = TSet<FGuid>();

	void SetReplyCallback(AP_SetReply setReply);
	void LocationScoutedCallback(std::vector<AP_NetworkItem>) const;
	void DeathLinkReceivedCallback(const FString& source, const FString& cause);

	TQueue<TTuple<int64, bool>> ReceivedItems;
	TQueue<int64> CheckedLocations;
	TQueue<TPair<FString, FString>> PendingDeathlinks;
	TQueue<TPair<FString, FLinearColor>> ChatMessageQueue;
	std::atomic_bool isReconnect = false;

	FApConfigurationStruct config;

	bool canRecieveChat;

	bool InitializeTick(FDateTime connectingStartedTime, int timeout);

	void ConnectToArchipelago();
	void TimeoutConnection() const;

	void CheckConnectionState();

	void ProcessReceivedItems();
	void ProcessCheckedLocations();
	void ProcessDeadlinks();

	void HandleAPMessages();

	// TODO do we want to keep this around or call AApMessagingSubsystem::DisplayMessage directly?
	void SendChatMessage(const FString& Message, const FLinearColor& Color) const;

	template<typename RetType>
	RetType CallOnGameThread(TFunction<RetType()> InFunction) const;
	template<>
	FORCEINLINE void CallOnGameThread(TFunction<void()> InFunction) const;
};
