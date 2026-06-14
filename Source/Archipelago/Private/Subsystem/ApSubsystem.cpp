#include "Subsystem/ApSubsystem.h"

#include "Subsystem/ApMessagingSubsystem.h"
#include "Subsystem/ApSlotDataSubsystem.h"
#include "ApUtils.h"
#include "JsonObjectConverter.h"
#include "Async/Async.h"
#include "SessionSettings/SessionSettingsManager.h"
#include "Settings/SMLOptionsLibrary.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(LogApSubsystem);

#define LOCTEXT_NAMESPACE "Archipelago"

AApSubsystem::AApSubsystem() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.5f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

AApSubsystem* AApSubsystem::Get(UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApSubsystem* AApSubsystem::Get(UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApSubsystem>();
}

void AApSubsystem::ConnectToArchipelago() {
	USessionSettingsManager* SessionSettings = GetWorld()->GetSubsystem<USessionSettingsManager>();

	FString uriFString = USMLOptionsLibrary::GetStringOptionValue(SessionSettings, "Archipelago.Connection.ServerURI").TrimStartAndEnd();
	FString userFString = USMLOptionsLibrary::GetStringOptionValue(SessionSettings, "Archipelago.Connection.UserName").TrimStartAndEnd();
	FString passwordFString = USMLOptionsLibrary::GetStringOptionValue(SessionSettings, "Archipelago.Connection.Password");

	std::string const uri = TCHAR_TO_UTF8(*uriFString);
	std::string const user = TCHAR_TO_UTF8(*userFString);
	std::string const password = TCHAR_TO_UTF8(*passwordFString);

	AP_NetworkVersion apVersion;
	apVersion.major = 0;
	apVersion.minor = 6;
	apVersion.build = 0;

	AP_SetClientVersion(&apVersion);

	AP_Init(uri.c_str(), "Satisfactory", user.c_str(), password.c_str());

	AP_SetItemClearCallback([this]() {
		ReceivedItems.Empty();
		isReconnect = true;
		});
	AP_SetItemRecvCallback([this](int64 itemId, bool notify, bool isFromServer) {
		ReceivedItems.Enqueue(TTuple<int64, bool>(itemId, isFromServer));
		});
	AP_SetLocationCheckedCallback([this](int64 locationId) {
		CheckedLocations.Enqueue(locationId);
		});
	AP_SetLocationInfoCallback([this](std::vector<AP_NetworkItem> scoutedItems) {
		LocationScoutedCallback(std::move(scoutedItems));
		});
	AP_SetLoggingCallback([this](std::string message) {
		UE_LOG(LogApSubsystem, Display, TEXT("LogFromAPCpp: %s"), *UApUtils::FStr(message));
		});
	AP_RegisterBouncedCallback([this](AP_Bounce bounce) {
		BounceReceivedCallback(bounce);
		});
	AP_SetPackageReceivedCallback([this](std::string json) {
		PackageReceivedCallback(json);
		});

	AP_SetGiftingSupported(false);

	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::Starting AP"));
	AP_Start();
}

void AApSubsystem::BeginPlay() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::BeginPlay()"));

	Super::BeginPlay();
}

void AApSubsystem::DispatchLifecycleEvent(ELifecyclePhase phase) {
	if (!HasAuthority())
		UE_LOG(LogApSubsystem, Fatal, TEXT("AApSubsystem()::DispatchLifecycleEvent() Called without authority"));

	if (phase == ELifecyclePhase::CONSTRUCTION) {
		config = FApConfigurationStruct::GetActiveConfig(GetWorld());
	}
	else if (phase == ELifecyclePhase::INITIALIZATION) {
		connectionInfoSubsystem = AApConnectionInfoSubsystem::Get(GetWorld());
		fgcheck(connectionInfoSubsystem);

		connectionInfoSubsystem->ConnectionState = EApConnectionState::Connecting;
		connectionInfoSubsystem->ConnectionStateDescription = LOCTEXT("Connecting", "Connecting...");

		UE_LOG(LogApSubsystem, Display, TEXT("Initiating Archipelago server connection in background..."));
		bool dummy = CallOnGameThread<bool>([this]() {
			ConnectToArchipelago();
			return true;
			});

		UE_LOG(LogApSubsystem, Display, TEXT("Waiting for AP Server connection to succeed..."));
		FDateTime connectingStartedTime = FDateTime::Now();

		FGenericPlatformProcess::ConditionalSleep([this, connectingStartedTime]() {
			return InitializeTick(connectingStartedTime, config.Timeout);
			}, 0.5);
	}
	else if (phase == ELifecyclePhase::POST_INITIALIZATION) {
		SetActorTickEnabled(true);
	}
}

bool AApSubsystem::InitializeTick(FDateTime connectingStartedTime, int timeout) const {
	return CallOnGameThread<bool>([this, connectingStartedTime, timeout]() {
		if (connectionInfoSubsystem->ConnectionState == EApConnectionState::Connecting) {
			if ((FDateTime::Now() - connectingStartedTime).GetSeconds() > timeout)
				TimeoutConnection();
			else
				CheckConnectionState();
		}

		return connectionInfoSubsystem->ConnectionState == EApConnectionState::Connected
			|| connectionInfoSubsystem->ConnectionState == EApConnectionState::ConnectionFailed;
		});
}

void AApSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (connectionInfoSubsystem->ConnectionState != EApConnectionState::Connected)
		return;

	ProcessReceivedItems();
	ProcessCheckedLocations();
	ProcessDeadlinks();
	ProcessApPackages();
	HandleAPMessages();
}

void AApSubsystem::EndPlay(const EEndPlayReason::Type endPlayReason) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::EndPlay(%i)"), endPlayReason);

	Super::EndPlay(endPlayReason);

	CallOnGameThread<void>([]() { AP_Shutdown(); });
}

void AApSubsystem::BounceReceivedCallback(AP_Bounce bounce)
{
	if (bounce.tags == nullptr || bounce.data.empty()) {
		return;
	}

	if (std::ranges::find(*bounce.tags, "DeathLink") != bounce.tags->end()) {
		FString data = UApUtils::FStr(bounce.data);
		const TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*data);

		TSharedPtr<FJsonObject> parsedJson;

		if (!FJsonSerializer::Deserialize(reader, parsedJson))
			return;

		FString referenceString;
		FGuid reference;
		if (parsedJson->TryGetStringField(TEXT("reference"), referenceString) && FGuid::Parse(referenceString, reference))
		{
			if (sendDeathLinkReferences.Contains(reference)) {
				sendDeathLinkReferences.Remove(reference);
				return;
			}
		}

		FString source;
		if (!parsedJson->TryGetStringField(TEXT("source"), source))
			source = "Unknown";

		FString cause;
		if (!parsedJson->TryGetStringField(TEXT("cause"), cause))
			cause = "";

		DeathLinkReceivedCallback(source, cause);
	}
	else {
		UE_LOGFMT(LogApSubsystem, Display, "AApSubsystem::BounceReceivedCallback() unknown bounce packet received");
	}
}

void AApSubsystem::DeathLinkReceivedCallback(const FString& source, const FString& cause) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::DeathLinkReceivedCallback()"));

	TPair<FString, FString> messagePair = TPair<FString, FString>(source, cause);

	PendingDeathlinks.Enqueue(messagePair);
}

void AApSubsystem::LocationScoutedCallback(std::vector<AP_NetworkItem> scoutedLocations) const {
	UE_LOGFMT(LogApSubsystem, Display, "AApSubsystem::LocationScoutedCallback(vector[{0}])", scoutedLocations.size());

	TMap<int64, FApNetworkItem> scoutedLocationsResult = TMap<int64, FApNetworkItem>();

	for (AP_NetworkItem apLocation : scoutedLocations) {
		FApNetworkItem location;
		location.item = apLocation.item;
		location.location = apLocation.location;
		location.player = apLocation.player;
		location.flags = apLocation.flags;
		location.itemName = UApUtils::FStr(apLocation.itemName);
		location.locationName = UApUtils::FStr(apLocation.locationName);
		location.playerName = UApUtils::FStr(apLocation.playerName);

		scoutedLocationsResult.Add(location.location, location);
	}

	if (locationScoutingPromise != nullptr)
		locationScoutingPromise->SetValue(scoutedLocationsResult);
}

void AApSubsystem::MonitorDataStoreJsonObjectValue(const FString& key, TFunction<void(const FString&, const TSharedRef<FJsonValue>&, const TSharedRef<FJsonValue>&, int)> callback) {
	JsonUpdate rawDataUpdate;
	rawDataUpdate.Callback = callback;

	if (dataStoreReplyCallbacks.Contains(key))
		dataStoreReplyCallbacks[key] = MakeShared<JsonUpdate>(rawDataUpdate);
	else
		dataStoreReplyCallbacks.Add(key, MakeShared<JsonUpdate>(rawDataUpdate));

	TArray<TSharedPtr<FJsonValue>> keysArray;
	keysArray.Add(MakeShared<FJsonValueString>(key));

	TSharedRef<FJsonObject> setNotifyCmd = MakeShared<FJsonObject>();
	setNotifyCmd->SetStringField("cmd", "SetNotify");
	setNotifyCmd->SetArrayField("keys", keysArray);

	TArray<TSharedPtr<FJsonValue>> operations;

	TSharedRef<FJsonObject> defaultOperation = MakeShared<FJsonObject>();
	defaultOperation->SetStringField("operation", "default");
	defaultOperation->SetObjectField("value", MakeShared<FJsonObject>());

	operations.Add(MakeShared<FJsonValueObject>(defaultOperation));

	TSharedRef<FJsonObject> setCmd = MakeShared<FJsonObject>();
	setCmd->SetStringField("cmd", "Set");
	setCmd->SetStringField("key", key);
	setCmd->SetObjectField("default", MakeShared<FJsonObject>());
	setCmd->SetArrayField("operations", operations);

	TArray<TSharedRef<FJsonObject>> jsons;

	jsons.Add(setNotifyCmd);
	jsons.Add(setCmd);

	Send(jsons);
}

void AApSubsystem::MonitorInt64DataStoreValue(const TArray<FString>& keys, TFunction<void(const FString&, const uint64*, const uint64*, int)> callback) {
	for (const FString& key : keys) {
		UInt64Update int64Update;
		int64Update.Callback = callback;

		if (dataStoreReplyCallbacks.Contains(key))
			dataStoreReplyCallbacks[key] = MakeShared<UInt64Update>(int64Update);
		else
			dataStoreReplyCallbacks.Add(key, MakeShared<UInt64Update>(int64Update));

		UInt64Get int64Get;
		int64Get.Callback = [callback](const FString& callbackKey, const uint64* callbackValue)
			{
				callback(callbackKey, callbackValue, callbackValue, -1);
			};

		if (dataStorageRetrievalCallbacks.Contains(key))
			dataStorageRetrievalCallbacks[key] = MakeShared<UInt64Get>(int64Get);
		else
			dataStorageRetrievalCallbacks.Add(key, MakeShared<UInt64Get>(int64Get));
	}

	TArray<TSharedPtr<FJsonValue>> keysArray;
	for (const FString& key : keys) {
		keysArray.Add(MakeShared<FJsonValueString>(key));
	}

	TSharedRef<FJsonObject> setNotifyCmd = MakeShared<FJsonObject>();
	setNotifyCmd->SetStringField("cmd", "SetNotify");
	setNotifyCmd->SetArrayField("keys", keysArray);

	TSharedRef<FJsonObject> getCmd = MakeShared<FJsonObject>();
	getCmd->SetStringField("cmd", "Get");
	getCmd->SetArrayField("keys", keysArray);

	TArray<TSharedRef<FJsonObject>> jsons;

	jsons.Add(setNotifyCmd);
	jsons.Add(getCmd);

	Send(jsons);
}

void AApSubsystem::MonitorDataStoreUnboundedNumberValue(const FString& key, TFunction<void(const FString&, const TSharedRef<FJsonValueNumberString>&, const TSharedRef<FJsonValueNumberString>&, int)> callback) {
	LargeIntegerUpdate rawDataUpdate;
	rawDataUpdate.Callback = callback;

	if (dataStoreReplyCallbacks.Contains(key))
		dataStoreReplyCallbacks[key] = MakeShared<LargeIntegerUpdate>(rawDataUpdate);
	else
		dataStoreReplyCallbacks.Add(key, MakeShared<LargeIntegerUpdate>(rawDataUpdate));

	TArray<TSharedPtr<FJsonValue>> keysArray;
	keysArray.Add(MakeShared<FJsonValueString>(key));

	TSharedRef<FJsonObject> setNotifyCmd = MakeShared<FJsonObject>();
	setNotifyCmd->SetStringField("cmd", "SetNotify");
	setNotifyCmd->SetArrayField("keys", keysArray);

	TArray<TSharedPtr<FJsonValue>> operations;

	TSharedRef<FJsonObject> defaultOperation = MakeShared<FJsonObject>();
	defaultOperation->SetStringField("operation", "default");
	defaultOperation->SetField("value", MakeShared<FJsonValueNumberString>(FString::FromInt(0)));

	operations.Add(MakeShared<FJsonValueObject>(defaultOperation));

	TSharedRef<FJsonObject> setCmd = MakeShared<FJsonObject>();
	setCmd->SetStringField("cmd", "Set");
	setCmd->SetStringField("key", key);
	setCmd->SetField("default", MakeShared<FJsonValueNumberString>(FString::FromInt(0)));
	setCmd->SetArrayField("operations", operations);

	TArray<TSharedRef<FJsonObject>> jsons;

	jsons.Add(setNotifyCmd);
	jsons.Add(setCmd);

	Send(jsons);
}

void AApSubsystem::ModifyDataStorageInt64NoCap(const FString& key, int64 amount) const
{
	Send(BuildNumericSetPacket(key, amount));
}

void AApSubsystem::ModifyDataStorageInt64(const FString& key, int64 amount) const {
	Send(BuildNumericSetPacket(key, amount, UINT64_MAX));
}

void AApSubsystem::ModifyDataStorageInt64(const TMap<FString, int64>& adjustmentsPerKey) const
{
	TArray<TSharedRef<FJsonObject>> setPackets;

	for (const TPair<FString, int64>& adjustment : adjustmentsPerKey)
	{
		if (adjustment.Value != 0)
			setPackets.Add(BuildNumericSetPacket(adjustment.Key, adjustment.Value, UINT64_MAX));
	}

	Send(setPackets);
}

void AApSubsystem::GetDataStorageJsonFields(const TSet<FString>& keys, TFunction<void(const FString&, const TSharedRef<FJsonValue>&)> callback)
{
	TArray<TSharedPtr<FJsonValue>> keysArray;
	for (const FString& key : keys) {
		keysArray.Add(MakeShared<FJsonValueString>(key));

		JsonGet pendingData;
		pendingData.Callback = callback;

		dataStorageRetrievalCallbacks.Add(key, MakeShared<JsonGet>(pendingData));
	}

	TSharedRef<FJsonObject> cmd = MakeShared<FJsonObject>();
	cmd->SetStringField("cmd", "Get");
	cmd->SetArrayField("keys", keysArray);

	Send(cmd);
}

void AApSubsystem::GetDataStorageInt64Fields(const TSet<FString>& keys, TFunction<void(const FString&, const uint64*)> callback)
{
	TArray<TSharedPtr<FJsonValue>> keysArray;
	for (const FString& key : keys) {
		keysArray.Add(MakeShared<FJsonValueString>(key));

		UInt64Get pendingData;
		pendingData.Callback = callback;

		dataStorageRetrievalCallbacks.Add(key, MakeShared<UInt64Get>(pendingData));
	}

	TSharedRef<FJsonObject> cmd = MakeShared<FJsonObject>();
	cmd->SetStringField("cmd", "Get");
	cmd->SetArrayField("keys", keysArray);

	Send(cmd);
}

void AApSubsystem::SetDataStorageJsonField(const FString& key, const TSharedRef<FJsonObject>& json) const
{
	TArray<TSharedPtr<FJsonValue>> operations;

	TSharedRef<FJsonObject> replaceOperation = MakeShared<FJsonObject>();
	replaceOperation->SetStringField("operation", "replace");
	replaceOperation->SetObjectField("value", json);

	operations.Add(MakeShared<FJsonValueObject>(replaceOperation));

	TSharedRef<FJsonObject> setCmd = MakeShared<FJsonObject>();
	setCmd->SetStringField("cmd", "Set");
	setCmd->SetStringField("key", key);
	setCmd->SetObjectField("default", MakeShared<FJsonObject>());
	setCmd->SetArrayField("operations", operations);

	Send(setCmd);
}

void AApSubsystem::SetItemReceivedCallback(TFunction<void(int64, bool)> onItemReceived) {
	itemReceivedCallbacks.Add(onItemReceived);
}

void AApSubsystem::SetLocationCheckedCallback(TFunction<void(int64)> onLocationChecked) {
	locationCheckedCallbacks.Add(onLocationChecked);
}

void AApSubsystem::SetDeathLinkReceivedCallback(TFunction<void(FString, FString)> onDeathLinkReceived) {
	deathLinkReceivedCallbacks.Add(onDeathLinkReceived);
}

void AApSubsystem::SetReconnectCallback(TFunction<void(void)> onReconnect) {
	onReconnectCallbacks.Add(onReconnect);
}

void AApSubsystem::ProcessReceivedItems() {
	if (isReconnect) {
		isReconnect = false;
		for (TFunction<void(void)> callback : onReconnectCallbacks)
			callback();
	}

	TTuple<int64, bool> item;
	while (ReceivedItems.Dequeue(item)) {
		for (TFunction<void(int64, bool)> callback : itemReceivedCallbacks)
			callback(item.Key, item.Value);
	}
}

void AApSubsystem::ProcessCheckedLocations() {
	int64 locationId;
	while (CheckedLocations.Dequeue(locationId)) {
		for (TFunction<void(int64)> callback : locationCheckedCallbacks)
			callback(locationId);
	}
}

void AApSubsystem::ProcessDeadlinks() {
	TPair<FString, FString> deadlinkMessage;
	while (PendingDeathlinks.Dequeue(deadlinkMessage)) {
		for (TFunction<void(FString, FString)> callback : deathLinkReceivedCallbacks)
			callback(deadlinkMessage.Key, deadlinkMessage.Value);
	}
}

void AApSubsystem::EnableDeathLink() const {
	CallOnGameThread<void>([]() { AP_EnabledDeathlinkAnyway(); });
}

void AApSubsystem::TriggerDeathLink(FString source, FString cause) {
	AP_Bounce bounce;
	std::string tag = "DeathLink";
	std::vector<std::string> tagsArray = { tag };

	FGuid reference = FGuid::NewGuid();
	FString referenceString = reference.ToString();

	sendDeathLinkReferences.Add(reference);

	FBounceDayo deathLinkData;
	deathLinkData.Source = source;
	deathLinkData.Cause = cause;
	deathLinkData.Time = FDateTime::Now().ToUnixTimestampDecimal();
	deathLinkData.Reference = referenceString;

	FString jsonString;
	FJsonObjectConverter::UStructToJsonObjectString(deathLinkData, jsonString);

	bounce.tags = &tagsArray;
	bounce.data = TCHAR_TO_UTF8(*jsonString);

	CallOnGameThread<void>([bounce]() { AP_SendBounce(bounce); });
}

void AApSubsystem::Send(const TSharedRef<FJsonObject>& json) const
{
	Send(TArray<TSharedRef<FJsonObject>>{ json });
}

void AApSubsystem::Send(const TArray<TSharedRef<FJsonObject>>& jsons) const
{
	TArray<TSharedPtr<FJsonValue>> jsonValues;
	for (const TSharedRef<FJsonObject>& json : jsons) {
		jsonValues.Add(MakeShared<FJsonValueObject>(json));
	}

	FString OutputString;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutputString);

	FJsonSerializer::Serialize(jsonValues, Writer);

	std::string outputJson = TCHAR_TO_UTF8(*OutputString);

	CallOnGameThread<void>([outputJson]() { AP_Send(outputJson); });
}

void AApSubsystem::CheckConnectionState() const {
	if (!IsInGameThread())
		return;

	if (connectionInfoSubsystem->ConnectionState == EApConnectionState::Connecting) {
		AP_ConnectionStatus status = AP_GetConnectionStatus();

		if (status == AP_ConnectionStatus::Authenticated) {
			AP_RoomInfo roomInfo;
			AP_GetRoomInfo(&roomInfo);

			FString seedName = UApUtils::FStr(roomInfo.seed_name);
			if (!connectionInfoSubsystem->roomSeed.IsEmpty() && connectionInfoSubsystem->roomSeed != seedName) {
				AP_Shutdown();

				connectionInfoSubsystem->ConnectionState = EApConnectionState::ConnectionFailed;
				connectionInfoSubsystem->ConnectionStateDescription = LOCTEXT("SeedMismatch", "Room seed does not match save's seed - this save does not belong to the multiworld you're connecting to. Ensure you're loading the right save file and check your connection details.");
			}
			else {
				connectionInfoSubsystem->roomSeed = seedName;
				connectionInfoSubsystem->currentPlayerTeam = AP_GetCurrentPlayerTeam();
				connectionInfoSubsystem->currentPlayerSlot = AP_GetPlayerID();
				connectionInfoSubsystem->ConnectionState = EApConnectionState::Connected;

				AApSlotDataSubsystem* slotDataSubsystem = AApSlotDataSubsystem::Get(GetWorld());
				fgcheck(slotDataSubsystem);

				//TODO: i dont like this dependency graph where ApSubsystem depends on ApSlotDataSubsystem which depends on ApSubsystem
				if (!slotDataSubsystem->HasLoadedSlotData()) {
					AP_Shutdown();

					connectionInfoSubsystem->ConnectionState = EApConnectionState::ConnectionFailed;

					if (slotDataSubsystem->GetSlotDataState() == EApSlotDataState::Failed) {
						connectionInfoSubsystem->ConnectionStateDescription = slotDataSubsystem->GetLastError();
					}

					UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::CheckConnectionState(), Failed to load slotdata"));
				}
				else {
					connectionInfoSubsystem->ConnectionStateDescription = LOCTEXT("AuthSuccess", "Authentication succeeded.");
					UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::Tick(), Successfully Authenticated"));
				}
			}
		}
		else if (status == AP_ConnectionStatus::ConnectionRefused) {
			AP_Shutdown();

			connectionInfoSubsystem->ConnectionState = EApConnectionState::ConnectionFailed;
			connectionInfoSubsystem->ConnectionStateDescription = LOCTEXT("ConnectionRefused", "Connection refused by server. Check your connection details and load the save again.");
			UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::CheckConnectionState(), ConnectionRefused"));
		}
	}
}

void AApSubsystem::ProcessApPackages()
{
	for (int i = 0; i < 5; i++) { // small batches to not lock up the game thread for too long if there are many packages, the rest will be processed in the next ticks
		FString queuedMessage;

		if (JsonPackageQueue.Dequeue(queuedMessage)) {
			const TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*queuedMessage);

			TSharedPtr<FJsonObject> parsedJson;
			FJsonSerializer::Deserialize(reader, parsedJson);

			if (!parsedJson || !parsedJson.IsValid())
				UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::ProcessApPackages(), Failed to parse \"{0}\"", queuedMessage);

			FString cmd = parsedJson->GetStringField(TEXT("cmd"));
			if (cmd == TEXT("Retrieved")) {
				OnRetrievedPackage(parsedJson);
			}
			else if (cmd == TEXT("SetReply")) {
				OnSetReply(parsedJson);
			}
		}
	}

	for (const FString& processedKey : processedCallbacks)
		dataStorageRetrievalCallbacks.Remove(processedKey);
}

void AApSubsystem::OnRetrievedPackage(const TSharedPtr<FJsonObject> json)
{
	const TSharedPtr<FJsonObject>* keys;
	if (!json->TryGetObjectField(TEXT("keys"), keys))
	{
		UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::OnRetrievedPackage(): Protocol violation, Retrieved does not contain 'keys' field");
		return;
	}

	const TMap<FString, TSharedPtr<FJsonValue>>& values = (*keys)->Values;

	for (const TPair<FString, TSharedPtr<FJsonValue>>& pair : values)
	{
		FString key = pair.Key;
		TSharedPtr<FJsonValue> value = pair.Value;

		CallDataStorageCallbackForRetrieved(pair.Key, value);
	}
}

void AApSubsystem::OnSetReply(const TSharedPtr<FJsonObject> json)
{
	FString key;
	if (!json->TryGetStringField(TEXT("key"), key))
	{
		UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::OnSetReply(): Protocol violation, SetReply does not contain 'key' field");
		return;
	}

	int slot;
	if (!json->TryGetNumberField(TEXT("slot"), slot))
	{
		UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::OnSetReply(): Protocol violation, SetReply does not contain 'slot' field");
		return;
	}

	TSharedPtr<FJsonValue> value = json->TryGetField(TEXT("value"));
	if (!value || !value.IsValid())
	{
		UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::OnSetReply(): Protocol violation, SetReply does not contain 'value' field");
		return;
	}

	TSharedPtr<FJsonValue> originalValue = json->TryGetField(TEXT("original_value"));
	if (!originalValue || !originalValue.IsValid())
	{
		UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::OnSetReply(): Protocol violation, SetReply does not contain 'value' field");
		return;
	}

	CallDataStorageCallbackForSetReply(key, originalValue, value, slot);
}

void AApSubsystem::CallDataStorageCallbackForRetrieved(FString key, const TSharedPtr<FJsonValue> json)
{
	if (!dataStorageRetrievalCallbacks.Contains(key))
	{
		UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallback({0}): No callback waiting for key", key);
		return;
	}

	const TSharedRef<FPendingServerDataBase>& pendingData = dataStorageRetrievalCallbacks[key];

	if (pendingData->Type == EDataType::Number)
	{
		const TSharedRef<UInt64Get> pendingDataInt64 = StaticCastSharedRef<UInt64Get>(pendingData);
		if (json->IsNull())
		{
			pendingDataInt64->Callback(key, nullptr);
		}
		else
		{
			uint64 value;
			FString valueString = json->AsString();

			if (!valueString.IsNumeric())
			{
				UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallback({0}): Invalid number string in JSON", key);
			}
			else
			{
				LexFromString(value, *valueString);

				pendingDataInt64->Callback(key, &value);
			}
		}
	}
	else if (pendingData->Type == EDataType::Object)
	{
		const TSharedRef<JsonGet> pendingDataJson = StaticCastSharedRef<JsonGet>(pendingData);
		if (json->IsNull())
		{
			pendingDataJson->Callback(key, MakeShared<FJsonValueNull>());
		}
		else
		{
			const TSharedPtr<FJsonObject>* value;
			if (!json->TryGetObject(value) || !value)
			{
				UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallback({0}): Failed to get object from JSON", key);
			}
			else
			{
				pendingDataJson->Callback(key, MakeShared<FJsonValueObject>(*value));
			}
		}
	}
	else
	{
		UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallback({0}): No handling implemented for the supplied type");
	}

	processedCallbacks.Add(key);
}

void AApSubsystem::CallDataStorageCallbackForSetReply(FString key, const TSharedPtr<FJsonValue>& oldJson, const TSharedPtr<FJsonValue>& json, int slot)
{
	if (!dataStoreReplyCallbacks.Contains(key))
	{
		UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallbackForSetReply({0}): No callback waiting for key", key);
		return;
	}

	const TSharedRef<FUpdatedServerDataBase>& pendingData = dataStoreReplyCallbacks[key];
	if (pendingData->Type == EDataType::Number)
	{
		const TSharedRef<UInt64Update> pendingDataInt64 = StaticCastSharedRef<UInt64Update>(pendingData);
		if (oldJson->IsNull())
		{
			if (json->IsNull())
				pendingDataInt64->Callback(key, nullptr, nullptr, slot);
			else
			{
				uint64 value;
				if (!json->TryGetNumber(value))
				{
					UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallback({0}): Failed to get number from JSON", key);
					return;
				}
				else
				{
					FString valueString = json->AsString();
					LexFromString(value, *valueString);

					pendingDataInt64->Callback(key, nullptr, &value, slot);
				}
			}
		}
		else
		{
			uint64 oldValue;
			if (!oldJson->TryGetNumber(oldValue))
			{
				UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallback({0}): Failed to get old number from JSON", key);
				return;
			}

			FString oldValueString = oldJson->AsString();
			LexFromString(oldValue, *oldValueString);

			if (json->IsNull())
			{
				pendingDataInt64->Callback(key, &oldValue, nullptr, slot);
			}
			else
			{
				uint64 value;
				if (!json->TryGetNumber(value))
				{
					UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallback({0}): Failed to get number from JSON", key);
					return;
				}
				else
				{
					FString valueString = json->AsString();
					LexFromString(value, *valueString);

					pendingDataInt64->Callback(key, &oldValue, &value, slot);
				}
			}
		}
	}
	else if (pendingData->Type == EDataType::Object)
	{
		const TSharedRef<JsonUpdate> pendingDataJson = StaticCastSharedRef<JsonUpdate>(pendingData);
		if (oldJson->IsNull())
		{
			if (json->IsNull())
			{
				pendingDataJson->Callback(key, MakeShared<FJsonValueNull>(), MakeShared<FJsonValueNull>(), slot);
			}
			else
			{
				const TSharedPtr<FJsonObject>* value;
				if (!json->TryGetObject(value) || !value)
				{
					UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallback({0}): Failed to get number from JSON", key);
					return;
				}
				else
				{
					pendingDataJson->Callback(key, MakeShared<FJsonValueNull>(), MakeShared<FJsonValueObject>(*value), slot);
				}
			}
		}
		else
		{
			const TSharedPtr<FJsonObject>* oldValue;
			if (!json->TryGetObject(oldValue) || !oldValue)
			{
				UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallback({0}): Failed to get old number from JSON", key);
				return;
			}

			if (json->IsNull())
			{
				pendingDataJson->Callback(key, MakeShared<FJsonValueObject>(*oldValue), MakeShared<FJsonValueNull>(), slot);
			}
			else
			{
				const TSharedPtr<FJsonObject>* value;
				if (!json->TryGetObject(value) || !value)
				{
					UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallback({0}): Failed to get number from JSON", key);
					return;
				}
				else
				{
					pendingDataJson->Callback(key, MakeShared<FJsonValueObject>(*oldValue), MakeShared<FJsonValueObject>(*value), slot);
				}
			}
		}
	}
	else if (pendingData->Type == EDataType::LargeInteger)
	{
		const TSharedRef<LargeIntegerUpdate> pendingDataJson = StaticCastSharedRef<LargeIntegerUpdate>(pendingData);
		if (oldJson->IsNull())
		{
			if (json->IsNull())
			{
				pendingDataJson->Callback(key, MakeShared<FJsonValueNumberString>(FString::FromInt(0)), MakeShared<FJsonValueNumberString>(FString::FromInt(0)), slot);
			}
			else
			{
				FString valueString = json->AsString();
				if (!valueString.IsNumeric())
				{
					UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallback({0}): Invalid number string in JSON", key);
					return;
				}

				const TSharedRef<FJsonValueNumberString> value = MakeShared<FJsonValueNumberString>(valueString);

				pendingDataJson->Callback(key, MakeShared<FJsonValueNumberString>(FString::FromInt(0)), value, slot);
			}
		}
		else
		{
			FString oldValueString = oldJson->AsString();
			if (!oldValueString.IsNumeric())
			{
				UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallback({0}): Invalid number string in JSON", key);
				return;
			}

			const TSharedRef<FJsonValueNumberString> oldValue = MakeShared<FJsonValueNumberString>(oldValueString);

			if (json->IsNull())
			{
				pendingDataJson->Callback(key, oldValue, MakeShared<FJsonValueNumberString>(FString::FromInt(0)), slot);
			}
			else
			{
				FString valueString = json->AsString();
				if (!valueString.IsNumeric())
				{
					UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallback({0}): Invalid number string in JSON", key);
					return;
				}

				const TSharedRef<FJsonValueNumberString> value = MakeShared<FJsonValueNumberString>(valueString);

				pendingDataJson->Callback(key, oldValue, value, slot);
			}
		}
	}
	else
	{
		UE_LOGFMT(LogApSubsystem, Error, "AApSubsystem::CallDataStorageCallback({0}): No handling implemented for the supplied type");
	}
}

void AApSubsystem::AddChatMessage(FText message, FLinearColor color) {
	ChatMessageQueue.Enqueue(TPair<FString, FLinearColor>(message.ToString(), color));
}

void AApSubsystem::HandleAPMessages() {
	for (int i = 0; i < 10; i++) {
		TPair<FString, FLinearColor> queuedMessage;
		if (ChatMessageQueue.Dequeue(queuedMessage)) {
			SendChatMessage(queuedMessage.Key, queuedMessage.Value);
		}
		else {
			CallOnGameThread<void>([this]() {
				if (!AP_IsMessagePending())
					return;

				AP_Message* message = AP_GetLatestMessage();
				SendChatMessage(UApUtils::FStr(message->text), FLinearColor::White);

				AP_ClearLatestMessage();
				});
		}
	}
}

void AApSubsystem::SendChatMessage(const FString& Message, const FLinearColor& Color) const {
	UE_LOG(LogApSubsystem, Display, TEXT("Archipelago Cpp Chat Message: %s"), *Message);
	AApMessagingSubsystem* messaging = AApMessagingSubsystem::Get(GetWorld());
	fgcheck(messaging);
	messaging->DisplayMessage(Message, Color);
}

void AApSubsystem::TimeoutConnection() const {
	AP_Shutdown();

	connectionInfoSubsystem->ConnectionState = EApConnectionState::ConnectionFailed;
	connectionInfoSubsystem->ConnectionStateDescription = LOCTEXT("AuthFailed", "Authentication failed. Check your connection details and load the save again.");
	UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::TimeoutConnectionIfNotConnected(), Authenticated Failed"));
}

void AApSubsystem::PackageReceivedCallback(const std::string rawJson)
{
	FString json = UApUtils::FStr(rawJson);

	UE_LOGFMT(LogApSubsystem, Display, "AApSubsystem::PackageReceivedCallback({0})", json);

	JsonPackageQueue.Enqueue(json);
}

FString AApSubsystem::GetApItemName(int64 id) const {
	return UApUtils::FStr(CallOnGameThread<std::string>([id]() { return AP_GetItemName("Satisfactory", id); }));
}

void AApSubsystem::SetVaultState(const TMap<FString, TMap<FString, float>>& vaultItemTraitMapping) const
{
	TSharedRef<FJsonObject> traitsJsonObject = MakeShareable(new FJsonObject());

	for (const TPair<FString, TMap<FString, float>>& traitsPerItem : vaultItemTraitMapping)
	{
		TSharedRef<FJsonObject> traitJsonObject = MakeShareable(new FJsonObject());

		for (const TPair<FString, float>& traitValues : traitsPerItem.Value)
		{
			const float Rounded = FMath::RoundToFloat(traitValues.Value * 1000.0f) / 1000.0f;  //round to 3 decimals
			traitJsonObject->SetField(traitValues.Key, MakeShared<FJsonValueNumberString>(FString::SanitizeFloat(Rounded)));
		}

		traitsJsonObject->SetObjectField(traitsPerItem.Key, traitJsonObject);
	}

	 SetDataStorageJsonField("VSatisfactory", traitsJsonObject);
}

void AApSubsystem::SetGiftBoxState(bool open, const TSet<FString>& acceptedTraits) const {
	TArray<TSharedPtr<FJsonValue>> traits;

	for (const FString& trait : acceptedTraits) {
		traits.Add(MakeShared<FJsonValueString>(trait));
	}

	TSharedRef<FJsonObject> giftBoxObject = MakeShared<FJsonObject>();
	giftBoxObject->SetField("minimum_gift_data_version", MakeShared<FJsonValueNumberString>(FString::FromInt(3)));
	giftBoxObject->SetField("maximum_gift_data_version", MakeShared<FJsonValueNumberString>(FString::FromInt(3)));
	giftBoxObject->SetNumberField("maximum_gift_data_version", 3);
	giftBoxObject->SetBoolField("is_open", open);
	giftBoxObject->SetBoolField("accepts_any_gift", false);
	giftBoxObject->SetArrayField("desired_traits", traits);

	TSharedRef<FJsonObject> giftWrapper = MakeShared<FJsonObject>();
	giftWrapper->SetObjectField(FString::FromInt(connectionInfoSubsystem->GetCurrentPlayerSlot()), giftBoxObject);

	TArray<TSharedPtr<FJsonValue>> operations;

	TSharedRef<FJsonObject> updateOperation = MakeShared<FJsonObject>();
	updateOperation->SetStringField("operation", "update");
	updateOperation->SetObjectField("value", giftWrapper);

	operations.Add(MakeShared<FJsonValueObject>(updateOperation));

	FString key = FString::Format(TEXT("GiftBox;{0}"), { connectionInfoSubsystem->GetCurrentPlayerTeam() });

	TSharedRef<FJsonObject> setCmd = MakeShared<FJsonObject>();
	setCmd->SetStringField("cmd", "Set");
	setCmd->SetStringField("key", key);
	setCmd->SetObjectField("default", MakeShared<FJsonObject>());
	setCmd->SetArrayField("operations", operations);

	Send(setCmd);
}

void AApSubsystem::SendGift(const FApGift& giftToSend) const {
	Send(BuildSendGift(giftToSend));
}

TSharedRef<FJsonObject> AApSubsystem::BuildSendGift(const FApGift& giftToSend) const
{
	static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

	TArray<TSharedPtr<FJsonValue>> traits;

	for (const FApGiftTrait& trait : giftToSend.Traits) {
		TSharedRef<FJsonObject> traitObject = MakeShared<FJsonObject>();
		traitObject->SetStringField("trait", giftTraitEnum->GetNameStringByValue(static_cast<int64>(trait.Trait)));
		if (!FMath::IsNearlyEqual(trait.Duration, 1.0f))
			traitObject->SetNumberField("duration", trait.Duration);
		if (!FMath::IsNearlyEqual(trait.Quality, 1.0f))
			traitObject->SetNumberField("quality", trait.Quality);

		traits.Add(MakeShared<FJsonValueObject>(traitObject));
	}

	TSharedRef<FJsonObject> giftObject = MakeShared<FJsonObject>();
	giftObject->SetStringField("id", giftToSend.Id);
	giftObject->SetStringField("item_name", giftToSend.ItemName);
	giftObject->SetField("amount", MakeShared<FJsonValueNumberString>(FString::FromInt(giftToSend.Amount)));
	if (giftToSend.ItemValue > 0)
		giftObject->SetField("item_value", MakeShared<FJsonValueNumberString>(FString::FromInt(giftToSend.ItemValue)));
	giftObject->SetField("sender_team", MakeShared<FJsonValueNumberString>(FString::FromInt(connectionInfoSubsystem->GetCurrentPlayerTeam())));
	giftObject->SetField("sender_slot", MakeShared<FJsonValueNumberString>(FString::FromInt(connectionInfoSubsystem->GetCurrentPlayerSlot())));
	giftObject->SetField("receiver_team", MakeShared<FJsonValueNumberString>(FString::FromInt(giftToSend.Receiver.Team)));
	giftObject->SetField("receiver_slot", MakeShared<FJsonValueNumberString>(FString::FromInt(giftToSend.Receiver.Slot)));
	giftObject->SetField("is_refund", MakeShared<FJsonValueBoolean>(giftToSend.isRefund));
	giftObject->SetArrayField("traits", traits);

	TSharedRef<FJsonObject> giftWrapper = MakeShared<FJsonObject>();
	giftWrapper->SetObjectField(giftToSend.Id, giftObject);

	TArray<TSharedPtr<FJsonValue>> operations;

	TSharedRef<FJsonObject> updateOperation = MakeShared<FJsonObject>();
	updateOperation->SetStringField("operation", "update");
	updateOperation->SetObjectField("value", giftWrapper);

	operations.Add(MakeShared<FJsonValueObject>(updateOperation));

	FString key;
	if (giftToSend.isRefund)
		key = FString::Format(TEXT("GiftBox;{0};{1}"), { giftToSend.Sender.Team, giftToSend.Sender.Slot });
	else
		key = FString::Format(TEXT("GiftBox;{0};{1}"), { giftToSend.Receiver.Team, giftToSend.Receiver.Slot });

	TSharedRef<FJsonObject> setCmd = MakeShared<FJsonObject>();
	setCmd->SetStringField("cmd", "Set");
	setCmd->SetStringField("key", key);
	setCmd->SetField("default", MakeShared<FJsonValueNumberString>(FString::FromInt(0)));
	setCmd->SetArrayField("operations", operations);

	return setCmd;
}

TSharedRef<FJsonObject> AApSubsystem::BuildNumericSetPacket(const FString& key, int64 amount, uint64 kappa)
{
	TArray<TSharedPtr<FJsonValue>> operations;

	TSharedRef<FJsonObject> addOperation = MakeShared<FJsonObject>();
	addOperation->SetStringField("operation", "add");
	addOperation->SetField("value", MakeShared<FJsonValueNumberString>(LexToString(amount)));

	operations.Add(MakeShared<FJsonValueObject>(addOperation));

	if (amount < 0) {
		TSharedRef<FJsonObject> maxOperation = MakeShared<FJsonObject>();
		maxOperation->SetStringField("operation", "max");
		maxOperation->SetField("value", MakeShared<FJsonValueNumberString>(FString::FromInt(0)));

		operations.Add(MakeShared<FJsonValueObject>(maxOperation));
	}
	else if (kappa != 0) {
		TSharedRef<FJsonObject> minOperation = MakeShared<FJsonObject>();
		minOperation->SetStringField("operation", "min");
		minOperation->SetField("value", MakeShared<FJsonValueNumberString>(LexToString(kappa))); //should be UINT64 Max right?

		operations.Add(MakeShared<FJsonValueObject>(minOperation));
	}

	TSharedRef<FJsonObject> setCmd = MakeShared<FJsonObject>();
	setCmd->SetStringField("cmd", "Set");
	setCmd->SetStringField("key", key);
	setCmd->SetField("default", MakeShared<FJsonValueNumberString>(FString::FromInt(0)));
	setCmd->SetArrayField("operations", operations);

	return setCmd;
}

void AApSubsystem::ProcessGifts(const TSet<FString>& acceptedIds, const TArray<FApGift>& rejectedGifts) const
{
	TSet<FString> acceptedIdsCopy = acceptedIds;

	if (acceptedIds.Num() == 0 && rejectedGifts.Num() == 0)
		return;

	TArray<TSharedRef<FJsonObject>> packets;

	for (FApGift giftToReject : rejectedGifts)
	{
		giftToReject.isRefund = true;

		acceptedIdsCopy.Add(giftToReject.Id);

		packets.Add(BuildSendGift(giftToReject));
	}

	TArray<TSharedPtr<FJsonValue>> operations;

	for (FString id : acceptedIdsCopy) {
		TSharedRef<FJsonObject> popOperation = MakeShared<FJsonObject>();
		popOperation->SetStringField("operation", "pop");
		popOperation->SetStringField("value", id);

		operations.Add(MakeShared<FJsonValueObject>(popOperation));
	}

	FString key = FString::Format(TEXT("GiftBox;{0};{1}"), { connectionInfoSubsystem->GetCurrentPlayerTeam(), connectionInfoSubsystem->GetCurrentPlayerSlot() });

	TSharedRef<FJsonObject> setCmd = MakeShared<FJsonObject>();
	setCmd->SetStringField("cmd", "Set");
	setCmd->SetStringField("key", key);
	setCmd->SetObjectField("default", MakeShared<FJsonObject>());
	setCmd->SetArrayField("operations", operations);

	packets.Add(setCmd);

	Send(packets);
}

TMap<FApPlayer, TTuple<FString, FString>> AApSubsystem::GetAllApPlayers() const {
	std::vector<AP_NetworkPlayer> apPlayers =
		CallOnGameThread<std::vector<AP_NetworkPlayer>>([]() { return AP_GetAllPlayers(); });

	TMap<FApPlayer, TTuple<FString, FString>> players;

	for (const AP_NetworkPlayer& apPlayer : apPlayers) {
		FApPlayer player;
		player.Team = apPlayer.team;
		player.Slot = apPlayer.slot;

		TTuple<FString, FString> nameAndGame(UApUtils::FStr(apPlayer.alias), UApUtils::FStr(apPlayer.game));

		players.Add(player, nameAndGame);
	}

	return players;
}

TSet<int64> AApSubsystem::GetAllLocations() {
	TSet<int64> locations;

	for (const int64 locationId : AP_GetAllLocations()) {
		locations.Add(locationId);
	}

	return locations;
}

void AApSubsystem::MarkGameAsDone() const {
	if (config.SaveMode) {
		UE_LOG(LogApSubsystem, Warning, TEXT("AApSubsystem::MarkGameAsDone() called in Save Mode, ignoring request"));
		return;
	}

	CallOnGameThread<void>([]() { AP_StoryComplete(); });
}

void AApSubsystem::Say(FString message) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::Say(%s)"), *message);

	AP_Say(TCHAR_TO_UTF8(*message));
}

FApNetworkItem AApSubsystem::ScoutLocation(int64 locationId) {
	TSet<int64> locationIds{ locationId };

	TMap<int64, FApNetworkItem> results = ScoutLocation(locationIds);

	if (results.Contains(locationId))
		return results[locationId];

	return FApNetworkItem();
}

TMap<int64, FApNetworkItem> AApSubsystem::ScoutLocation(const TSet<int64>& locationIds) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ScoutLocation(set: %i)"), locationIds.Num());

	std::set<int64_t> locationsToScout;

	for (int64 locationId : locationIds) {
		locationsToScout.insert(locationId);
	}

	//TOOD if we send in unknown location to the server, this promose is never fufilled
	// we likely need to add an async task with a scope lock on a boolean that can set the promise if the server doesnt after a certain time
	// currently causes the game to hang indefently
	locationScoutingPromise = MakeShared<TPromise<TMap<int64, FApNetworkItem>>>();

	CallOnGameThread<void>([this, &locationsToScout]() {
		AP_SendLocationScouts(locationsToScout, 0);
		});

	TMap<int64, FApNetworkItem> result = locationScoutingPromise->GetFuture().Get();

	locationScoutingPromise = nullptr;

	return result;
}

void AApSubsystem::CreateLocationHint(int64 locationId, bool spam) const
{
	TSet<int64> locationIds{ locationId };

	CreateLocationHint(locationIds, spam);
}

void AApSubsystem::CreateLocationHint(const TSet<int64>& locationIds, bool spam) const {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::CreateLocationHint(set: %i, %s)"), locationIds.Num(), spam ? TEXT("true") : TEXT("false"));

	if (config.SaveMode) {
		UE_LOG(LogApSubsystem, Warning, TEXT("AApSubsystem::CreateLocationHint() called in Save Mode, ignoring request"));
		return;
	}

	std::set<int64_t> locationsToHint;

	for (int64 locationId : locationIds) {
		locationsToHint.insert(locationId);
	}

	CallOnGameThread<void>([&locationsToHint, spam]() {
		AP_SendLocationScouts(locationsToHint, spam ? 1 : 2);
		});
}

void AApSubsystem::CheckLocation(int64 locationId) const {
	TSet<int64> locationIds{ locationId };

	CheckLocation(locationIds);
}

void AApSubsystem::CheckLocation(const TSet<int64>& locationIds) const {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::CheckLocation(set: %i)"), locationIds.Num());

	if (config.SaveMode) {
		UE_LOG(LogApSubsystem, Warning, TEXT("AApSubsystem::CheckLocation() called in Save Mode, ignoring request"));
		return;
	}

	std::set<int64_t> locationsToCheck;

	for (int64 locationId : locationIds) {
		locationsToCheck.insert(locationId);
	}

	CallOnGameThread<void>([&locationsToCheck]() {
		AP_SendItem(locationsToCheck);
		});
}

FString AApSubsystem::GetSlotDataJson() const {
	return CallOnGameThread<FString>([]() {
		std::string slotData = AP_GetSlotData();
		return UApUtils::FStr(slotData);
		});
}

template<typename RetType>
RetType AApSubsystem::CallOnGameThread(TFunction<RetType()> InFunction) const {
	if (IsInGameThread())
		return InFunction();

	TSharedRef<TPromise<RetType>> Promise = MakeShared<TPromise<RetType>>();

	AsyncTask(ENamedThreads::GameThread, [Promise, InFunction]() {
		Promise->SetValue(InFunction());
		});

	return Promise->GetFuture().Get();
}
template<>
void AApSubsystem::CallOnGameThread(TFunction<void()> InFunction) const {
	if (IsInGameThread())
		return InFunction();

	AsyncTask(ENamedThreads::GameThread, [InFunction]() {
		InFunction();
		});
}

#undef LOCTEXT_NAMESPACE
