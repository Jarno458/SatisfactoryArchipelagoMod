#include "Subsystem/ApSubsystem.h"
#include "ApUtils.h"
#include "Async/Async.h"
#include "SessionSettings/SessionSettingsManager.h"
#include "Settings/SMLOptionsLibrary.h"
#include "Logging/StructuredLog.h"
#include "FGGameUserSettings.h"

DEFINE_LOG_CATEGORY(LogApSubsystem);

//TODO REMOVE
#pragma optimize("", off)

#define LOCTEXT_NAMESPACE "Archipelago"

AApSubsystem::AApSubsystem() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.5f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

AApSubsystem* AApSubsystem::Get(class UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApSubsystem* AApSubsystem::Get(class UWorld* world) {
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
	AP_RegisterSetReplyCallback([this](AP_SetReply setPacket) {
		SetReplyCallback(std::move(setPacket));
	});
	AP_SetLocationInfoCallback([this](std::vector<AP_NetworkItem> scoutedItems) {
		LocationScoutedCallback(std::move(scoutedItems));
	});
	AP_SetDeathLinkRecvCallback([this](std::string source, std::string cause) {
		DeathLinkReceivedCallback(source, cause);
	});
	AP_SetLoggingCallback([this](std::string message) {
		UE_LOG(LogApSubsystem, Display, TEXT("LogFromAPCpp: %s"), *UApUtils::FStr(message));
	});
	AP_RegisterSlotDataRawCallback("Data", [this](std::string json) {
		connectionInfoSubsystem->slotDataJson = UApUtils::FStr(json);
	});

	AP_SetDeathLinkSupported(true);
	AP_SetGiftingSupported(true);

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

bool AApSubsystem::InitializeTick(FDateTime connectingStartedTime, int timeout) {
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

	HandleAPMessages();
}

void AApSubsystem::EndPlay(const EEndPlayReason::Type endPlayReason) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::EndPlay(%i)"), endPlayReason);

	Super::EndPlay(endPlayReason);

	CallOnGameThread<void>([]() { AP_Shutdown(); });
}

void AApSubsystem::DeathLinkReceivedCallback(std::string source, std::string cause) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::DeathLinkReceivedCallback()"));

	FText sourceString = UApUtils::FText(source);
	FText causeString = UApUtils::FText(cause);
	FText message;
	if (causeString.IsEmpty())
		message = FText::Format(LOCTEXT("DeathLinkReceived", "{0} has died, and so have you!"), sourceString);
	else
		message = FText::Format(LOCTEXT("DeathLinkReceivedWithCause", "{0} has died because {1}"), sourceString, causeString);

	PendingDeathlinks.Enqueue(message);
}

void AApSubsystem::SetReplyCallback(AP_SetReply setReply) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::SetReplyCallback(\"%s\")"), *UApUtils::FStr(setReply.key));

	FString key = UApUtils::FStr(setReply.key);

	if (dataStoreCallbacks.Contains(key))
		dataStoreCallbacks[key](setReply);
}

void AApSubsystem::LocationScoutedCallback(std::vector<AP_NetworkItem> scoutedLocations) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::LocationScoutedCallback(vector[%i])"), scoutedLocations.size());

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

void AApSubsystem::MonitorDataStoreValue(FString keyFString, TFunction<void()> callback) {
	TFunction<void(AP_SetReply)> callbackWrapper = [callback](AP_SetReply setReply) { callback(); };

	if (dataStoreCallbacks.Contains(keyFString))
		dataStoreCallbacks[keyFString] = callbackWrapper;
	else
		dataStoreCallbacks.Add(keyFString, callbackWrapper);

	callback();
}

void AApSubsystem::MonitorDataStoreValue(FString keyFString, AP_DataType dataType, FString defaultValueFString, TFunction<void(AP_SetReply)> callback) {
	if (dataStoreCallbacks.Contains(keyFString))
		dataStoreCallbacks[keyFString] = callback;
	else
		dataStoreCallbacks.Add(keyFString, callback);

	CallOnGameThread<void>([this, keyFString, dataType, defaultValueFString]() {
		std::string key = TCHAR_TO_UTF8(*keyFString);
		std::string defaultValue = TCHAR_TO_UTF8(*defaultValueFString);

		std::map<std::string, AP_DataType> keylist = { { key, dataType } };

		AP_SetNotify(keylist);

		AP_SetServerDataRequest setDefaultAndRecieceUpdate;
		setDefaultAndRecieceUpdate.key = key;

		AP_DataStorageOperation setDefault;
		setDefault.operation = "default";
		setDefault.value = &defaultValue;

		std::vector<AP_DataStorageOperation> operations;
		operations.push_back(setDefault);

		setDefaultAndRecieceUpdate.operations = operations;
		setDefaultAndRecieceUpdate.default_value = &defaultValue;
		setDefaultAndRecieceUpdate.type = dataType;
		setDefaultAndRecieceUpdate.want_reply = true;

		AP_SetServerData(&setDefaultAndRecieceUpdate);
	});
}

void AApSubsystem::ModdifyEnergyLink(long amount, FString defaultValueFString) {
	CallOnGameThread<void>([this, amount, defaultValueFString]() {
		AP_SetServerDataRequest sendEnergyLinkUpdate;
		sendEnergyLinkUpdate.key = "EnergyLink" + std::to_string(connectionInfoSubsystem->currentPlayerTeam);

		std::string valueToAdd = std::to_string(amount);
		std::string defaultValue = TCHAR_TO_UTF8(*defaultValueFString);

		AP_DataStorageOperation add;
		add.operation = "add";
		add.value = &valueToAdd;

		AP_DataStorageOperation lowerBoundry;
		lowerBoundry.operation = "max";
		lowerBoundry.value = &defaultValue;

		std::vector<AP_DataStorageOperation> operations;
		operations.push_back(add);
		operations.push_back(lowerBoundry);

		sendEnergyLinkUpdate.operations = operations;
		sendEnergyLinkUpdate.default_value = &defaultValue;
		sendEnergyLinkUpdate.type = AP_DataType::Raw;
		sendEnergyLinkUpdate.want_reply = true;

		AP_SetServerData(&sendEnergyLinkUpdate);
	});
}

void AApSubsystem::SetItemReceivedCallback(TFunction<void(int64, bool)> onItemReceived){
	itemReceivedCallbacks.Add(onItemReceived);
}

void AApSubsystem::SetLocationCheckedCallback(TFunction<void(int64)> onLocationChecked) {
	locationCheckedCallbacks.Add(onLocationChecked);
}

void AApSubsystem::SetDeathLinkReceivedCallback(TFunction<void(FText)> onDeathLinkReceived) {
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
	FText deadlinkMessage;
	while (PendingDeathlinks.Dequeue(deadlinkMessage)) {
		for (TFunction<void(FText)> callback : deathLinkReceivedCallbacks)
			callback(deadlinkMessage);
	}
}

void AApSubsystem::TriggerDeathLink() {
	CallOnGameThread<void>([]() { AP_DeathLinkSend(); });
}

void AApSubsystem::CheckConnectionState() {
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
				connectionInfoSubsystem->ConnectionStateDescription = LOCTEXT("SeedMissmatch", "Room seed does not match save's seed - this save does not belong to the multiworld you're connecting to. Ensure you're loading the right save file and check your connection details.");
			} else {
				connectionInfoSubsystem->roomSeed = seedName;
				connectionInfoSubsystem->currentPlayerTeam = AP_GetCurrentPlayerTeam();
				connectionInfoSubsystem->currentPlayerSlot = AP_GetPlayerID();
				connectionInfoSubsystem->ConnectionState = EApConnectionState::Connected;
				connectionInfoSubsystem->ConnectionStateDescription = LOCTEXT("AuthSuccess", "Authentication succeeded.");
				UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::Tick(), Successfully Authenticated"));
			}
		} else if (status == AP_ConnectionStatus::ConnectionRefused) {
			AP_Shutdown();

			connectionInfoSubsystem->ConnectionState = EApConnectionState::ConnectionFailed;
			connectionInfoSubsystem->ConnectionStateDescription = LOCTEXT("ConnectionRefused", "Connection refused by server. Check your connection details and load the save again.");
			UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::CheckConnectionState(), ConnectionRefused"));
		}
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
		} else {
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

void AApSubsystem::SendChatMessage(const FString& Message, const FLinearColor& Color) {
	UE_LOG(LogApSubsystem, Display, TEXT("Archipelago Cpp Chat Message: %s"), *Message);
	AApMessagingSubsystem* messaging = AApMessagingSubsystem::Get(GetWorld());
	fgcheck(messaging);
	messaging->DisplayMessage(Message, Color);
}

void AApSubsystem::TimeoutConnection() {
	AP_Shutdown();

	connectionInfoSubsystem->ConnectionState = EApConnectionState::ConnectionFailed;
	connectionInfoSubsystem->ConnectionStateDescription = LOCTEXT("AuthFailed", "Authentication failed. Check your connection details and load the save again.");
	UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::TimeoutConnectionIfNotConnected(), Authenticated Failed"));
}

FString AApSubsystem::GetApItemName(int64 id) {
	return UApUtils::FStr(CallOnGameThread<std::string>([id]() { return AP_GetItemName("Satisfactory", id); }));
}

void AApSubsystem::SetGiftBoxState(bool open, const TSet<FString>& acceptedTraits) {
	AP_RequestStatus result = CallOnGameThread<AP_RequestStatus>([open, acceptedTraits]() {
		AP_UseGiftAutoReject(true);

		std::vector<std::string> desiredTriats;

		for (const FString trait : acceptedTraits)
			desiredTriats.push_back(TCHAR_TO_UTF8(*trait));

		AP_GiftBoxProperties giftbox;
		giftbox.AcceptsAnyGift = false;
		giftbox.DesiredTraits = desiredTriats;
		giftbox.IsOpen = open;

		 return AP_SetGiftBoxProperties(giftbox);
	});

	if (result != AP_RequestStatus::Done)
		UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::SetGiftBoxState(\"%s\") Updating giftbox metadata failed"), (open ? TEXT("true") : TEXT("false")));
}

TMap<FApPlayer, FApTraitBits> AApSubsystem::GetAcceptedTraitsPerPlayer() {
	std::map<std::pair<int, std::string>, AP_GiftBoxProperties> giftboxes =
		CallOnGameThread<std::map<std::pair<int, std::string>, AP_GiftBoxProperties>>([]() { return AP_QueryGiftBoxes(); });

	static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

	TMap<FApPlayer, FApTraitBits> openGiftBoxes;
	for (std::pair<std::pair<int, std::string>, AP_GiftBoxProperties> giftbox : giftboxes) {
		if (giftbox.second.IsOpen) {
			FApPlayer player;
			player.Team = giftbox.first.first;
			player.Name = UApUtils::FStr(giftbox.first.second);

			TSet<EGiftTrait> acceptedTraits;
			for (std::string trait : giftbox.second.DesiredTraits) {
				const int64 enumValue = giftTraitEnum->GetValueByNameString(UApUtils::FStr(trait));
				if (enumValue != INDEX_NONE) {
					acceptedTraits.Add(static_cast<EGiftTrait>(enumValue));
				}
			}

			FApTraitBits metaData(giftbox.second.AcceptsAnyGift, acceptedTraits);

			openGiftBoxes.Add(player, metaData);
		}
	}

	return openGiftBoxes;
}

bool AApSubsystem::SendGift(FApSendGift giftToSend) {
	static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

	AP_Gift gift;
	gift.ItemName = TCHAR_TO_UTF8(*giftToSend.ItemName);
	gift.Amount = giftToSend.Amount;
	gift.ItemValue = giftToSend.ItemValue;
	gift.Receiver = TCHAR_TO_UTF8(*giftToSend.Receiver.Name);
	gift.ReceiverTeam = giftToSend.Receiver.Team;
	gift.Traits = std::vector<AP_GiftTrait>(giftToSend.Traits.Num());
	gift.IsRefund = false;

	for (int i = 0; i < giftToSend.Traits.Num(); i++)
	{
		AP_GiftTrait trait;
		trait.Trait = TCHAR_TO_UTF8(*giftTraitEnum->GetNameStringByValue((int64)giftToSend.Traits[i].Trait));
		trait.Duration = 1.0;
		trait.Quality = giftToSend.Traits[i].Quality;

		gift.Traits[i] = trait;
	}

	AP_RequestStatus result = CallOnGameThread<AP_RequestStatus>([gift]() { return AP_SendGift(gift); });

	if (result != AP_RequestStatus::Done)
		UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::SendGift({Name: \"%s\", Amount: %i}) Sending gift failed"), *giftToSend.ItemName, giftToSend.Amount);

	return result != AP_RequestStatus::Error;
}

TArray<FApReceiveGift> AApSubsystem::GetGifts() {
	std::vector<AP_Gift> gifts = CallOnGameThread<std::vector<AP_Gift>>([]() { return AP_CheckGifts(); });

	static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

	TArray<FApReceiveGift> currentGifts;
	for (AP_Gift apGift : gifts) {
		FApReceiveGift gift;
		gift.Id = UApUtils::FStr(apGift.ID);
		gift.ItemName = UApUtils::FStr(apGift.ItemName);
		gift.Amount = apGift.Amount;
		gift.ItemValue = apGift.ItemValue;
		gift.Traits;
		
		for (const AP_GiftTrait& apTrait : apGift.Traits) {
			const int64 enumValue = giftTraitEnum->GetValueByNameString(UApUtils::FStr(apTrait.Trait));
			if (enumValue == INDEX_NONE)
				continue;

			FApGiftTrait trait;
			trait.Trait = static_cast<EGiftTrait>(enumValue);
			trait.Duration = apTrait.Duration;
			trait.Quality = apTrait.Quality;

			gift.Traits.Add(trait);
		}

		currentGifts.Add(gift);
	}

	return currentGifts;
}

void AApSubsystem::RejectGift(TSet<FString> ids) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::RejectGift(count: %i)"), ids.Num());
	if (ids.Num() == 0)
		return;

	std::set<std::string> giftIds;
	for (FString id : ids) {
		UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::RejectGift(\"%s\")"), *id);
		giftIds.insert(TCHAR_TO_UTF8(*id));
	}

	AP_RequestStatus result = CallOnGameThread<AP_RequestStatus>([giftIds]() { return AP_RejectGift(giftIds); });

	if (result != AP_RequestStatus::Done)
		UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::RejectGift(count: %i) Rejecting gift failed"), ids.Num());
}

void AApSubsystem::AcceptGift(TSet<FString> ids) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::AcceptGift(count: %i)"), ids.Num());
	if (ids.Num() == 0)
		return;

	std::set<std::string> giftIds;
	for (FString id : ids) {
		UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::AcceptGift(\"%s\")"), *id);
		giftIds.insert(TCHAR_TO_UTF8(*id));
	}

	AP_RequestStatus result = CallOnGameThread<AP_RequestStatus>([giftIds]() { return AP_AcceptGift(giftIds); });

	if (result != AP_RequestStatus::Done)
		UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::AcceptGift(count: %i) Accepting gift failed"), ids.Num());
}

TArray<FApPlayer> AApSubsystem::GetAllApPlayers() {
	std::vector<std::pair<int, std::string>> apPlayers =
		CallOnGameThread<std::vector<std::pair<int, std::string>>>([]() { return AP_GetAllPlayers(); });

	TArray<FApPlayer> players;

	for (std::pair<int, std::string> apPlayer : apPlayers) {
		FApPlayer player;
		player.Team = apPlayer.first;
		player.Name = UApUtils::FStr(apPlayer.second);

		players.Add(player);
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

void AApSubsystem::MarkGameAsDone() {
	CallOnGameThread<void>([]() { AP_StoryComplete(); });
}

void AApSubsystem::Say(FString message) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::Say(%s)"), *message);

	AP_Say(TCHAR_TO_UTF8(*message));
}

FApNetworkItem AApSubsystem::ScoutLocation(int64 locationId) {
	TSet<int64> locationIds { locationId };

	TMap<int64, FApNetworkItem> results = ScoutLocation(locationIds);

	if (results.Contains(locationId))
		return results[locationId];

	return FApNetworkItem();
}

TMap<int64, FApNetworkItem> AApSubsystem::ScoutLocation(const TSet<int64>& locationIds) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ScoutLocation(set: %i)"), locationIds.Num());

	std::set<int64> locationsToScout;

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

void AApSubsystem::CreateLocationHint(int64 locationId, bool spam) {
	TSet<int64> locationIds { locationId };

	CreateLocationHint(locationIds, spam);
}

void AApSubsystem::CreateLocationHint(const TSet<int64>& locationIds, bool spam) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::CreateLocationHint(set: %i, %s)"), locationIds.Num(), spam ? TEXT("true") : TEXT("false"));

	std::set<int64> locationsToHint;

	for (int64 locationId : locationIds) {
		locationsToHint.insert(locationId);
	}

	CallOnGameThread<void>([&locationsToHint, spam]() {
		AP_SendLocationScouts(locationsToHint, spam ? 1 : 2);
	});
}

void AApSubsystem::CheckLocation(int64 locationId) {
	TSet<int64> locationIds { locationId };

	CheckLocation(locationIds);
}

void AApSubsystem::CheckLocation(const TSet<int64>& locationIds) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::CheckLocation(set: %i)"), locationIds.Num());

	std::set<int64> locationsToCheck;

	for (int64 locationId : locationIds) {
		locationsToCheck.insert(locationId);
	}

	CallOnGameThread<void>([&locationsToCheck]() {
		AP_SendItem(locationsToCheck);
	});
}

template<typename RetType>
RetType AApSubsystem::CallOnGameThread(TFunction<RetType()> InFunction) {
	if (IsInGameThread())
		return InFunction();
	
	TSharedRef<TPromise<RetType>> Promise = MakeShared<TPromise<RetType>>();

	AsyncTask(ENamedThreads::GameThread, [Promise, InFunction]() {
			Promise->SetValue(InFunction());
	});

	return Promise->GetFuture().Get();
}
template<>
void AApSubsystem::CallOnGameThread(TFunction<void()> InFunction) {
	if (IsInGameThread())
		return InFunction();

	AsyncTask(ENamedThreads::GameThread, [InFunction]() {
		InFunction();
	});
}

#pragma optimize("", on)

#undef LOCTEXT_NAMESPACE
