#include "Subsystem/ApSubsystem.h"
#include "ApUtils.h"
#include "Async/Async.h"

DEFINE_LOG_CATEGORY(LogApSubsystem);

//TODO REMOVE
#pragma optimize("", off)

#define LOCTEXT_NAMESPACE "Archipelago"

AApSubsystem* AApSubsystem::callbackTarget;

AApSubsystem::AApSubsystem() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.5f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;

	//ConnectionState = EApConnectionState::NotYetAttempted;
	//ConnectionStateDescription = LOCTEXT("NotYetAttempted", "A connection has not yet been attempted. Load a save file to attempt to connect.");
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
	std::string const uri = TCHAR_TO_UTF8(*config.Url);
	std::string const user = TCHAR_TO_UTF8(*config.Login);
	std::string const password = TCHAR_TO_UTF8(*config.Password);

	AP_Init(uri.c_str(), "Satisfactory", user.c_str(), password.c_str());

	callbackTarget = this;

	AP_SetItemClearCallback(AApSubsystem::ItemClearCallback);
	AP_SetItemRecvCallback(AApSubsystem::ItemReceivedCallback);
	AP_SetLocationCheckedCallback(AApSubsystem::LocationCheckedCallback);
	AP_RegisterSetReplyCallback(AApSubsystem::SetReplyCallback);
	AP_SetLocationInfoCallback(AApSubsystem::LocationScoutedCallback);
	AP_SetDeathLinkRecvCallback(AApSubsystem::DeathLinkReceivedCallback);
	AP_SetLoggingCallback(AApSubsystem::LogFromAPCpp);
	AP_RegisterSlotDataRawCallback("Data", AApSubsystem::ParseSlotData);
	AP_SetDeathLinkSupported(true);

	//if (!slotData.hasLoadedSlotData)


	connectionInfoSubsystem = AApConnectionInfoSubsystem::Get(GetWorld());

	connectionInfoSubsystem->ConnectionState = EApConnectionState::Connecting;
	connectionInfoSubsystem->ConnectionStateDescription = LOCTEXT("Connecting", "Connecting...");

	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::Starting AP"));
	AP_Start();
}

void AApSubsystem::BeginPlay() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::BeginPlay()"));

	Super::BeginPlay();

	/*UWorld* world = GetWorld();
	SManager = AFGSchematicManager::Get(world);

	portalSubsystem = AApPortalSubsystem::Get(world);
	mappingSubsystem = AApMappingsSubsystem::Get(world);
	trapSubsystem = AApTrapSubsystem::Get(world);
	phaseManager = AFGGamePhaseManager::Get(world);

	SetMamEnhancerConfigurationHooks();*/
}

void AApSubsystem::DispatchLifecycleEvent(ELifecyclePhase phase) {
	if (!HasAuthority())
		UE_LOG(LogApSubsystem, Fatal, TEXT("AApSubsystem()::DispatchLifecycleEvent() Called without authority"));

	if (config.IsLoaded() && !config.Enabled) {
		UE_LOG(LogApSubsystem, Warning, TEXT("Archipelago manually disabled by user config"));
		return;
	}

	if (phase == ELifecyclePhase::CONSTRUCTION) {
		if (!config.IsLoaded())
			config = FApConfigurationStruct::GetActiveConfig(GetWorld());
	}
	else if (phase == ELifecyclePhase::INITIALIZATION) {
		UE_LOG(LogApSubsystem, Display, TEXT("Initiating Archipelago server connection in background..."));
		bool dummy = CallOnGameThread<bool>([this]() {
			ConnectToArchipelago();
			return true;
		});

		UE_LOG(LogApSubsystem, Display, TEXT("Waiting for AP Server connection to succeed..."));
		FDateTime connectingStartedTime = FDateTime::Now();

		FGenericPlatformProcess::ConditionalSleep([this, connectingStartedTime]() {
			return InitializeTick(connectingStartedTime);
		}, 0.5);
	}
	else if (phase == ELifecyclePhase::POST_INITIALIZATION) {
		SetActorTickEnabled(true);
	}
}

bool AApSubsystem::InitializeTick(FDateTime connectingStartedTime) {
	return CallOnGameThread<bool>([this, connectingStartedTime]() {
		if (connectionInfoSubsystem->ConnectionState == EApConnectionState::Connecting) {
			if ((FDateTime::Now() - connectingStartedTime).GetSeconds() > 15)
				TimeoutConnection();
			else
				CheckConnectionState();
		}
		if (connectionInfoSubsystem->ConnectionState == EApConnectionState::Connected) {
			LoadRoomInfo();

			return true;
		}

		return connectionInfoSubsystem->ConnectionState == EApConnectionState::ConnectionFailed;
	});
}

void AApSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (connectionInfoSubsystem->ConnectionState != EApConnectionState::Connected)
		return;

	//if (portalSubsystem->IsInitialized())
	//	ReceiveItems();
	ProcessReceivedItems();
	ProcessCheckedLocations();

	//HandleCheckedLocations();
	HandleAPMessages();
	HandleDeathLink();

	//if (phaseManager->GetGamePhase() > lastGamePhase) {
	//	OnAvaiableSchematicsChanged();
	//	lastGamePhase = phaseManager->GetGamePhase();
	//}
}

void AApSubsystem::EndPlay(const EEndPlayReason::Type endPlayReason) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::EndPlay(%i)"), endPlayReason);

	Super::EndPlay(endPlayReason);

	CallOnGameThread<void>([]() { AP_Shutdown(); });
}

void AApSubsystem::ItemClearCallback() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ItemClearCallback()"));

	//YOLO we dont really need this right?
	//callbackTarget->currentItemIndex = 0;
}

void AApSubsystem::ItemReceivedCallback(int64 item, bool notify, bool isFromServer) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ItemReceivedCallback(%i, \"%s\")"), item, (notify ? TEXT("true") : TEXT("false")));

	TTuple<int64, bool> receivedItem = TTuple<int64, bool>(item, isFromServer);
	callbackTarget->ReceivedItems.Enqueue(receivedItem);
}

void AApSubsystem::LocationCheckedCallback(int64 id) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::LocationCheckedCallback(%i)"), id);

	callbackTarget->CheckedLocations.Enqueue(id);
}

void AApSubsystem::SetReplyCallback(AP_SetReply setReply) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::SetReplyCallback(\"%s\")"), *UApUtils::FStr(setReply.key));

	FString key = UApUtils::FStr(setReply.key);

	if (callbackTarget->dataStoreCallbacks.Contains(key))
		callbackTarget->dataStoreCallbacks[key](setReply);
}

void AApSubsystem::LocationScoutedCallback(std::vector<AP_NetworkItem> scoutedLocations) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::LocationScoutedCallback(vector[%i])"), scoutedLocations.size());

	TMap<int64, const FApNetworkItem> scoutedLocationsResult = TMap<int64, const FApNetworkItem>();

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

	callbackTarget->location_scouting_promise->SetValue(scoutedLocationsResult);
}

void AApSubsystem::ParseSlotData(std::string json) {
	FString jsonString = UApUtils::FStr(json);

	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ParseSlotData(\"%s\")"), *jsonString);

	callbackTarget->connectionInfoSubsystem->slotDataJson = jsonString;

	/*bool success = FApSlotData::ParseSlotData(jsonString, &callbackTarget->slotData);
	if (!success) {
		FText jsonText = FText::FromString(jsonString);
		AbortGame(FText::Format(LOCTEXT("SlotDataInvallid", "Archipelago SlotData Invalid! {0}"), jsonText));
	}*/
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

	callbackTarget->ChatMessageQueue.Enqueue(TPair<FString, FLinearColor>(message.ToString(), FLinearColor::Red));
	callbackTarget->instagib = true;
}

void AApSubsystem::LogFromAPCpp(std::string message) {
	FString mesageToLog = UApUtils::FStr(message);

	UE_LOG(LogApSubsystem, Display, TEXT("LogFromAPCpp: %s"), *mesageToLog);
}

void AApSubsystem::LoadRoomInfo() {
	if (!IsInGameThread())
		return;

	AP_RoomInfo roomInfo;
	AP_GetRoomInfo(&roomInfo);

	FString seedName = UApUtils::FStr(roomInfo.seed_name);
	if (connectionInfoSubsystem->roomSeed.IsEmpty())
		connectionInfoSubsystem->roomSeed = seedName;
	else if (connectionInfoSubsystem->roomSeed != seedName)
		AbortGame(LOCTEXT("RoomSeedMissmatch", "Room seed does not match seed of save, this save does not belong to the multiworld your connecting to"));

	connectionInfoSubsystem->currentPlayerTeam = AP_GetCurrentPlayerTeam();
	connectionInfoSubsystem->currentPlayerSlot = AP_GetPlayerID();
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

void AApSubsystem::HandleDeathLink() {
	if (IsRunningDedicatedServer())
		return; // TODO make deathlink work for dedicated servers

	AFGCharacterPlayer* player = callbackTarget->GetLocalPlayer();
	if (player == nullptr)
		return;

	HandleInstagib(player);

	if (player->IsAliveAndWell()) {
		awaitingHealty = false;
	} else {
		if (!awaitingHealty) {
			awaitingHealty = true;

			CallOnGameThread<void>([]() { AP_DeathLinkSend(); });
		}
	}
}

void AApSubsystem::HandleInstagib(AFGCharacterPlayer* player) {
	if (instagib) {
		instagib = false;

		TSubclassOf<UDamageType> const damageType = TSubclassOf<UDamageType>(UDamageType::StaticClass());
		FDamageEvent instagibDamageEvent = FDamageEvent(damageType);
		player->TakeDamage(1333337, instagibDamageEvent, player->GetFGPlayerController(), player);
	}
}

void AApSubsystem::ProcessReceivedItems() {
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

void AApSubsystem::CheckConnectionState() {
	if (!IsInGameThread())
		return;

	if (connectionInfoSubsystem->ConnectionState == EApConnectionState::Connecting) {
		AP_ConnectionStatus status = AP_GetConnectionStatus();

		if (status == AP_ConnectionStatus::Authenticated) {
			connectionInfoSubsystem->ConnectionState = EApConnectionState::Connected;
			connectionInfoSubsystem->ConnectionStateDescription = LOCTEXT("AuthSuccess", "Authentication succeeded.");
			UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::Tick(), Successfully Authenticated"));
		} else if (status == AP_ConnectionStatus::ConnectionRefused) {
			connectionInfoSubsystem->ConnectionState = EApConnectionState::ConnectionFailed;
			connectionInfoSubsystem->ConnectionStateDescription = LOCTEXT("ConnectionRefused", "Connection refused by server. Check your connection details and load the save again.");
			UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::CheckConnectionState(), ConnectionRefused"));
		}
	}
}

/*void AApSubsystem::ParseScoutedItemsAndCreateRecipiesAndSchematics() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ParseScoutedItemsAndCreateRecipiesAndSchematics()"));

	TMap<FString, TTuple<bool, TSubclassOf<UFGSchematic>>> schematicsPerMilestone = TMap<FString, TTuple<bool, TSubclassOf<UFGSchematic>>>();
	TMap<int64, TSubclassOf<UFGSchematic>> schematicsPerLocation = TMap<int64, TSubclassOf<UFGSchematic>>();

	for (TSubclassOf<UFGSchematic>& schematic : hardcodedSchematics) {
		UFGSchematic* schematicCDO = Cast<UFGSchematic>(schematic->GetDefaultObject());
		if (schematicCDO != nullptr && schematicCDO->mMenuPriority > 1000) {
			schematicsPerLocation.Add(FMath::RoundToInt(schematicCDO->mMenuPriority), schematic);
		}
	}

	for (FApNetworkItem& location : scoutedLocations) {
		if (location.locationName.StartsWith("Hub")) {
			FString milestone = location.locationName.Left(location.locationName.Find(","));
			FString uniqueMilestone = milestone + TEXT("_") + roomSeed;

			if (!schematicsPerMilestone.Contains(milestone)) {
				TTuple<bool, TSubclassOf<UFGSchematic>> schematic = UApUtils::FindOrCreateClass(TEXT("/Archipelago/"), *uniqueMilestone, UFGSchematic::StaticClass());
				schematicsPerMilestone.Add(milestone, schematic);
			}

			if (!locationsPerMilestone.Contains(schematicsPerMilestone[milestone].Value)) {
				locationsPerMilestone.Add(schematicsPerMilestone[milestone].Value, TArray<FApNetworkItem>{ location });
			} else {
				locationsPerMilestone[schematicsPerMilestone[milestone].Value].Add(location);
			}
		} else if (location.location >= 1338500 && location.location <= 1338571 && schematicsPerLocation.Contains(location.location)) {
			TSubclassOf<UFGSchematic> schematic = schematicsPerLocation[location.location];

			locationPerMamNode.Add(schematic, location);
		} else if (location.location >= 1338700 && location.location <= 1338709 && schematicsPerLocation.Contains(location.location)) {
			TSubclassOf<UFGSchematic> schematic = schematicsPerLocation[location.location];

			locationPerShopNode.Add(schematic, location);
		}
	}

	UE_LOG(LogApSubsystem, Display, TEXT("Generating HUB milestones"));

	for (TPair<TSubclassOf<UFGSchematic>, TArray<FApNetworkItem>>& itemPerMilestone : locationsPerMilestone) {
		for (TPair<FString, TTuple<bool, TSubclassOf<UFGSchematic>>>& schematicAndName : schematicsPerMilestone) {
			if (itemPerMilestone.Key == schematicAndName.Value.Value) {
				if (!schematicAndName.Value.Key)
					InitializaHubSchematic(schematicAndName.Key, itemPerMilestone.Key, itemPerMilestone.Value);

				contentRegistry->RegisterSchematic(FName(TEXT("Archipelago")), itemPerMilestone.Key);

				break;
			}
		}
	}

	for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerMamNode : locationPerMamNode) {
		InitializaSchematicForItem(itemPerMamNode.Key, itemPerMamNode.Value, false);
	}

	for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerMamNode : locationPerShopNode) {
		InitializaSchematicForItem(itemPerMamNode.Key, itemPerMamNode.Value, true);
	}

	TArray<TSubclassOf<class UFGResearchTree>> researchTrees;
	RManager->GetAllResearchTrees(researchTrees);

	for (TSubclassOf<UFGResearchTree>& tree : researchTrees) {
		UFGResearchTree* treeCDO = Cast<UFGResearchTree>(tree->GetDefaultObject());
		if (treeCDO != nullptr) {
			FString className = treeCDO->GetName();

			if (!className.Contains("AP_") && !className.EndsWith("HardDrive_C") && !className.EndsWith("XMas_C"))
				contentRegistry->RemoveResearchTree(tree);
		}
	};

	areRecipiesAndSchematicsInitialized = true;
}

*/

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
	AApMessagingSubsystem* messaging = AApMessagingSubsystem::Get(this);
	fgcheck(messaging);
	messaging->DisplayMessage(Message, Color);
}

/*void AApSubsystem::ScoutArchipelagoItems() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ScoutArchipelagoItems()"));

	std::set<int64> locations;

	int maxMilestones = 5;
	int maxSlots = 10;

	int64 hubBaseId = 1338000;

	for (int tier = 1; tier <= slotData.hubLayout.Num(); tier++) {
		for (int milestone = 1; milestone <= maxMilestones; milestone++) {
			for (int slot = 1; slot <= maxSlots; slot++) {
				if (milestone <= slotData.hubLayout[tier - 1].Num() && slot <= slotData.numberOfChecksPerMilestone)
					locations.insert(hubBaseId);

				hubBaseId++;
			}
		}
	}

	//mam locations
	for (int l = 1338500; l <= 1338571; l++)
		locations.insert(l);

	//shop locations
	for (int l = 1338700; l <= 1338709; l++)
		locations.insert(l);

	CallOnGameThread<void>([locations]() {
		UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ScoutArchipelagoItems() [GameThread] scouting %i locations"), locations.size());
		AP_SendLocationScouts(locations, 0); 
	});

	hasScoutedLocations = true;
}*/

void AApSubsystem::TimeoutConnection() {
	connectionInfoSubsystem->ConnectionState = EApConnectionState::ConnectionFailed;
	connectionInfoSubsystem->ConnectionStateDescription = LOCTEXT("AuthFailed", "Authentication failed. Check your connection details and load the save again.");
	UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::TimeoutConnectionIfNotConnected(), Authenticated Failed"));
}

FString AApSubsystem::GetApItemName(int64 id) {
	return UApUtils::FStr(CallOnGameThread<std::string>([id]() { return AP_GetItemName("Satisfactory", id); }));
}

void AApSubsystem::SetGiftBoxState(bool open) {
	AP_RequestStatus result = CallOnGameThread<AP_RequestStatus>([open]() {
		AP_UseGiftAutoReject(false);

		AP_GiftBoxProperties giftbox;
		giftbox.AcceptsAnyGift = true;
		giftbox.DesiredTraits = std::vector<std::string>();
		giftbox.IsOpen = open;

		 return AP_SetGiftBoxProperties(giftbox);
	});

	if (result != AP_RequestStatus::Done)
		UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::SetGiftBoxState(\"%s\") Updating giftbox metadata failed"), (open ? TEXT("true") : TEXT("false")));
}

TMap<FApPlayer, FApGiftBoxMetaData> AApSubsystem::GetAcceptedTraitsPerPlayer() {
	std::map<std::pair<int, std::string>, AP_GiftBoxProperties> giftboxes =
		CallOnGameThread<std::map<std::pair<int, std::string>, AP_GiftBoxProperties>>([]() { return AP_QueryGiftBoxes(); });

	TMap<FApPlayer, FApGiftBoxMetaData> openGiftBoxes;

	for (std::pair<std::pair<int, std::string>, AP_GiftBoxProperties> giftbox : giftboxes) {
		if (giftbox.second.IsOpen) {
			FApPlayer player;
			player.Team = giftbox.first.first;
			player.Name = UApUtils::FStr(giftbox.first.second);

			FApGiftBoxMetaData metaData;
			metaData.AcceptAllTraits = giftbox.second.AcceptsAnyGift;
			metaData.AcceptedTraits = TArray<FString>();

			for (std::string trait : giftbox.second.DesiredTraits)
				metaData.AcceptedTraits.Add(UApUtils::FStr(trait));

			openGiftBoxes.Add(player, metaData);
		}
	}

	return openGiftBoxes;
}

bool AApSubsystem::SendGift(FApSendGift giftToSend) {
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
		trait.Trait = TCHAR_TO_UTF8(*giftToSend.Traits[i].Trait);
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

	TArray<FApReceiveGift> currentGifts;

	for (AP_Gift apGift : gifts) {
		FApReceiveGift gift;
		gift.Id = UApUtils::FStr(apGift.ID);
		gift.ItemName = UApUtils::FStr(apGift.ItemName);
		gift.Amount = apGift.Amount;
		gift.ItemValue = apGift.ItemValue;
		gift.Traits.SetNum(apGift.Traits.size());
		
		for (int i = 0; i < apGift.Traits.size(); i++)
		{
			FApGiftTrait trait;
			trait.Trait = UApUtils::FStr(apGift.Traits[i].Trait);
			trait.Duration = apGift.Traits[i].Duration;
			trait.Quality = apGift.Traits[i].Quality;

			gift.Traits[i] = trait;
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

void AApSubsystem::MarkGameAsDone() {
	CallOnGameThread<void>([]() { AP_StoryComplete(); });
}

void AApSubsystem::Say(FString message) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::Say(%s)"), *message);

	AP_Say(TCHAR_TO_UTF8(*message));
}

const FApNetworkItem AApSubsystem::ScoutLocation(int64 locationId) {
	TSet<int64> locationIds { locationId };

	const TMap<int64, const FApNetworkItem> results = ScoutLocation(locationIds);

	if (results.Contains(locationId))
		return results[locationId];
}

const TMap<int64, const FApNetworkItem> AApSubsystem::ScoutLocation(const TSet<int64>& locationIds) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ScoutLocation(set: %i)"), locationIds.Num());

	std::set<int64> locationsToScout;

	for (int64 locationId : locationIds) {
		locationsToScout.insert(locationId);
	}

	location_scouting_promise = MakeShared<TPromise<const TMap<int64, const FApNetworkItem>>>();

	CallOnGameThread<void>([this, &locationsToScout]() {
		AP_SendLocationScouts(locationsToScout, 0);
	});

	return location_scouting_promise->GetFuture().Get();
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

/*
void AApSubsystem::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::PreSaveGame_Implementation(saveVersion: %i, gameVersion: %i)"), saveVersion, gameVersion);

	for (int tier = 0; tier < slotData.hubLayout.Num(); tier++) {
		for (int milestone = 0; milestone < slotData.hubLayout[tier].Num(); milestone++) {
			FApSaveableHubLayout hubLayout;
			hubLayout.tier = tier;
			hubLayout.milestone = milestone;
			hubLayout.costs = slotData.hubLayout[tier][milestone];

			saveSlotDataHubLayout.Add(hubLayout);
		}
	}
}	

void AApSubsystem::PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::PostSaveGame_Implementation(saveVersion: %i, gameVersion: %i)"), saveVersion, gameVersion);

	saveSlotDataHubLayout.Empty();
}*/

//TODO fix is now fired when loading a fresh save
void AApSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::PostLoadGame_Implementation(saveVersion: %i, gameVersion: %i)"), saveVersion, gameVersion);

	/*if (!saveSlotDataHubLayout.IsEmpty() && slotData.numberOfChecksPerMilestone > 0) {
		for (FApSaveableHubLayout hubLayout : saveSlotDataHubLayout) {
			if ((slotData.hubLayout.Num() - 1) < hubLayout.tier)
				slotData.hubLayout.Add(TArray<TMap<FString, int>>());

			if ((slotData.hubLayout[hubLayout.tier].Num() - 1) < hubLayout.milestone)
				slotData.hubLayout[hubLayout.tier].Add(hubLayout.costs);
		}

		slotData.hasLoadedSlotData = true;
	}*/

	//if (!scoutedLocations.IsEmpty())
	//	areScoutedLocationsReadyToParse = true;

	FApConfigurationStruct modConfig = FApConfigurationStruct::GetActiveConfig(GetWorld());
	if (!config.IsLoaded() || modConfig.ForceOverride)
		config = modConfig;

	config.Debugging = modConfig.Debugging;
}

void AApSubsystem::AbortGame(FText reason) {
	if (IsRunningDedicatedServer()) {
		UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::AbortGame(%s)"), *reason.ToString());
	} else {
		UWorld* world = GEngine->GameViewport->GetWorld();
		UApGameInstanceModule* gameInstance = UApUtils::GetGameInstanceModule(world);
		APlayerController* player = world->GetFirstPlayerController();

		gameInstance->YeetToMainMenu(player, reason);
	}
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
