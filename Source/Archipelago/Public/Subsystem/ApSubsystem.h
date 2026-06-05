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

enum class EDataType
{
	Number,
	Object,
	LargeInteger
};

struct FPendingServerDataBase
{
	FString Key;

	EDataType Type;
};

template<EDataType JsonType, typename TValue>
struct FPendingServerData : FPendingServerDataBase
{
	FPendingServerData() { Type = JsonType; }

	TFunction<void(const FString&, TValue)> Callback;
};

struct FUpdatedServerDataBase
{
	FString Key;

	EDataType Type;
};

template<EDataType JsonType, typename TValue>
struct FUpdatedServerData : FUpdatedServerDataBase
{
	FUpdatedServerData() { Type = JsonType; }

	TFunction<void(const FString&, TValue, TValue, int)> Callback;
};

typedef FPendingServerData<EDataType::Number, const uint64*> UInt64Get;
typedef FPendingServerData<EDataType::Object, const TSharedRef<FJsonValue>&> JsonGet;

typedef FUpdatedServerData<EDataType::Number, const uint64*> UInt64Update;
typedef FUpdatedServerData<EDataType::Object, const TSharedRef<FJsonValue>&> JsonUpdate;
typedef FUpdatedServerData<EDataType::LargeInteger, const TSharedRef<FJsonValueNumberString>&> LargeIntegerUpdate;

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

	 //data storage
	void MonitorDataStoreJsonObjectValue(const FString& key, TFunction<void(const FString&, const TSharedRef<FJsonValue>&, const TSharedRef<FJsonValue>&, int)> callback);
	void MonitorInt64DataStoreValue(const TArray<FString>& keys, TFunction<void(const FString&, const uint64*, const uint64*, int)> callback);
	void MonitorDataStoreUnboundedNumberValue(const FString& key, TFunction<void(const FString&, const TSharedRef<FJsonValueNumberString>&, const TSharedRef<FJsonValueNumberString>&, int)> callback);
	void ModifyDataStorageInt64NoCap(const FString& key, int64 amount) const;
	void ModifyDataStorageInt64(const FString& key, int64 amount) const;
	void ModifyDataStorageInt64(const TMap<FString, int64>& adjustmentsPerKey) const;
	void GetDataStorageJsonFields(const TSet<FString>& keys, TFunction<void(const FString&, const TSharedRef<FJsonValue>&)> callback);
	void GetDataStorageInt64Fields(const TSet<FString>& keys, TFunction<void(const FString&, const uint64*)> callback);
	void SetDataStorageJsonField(const FString& key, const TSharedRef<FJsonObject>& json) const;

	void SetVaultState(const TMap<FString, TMap<FString, float>>& vaultItemTraitMapping) const;
	void SetGiftBoxState(bool open, const TSet<FString>& acceptedTraits) const;
	void SendGift(const FApGift& giftToSend) const;
	void ProcessGifts(const TSet<FString>& acceptedIds, const TArray<FApGift>& rejectedGifts) const;
	//

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FApConfigurationStruct GetConfig() const { return config; }

	FString GetApItemName(int64 itemId) const;

	TMap<FApPlayer, TTuple<FString, FString>> GetAllApPlayers() const;
	static TSet<int64> GetAllLocations();

	void MarkGameAsDone() const;
	static void Say(FString message);

	FApNetworkItem ScoutLocation(int64 locationId);
	TMap<int64, FApNetworkItem> ScoutLocation(const TSet<int64>& locationIds);

	FString GetSlotDataJson() const;

	void CreateLocationHint(int64 locationId, bool spam = false) const;
	void CreateLocationHint(const TSet<int64>& locationIds, bool spam = false) const;

	void CheckLocation(int64 locationId) const;
	void CheckLocation(const TSet<int64>& locationIds) const;

	void SetItemReceivedCallback(TFunction<void(int64, bool)> onItemReceived);
	void SetLocationCheckedCallback(TFunction<void(int64)> onLocationChecked);
	void SetDeathLinkReceivedCallback(TFunction<void(FString, FString)> onDeathLinkReceived);
	void SetReconnectCallback(TFunction<void(void)> onReconnect);
	void BounceReceivedCallback(AP_Bounce bounce);

	void AddChatMessage(FText, FLinearColor);

	void EnableDeathLink() const;
	void TriggerDeathLink(FString source, FString cause);

private:
	AApConnectionInfoSubsystem* connectionInfoSubsystem;

	TSharedPtr<TPromise<TMap<int64, FApNetworkItem>>> locationScoutingPromise = nullptr;

	TArray<TFunction<void(int64, bool)>> itemReceivedCallbacks;
	TArray<TFunction<void(int64)>> locationCheckedCallbacks;
	TArray<TFunction<void(FString, FString)>> deathLinkReceivedCallbacks;
	TArray<TFunction<void(void)>> onReconnectCallbacks;

	TMap<FString, TSharedRef<FPendingServerDataBase>> dataStorageRetrievalCallbacks;
	TMap<FString, TSharedRef<FUpdatedServerDataBase>> dataStoreReplyCallbacks;
	TArray<FString> processedCallbacks;

	TSet<FGuid> sendDeathLinkReferences = TSet<FGuid>();

	void LocationScoutedCallback(std::vector<AP_NetworkItem>) const;
	void DeathLinkReceivedCallback(const FString& source, const FString& cause);
	void PackageReceivedCallback(const std::string json);

	TQueue<TTuple<int64, bool>> ReceivedItems;
	TQueue<int64> CheckedLocations;
	TQueue<TPair<FString, FString>> PendingDeathlinks;
	TQueue<TPair<FString, FLinearColor>> ChatMessageQueue;
	TQueue<FString> JsonPackageQueue;
	std::atomic_bool isReconnect = false;

	FApConfigurationStruct config;

	bool canRecieveChat;

	bool InitializeTick(FDateTime connectingStartedTime, int timeout) const;

	void ConnectToArchipelago();
	void TimeoutConnection() const;

	void CheckConnectionState() const;

	void ProcessReceivedItems();
	void ProcessCheckedLocations();
	void ProcessDeadlinks();
	void ProcessApPackages();
	void HandleAPMessages();

	void OnRetrievedPackage(const TSharedPtr<FJsonObject> json);
	void OnSetReply(const TSharedPtr<FJsonObject> Value);

	 //internal data storage functions
	TSharedRef<FJsonObject> BuildSendGift(const FApGift& giftToSend) const;
	static TSharedRef<FJsonObject> BuildNumericSetPacket(const FString& key, int64 value, uint64 kappa = 0);

	void Send(const TSharedRef<FJsonObject>& json) const;
	void Send(const TArray<TSharedRef<FJsonObject>>& jsons) const;

	void CallDataStorageCallbackForRetrieved(FString key, const TSharedPtr<FJsonValue> json);
	void CallDataStorageCallbackForSetReply(FString key, const TSharedPtr<FJsonValue>& originalValue, const TSharedPtr<FJsonValue>& value, int slot);
	 //

    // TODO do we want to keep this around or call AApMessagingSubsystem::DisplayMessage directly?
	void SendChatMessage(const FString& Message, const FLinearColor& Color) const;

	template<typename RetType>
	RetType CallOnGameThread(TFunction<RetType()> InFunction) const;
	template<>
	FORCEINLINE void CallOnGameThread(TFunction<void()> InFunction) const;
};
