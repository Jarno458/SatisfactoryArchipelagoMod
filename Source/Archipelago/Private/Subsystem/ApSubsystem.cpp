#include "Subsystem/ApSubsystem.h"
#include "ApUtils.h"
#include "Async/Async.h"

DEFINE_LOG_CATEGORY(LogApSubsystem);

//TODO REMOVE
#pragma optimize("", off)

#define LOCTEXT_NAMESPACE "Archipelago"

AApSubsystem::AApSubsystem() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.5f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;

	ConnectionState = EApConnectionState::NotYetAttempted;
	ConnectionStateDescription = LOCTEXT("NotYetAttempted", "A connection has not yet been attempted. Load a save file to attempt to connect.");
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

void AApSubsystem::DispatchLifecycleEvent(ELifecyclePhase phase, TArray<TSubclassOf<UFGSchematic>> apHardcodedSchematics) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem()::DispatchLifecycleEvent(%s)"), *UEnum::GetValueAsString(phase));

	if (!HasAuthority())
		UE_LOG(LogApSubsystem, Fatal, TEXT("AApSubsystem()::DispatchLifecycleEvent() Called without authority"));

	if (config.IsLoaded() && !config.Enabled) {
		UE_LOG(LogApSubsystem, Warning, TEXT("Archipelago manually disabled by user config"));
		return;
	}

	if (phase == ELifecyclePhase::CONSTRUCTION) {
		if (!config.IsLoaded())
			config = FApConfigurationStruct::GetActiveConfig(GetWorld());

		hardcodedSchematics = apHardcodedSchematics;

		for (TSubclassOf<UFGSchematic>& schematic : hardcodedSchematics) {
			UFGSchematic* schematicCDO = Cast<UFGSchematic>(schematic->GetDefaultObject());
			if (!IsValid(schematicCDO))
				continue;

			FString className = schematicCDO->GetName();
			if (className.Contains("Slots_"))
				inventorySlotRecipes.Add(schematic);
		}
	} else if (phase == ELifecyclePhase::INITIALIZATION) {
		SetActorTickEnabled(true);

		UE_LOG(LogApSubsystem, Display, TEXT("Initiating Archipelago server connection in background..."));
		bool dummy = CallOnGameThread<bool>([this]() { 
			ConnectToArchipelago(); 
			return true;
		});

		UWorld* world = GetWorld();
		contentLibSubsystem = world->GetGameInstance()->GetSubsystem<UContentLibSubsystem>();
		fgcheck(contentLibSubsystem)
		contentRegistry = UModContentRegistry::Get(world);
		fgcheck(contentRegistry)
		mappingSubsystem = AApMappingsSubsystem::Get(world);
		fgcheck(mappingSubsystem)
		RManager = AFGResearchManager::Get(world);
		fgcheck(RManager)
		unlockSubsystem = AFGUnlockSubsystem::Get(world);
		fgcheck(unlockSubsystem)

		//TODO: generatic AP Items can be totally hardcoded outside of the initialization phase
		UE_LOG(LogApSubsystem, Display, TEXT("Generating schematics from AP Item IDs..."));
		for (TPair<int64, TSharedRef<FApItemBase>>& apitem : mappingSubsystem->ApItems) {
			if (apitem.Value->Type == EItemType::Recipe || apitem.Value->Type == EItemType::Building)
				CreateSchematicBoundToItemId(apitem.Key, StaticCastSharedRef<FApRecipeItem>(apitem.Value));
		}

		UE_LOG(LogApSubsystem, Display, TEXT("Waiting for AP Server connection to succeed..."));
		FDateTime connectingStartedTime = FDateTime::Now();

		FGenericPlatformProcess::ConditionalSleep([this, connectingStartedTime]() { 
			return InitializeTick(connectingStartedTime); 
		}, 0.5);
	} else if (phase == ELifecyclePhase::POST_INITIALIZATION) {
		if (ConnectionState == EApConnectionState::Connected) {
			TArray<TSubclassOf<UFGSchematic>> unlockedSchematics;

			SManager->GetAllPurchasedSchematics(unlockedSchematics);

			for (TSubclassOf<UFGSchematic>& schematic : unlockedSchematics)
				OnSchematicCompleted(schematic);
		}
	}
}

bool AApSubsystem::UpdateFreeSamplesConfiguration(){
	FConfigId FreeSamplesConfigId { "FreeSamples", "" };

	UConfigManager* ConfigManager = GetWorld()->GetGameInstance()->GetSubsystem<UConfigManager>();
	if (ConfigManager == nullptr) {
		return false;
	}

	UConfigPropertySection* configRoot = ConfigManager->GetConfigurationRootSection(FreeSamplesConfigId);
	if (configRoot == nullptr) {
		return false;
	}

	if (configRoot->SectionProperties.Contains("Equipment")) {
		UConfigPropertySection* EquipmentSection = Cast<UConfigPropertySection>(configRoot->SectionProperties["Equipment"]);
		if (EquipmentSection != nullptr && EquipmentSection->SectionProperties.Contains("Quantity")) {
			UConfigPropertyInteger* quantity = Cast<UConfigPropertyInteger>(EquipmentSection->SectionProperties["Quantity"]);
			if (quantity != nullptr)
				quantity->Value = slotData.freeSampleEquipment;
		}
	}

	if (configRoot->SectionProperties.Contains("Buildings")) {
		UConfigPropertySection* BuildingsSection = Cast<UConfigPropertySection>(configRoot->SectionProperties["Buildings"]);
		if (BuildingsSection != nullptr && BuildingsSection->SectionProperties.Contains("Quantity")) {
			UConfigPropertyInteger* quantity = Cast<UConfigPropertyInteger>(BuildingsSection->SectionProperties["Quantity"]);
			if (quantity != nullptr)
				quantity->Value = slotData.freeSampleBuildings;
		}
	}

	if (configRoot->SectionProperties.Contains("Parts")) {
		UConfigPropertySection* PartsSection = Cast<UConfigPropertySection>(configRoot->SectionProperties["Parts"]);
		if (PartsSection != nullptr && PartsSection->SectionProperties.Contains("Quantity")) {
			UConfigPropertyInteger* quantity = Cast<UConfigPropertyInteger>(PartsSection->SectionProperties["Quantity"]);
			if (quantity != nullptr)
				quantity->Value = slotData.freeSampleParts;
		}
	}

	if (configRoot->SectionProperties.Contains("Exclude")) {
		UConfigPropertySection* ExcludeSection = Cast<UConfigPropertySection>(configRoot->SectionProperties["Exclude"]);
		if (ExcludeSection != nullptr && ExcludeSection->SectionProperties.Contains("SkipRadioactive")) {
			UConfigPropertyBool* excludeRadioActive = Cast<UConfigPropertyBool>(ExcludeSection->SectionProperties["SkipRadioactive"]);
			if (excludeRadioActive != nullptr)
				excludeRadioActive->Value = !slotData.freeSampleRadioactive;
		}
	}

	ConfigManager->MarkConfigurationDirty(FreeSamplesConfigId);
	ConfigManager->FlushPendingSaves();
	ConfigManager->ReloadModConfigurations();

	return true;
}

void AApSubsystem::SetMamEnhancerConfigurationHooks() {
	FConfigId MamEnhancerConfigId{ "MAMTips", "" };

	UConfigManager* ConfigManager = GetWorld()->GetGameInstance()->GetSubsystem<UConfigManager>();
	if (ConfigManager == nullptr) {
		return;
	}

	UConfigPropertySection* configRoot = ConfigManager->GetConfigurationRootSection(MamEnhancerConfigId);
	if (configRoot == nullptr) {
		return;
	}

	if (configRoot->SectionProperties.Contains("ShowHiddenNodesDetails")) {
		UConfigPropertyBool* showHidenNodeDetails = Cast<UConfigPropertyBool>(configRoot->SectionProperties["ShowHiddenNodesDetails"]);
		if (showHidenNodeDetails != nullptr)
			showHidenNodeDetails->OnPropertyValueChanged.AddDynamic(this, &AApSubsystem::LockMamEnhancerSpoilerConfiguration);
	}

	if (configRoot->SectionProperties.Contains("MakeHiddenPrettyMode")) {
		UConfigPropertyInteger* hidenDisplayMode = Cast<UConfigPropertyInteger>(configRoot->SectionProperties["MakeHiddenPrettyMode"]);
		if (hidenDisplayMode != nullptr)
			hidenDisplayMode->OnPropertyValueChanged.AddDynamic(this, &AApSubsystem::LockMamEnhancerSpoilerConfiguration);
	}

	LockMamEnhancerSpoilerConfiguration();
}


void AApSubsystem::LockMamEnhancerSpoilerConfiguration() {
	FConfigId MamEnhancerConfigId{ "MAMTips", "" };

	UConfigManager* ConfigManager = GetWorld()->GetGameInstance()->GetSubsystem<UConfigManager>();
	if (ConfigManager == nullptr) {
		return;
	}

	UConfigPropertySection* configRoot = ConfigManager->GetConfigurationRootSection(MamEnhancerConfigId);
	if (configRoot == nullptr) {
		return;
	}

	bool dirty = false;

	if (configRoot->SectionProperties.Contains("ShowHiddenNodesDetails")) {
		UConfigPropertyBool* showHidenNodeDetails = Cast<UConfigPropertyBool>(configRoot->SectionProperties["ShowHiddenNodesDetails"]);
		if (showHidenNodeDetails != nullptr) {
			if (showHidenNodeDetails->Value) {
				showHidenNodeDetails->Value = false;

				dirty = true;
			}
		}
	}

	// 1 = Empty Gray Boxes (Base Game)
	// 2 = Show Question Mark Icons
	// 5 = Show "Who's That Jace?" Icons (Silly)
	if (configRoot->SectionProperties.Contains("MakeHiddenPrettyMode")) {
		UConfigPropertyInteger* hidenDisplayMode = Cast<UConfigPropertyInteger>(configRoot->SectionProperties["MakeHiddenPrettyMode"]);
		if (hidenDisplayMode != nullptr) {
			if (hidenDisplayMode->Value != 1 && hidenDisplayMode->Value != 2 && hidenDisplayMode->Value != 5) {
				hidenDisplayMode->Value = 1;

				dirty = true;
			}
		}
	}

	if (dirty) {
		ConfigManager->MarkConfigurationDirty(MamEnhancerConfigId);
		ConfigManager->FlushPendingSaves();
		ConfigManager->ReloadModConfigurations();
	}
}

bool AApSubsystem::InitializeTick(FDateTime connectingStartedTime) {
	return CallOnGameThread<bool>([this, connectingStartedTime]() {
		if (ConnectionState == EApConnectionState::Connecting) {
			if ((FDateTime::Now() - connectingStartedTime).GetSeconds() > 15)
				TimeoutConnection();
			else
				CheckConnectionState();
		} 
		if (ConnectionState == EApConnectionState::Connected) {
			if (!hasLoadedRoomInfo)
				LoadRoomInfo();

			if (currentPlayerSlot == 0) {
				currentPlayerTeam = AP_GetCurrentPlayerTeam();
				currentPlayerSlot = AP_GetPlayerID();
			}

			if (!areScoutedLocationsReadyToParse && !hasScoutedLocations)
				ScoutArchipelagoItems();

			if (!mappingSubsystem->HasLoadedItemNameMappings())
				mappingSubsystem->InitializeAfterConnectingToAp();
		}

		if (!areRecipiesAndSchematicsInitialized
			&& areScoutedLocationsReadyToParse
			&& slotData.hasLoadedSlotData
			&& mappingSubsystem->HasLoadedItemNameMappings()) {

			ParseScoutedItemsAndCreateRecipiesAndSchematics();

			//must be called before BeginPlay() as FreeSamples caches its exclude radioactive flag there
			if (!UpdateFreeSamplesConfiguration()) {
				UE_LOG(LogApSubsystem, Error, TEXT("Failed update configuration of Free Samples"));
			}
		}
		
		return ConnectionState == EApConnectionState::ConnectionFailed 
			|| (areRecipiesAndSchematicsInitialized && ConnectionState == EApConnectionState::Connected);
	});
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
	AP_SetDeathLinkSupported(true);

	if (!slotData.hasLoadedSlotData)
		AP_RegisterSlotDataRawCallback("Data", AApSubsystem::ParseSlotData);

	ConnectionState = EApConnectionState::Connecting;
	ConnectionStateDescription = LOCTEXT("Connecting", "Connecting...");

	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::Starting AP"));
	AP_Start();
}

void AApSubsystem::BeginPlay() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::BeginPlay()"));

	Super::BeginPlay();

	UWorld* world = GetWorld();
	SManager = AFGSchematicManager::Get(world);

	portalSubsystem = AApPortalSubsystem::Get(world);
	mappingSubsystem = AApMappingsSubsystem::Get(world);
	trapSubsystem = AApTrapSubsystem::Get(world);
	phaseManager = AFGGamePhaseManager::Get(world);

	RManager->ResearchCompletedDelegate.AddDynamic(this, &AApSubsystem::OnMamResearchCompleted);
	RManager->ResearchTreeUnlockedDelegate.AddDynamic(this, &AApSubsystem::OnMamResearchTreeUnlocked);
	SManager->PurchasedSchematicDelegate.AddDynamic(this, &AApSubsystem::OnSchematicCompleted);

	SetMamEnhancerConfigurationHooks();
}

void AApSubsystem::EndPlay(const EEndPlayReason::Type endPlayReason) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::EndPlay(%i)"), endPlayReason);

	Super::EndPlay(endPlayReason);

	CallOnGameThread<void>([]() { AP_Shutdown(); });
}

void AApSubsystem::OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubSystem::OnResearchCompleted(schematic)"));

	OnAvaiableSchematicsChanged();
}

void AApSubsystem::OnMamResearchTreeUnlocked(TSubclassOf<class UFGResearchTree> researchTree) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubSystem::OnMamResearchTreeUnlocked(researchTree)"));

	OnAvaiableSchematicsChanged();
}

void AApSubsystem::OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubSystem::OnSchematicCompleted(schematic)"));

	ESchematicType type = UFGSchematic::GetType(schematic);

	std::set<int64> unlockedChecks;

	switch (type) {
		case ESchematicType::EST_Milestone:
			if (locationsPerMilestone.Contains(schematic)) {
				for (auto& location : locationsPerMilestone[schematic])
					unlockedChecks.insert(location.location);
			}
			break;
		case ESchematicType::EST_MAM:
			if (locationPerMamNode.Contains(schematic))
				unlockedChecks.insert(locationPerMamNode[schematic].location);
			break;
		case ESchematicType::EST_ResourceSink:
			if (locationPerShopNode.Contains(schematic))
				unlockedChecks.insert(locationPerShopNode[schematic].location);
			break;
		case ESchematicType::EST_Custom:
			if (schematic == ItemSchematics[mappingSubsystem->GetAwesomeShopItemId()] 
				|| schematic == ItemSchematics[mappingSubsystem->GetMamItemId()])
				OnAvaiableSchematicsChanged();
			break;
		default:
			return;
	}

	if (unlockedChecks.size() > 0) {
		if (ConnectionState == EApConnectionState::Connected) {
			CallOnGameThread<void>([unlockedChecks]() { AP_SendItem(unlockedChecks); });
		} else {
			for (int64 location : unlockedChecks) {
				switch (type) {
					case ESchematicType::EST_Milestone:
						for (TPair<TSubclassOf<UFGSchematic>, TArray<FApNetworkItem>>& itemPerMilestone : locationsPerMilestone) {
							for (int index = 0; index < itemPerMilestone.Value.Num(); index++) {
								FApNetworkItem networkItem = itemPerMilestone.Value[index];
								if (networkItem.location == location) {
									if (networkItem.player == currentPlayerSlot)
										ReceivedItems.Enqueue(TTuple<int64, bool>(networkItem.item, false));

									continue;
								}
							}
						}
						break;
					case ESchematicType::EST_MAM:
						for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerMamNode : locationPerMamNode) {
							if (itemPerMamNode.Value.location == location) {
								if (itemPerMamNode.Value.player == currentPlayerSlot)
									ReceivedItems.Enqueue(TTuple<int64, bool>(itemPerMamNode.Value.item, false));

								continue;
							}
						}
						break;
					case ESchematicType::EST_ResourceSink:
						for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerShopNode : locationPerShopNode) {
							if (itemPerShopNode.Value.location == location) {
								if (itemPerShopNode.Value.player == currentPlayerSlot)
									ReceivedItems.Enqueue(TTuple<int64, bool>(itemPerShopNode.Value.item, false));

								continue;
							}
						}
						break;
					default:
						continue;
				}
			}
		}
	}
}

void AApSubsystem::OnAvaiableSchematicsChanged() {
	std::set<int64> locationHintsToPublish;

	int maxAvailableTechTier = ((int)phaseManager->GetGamePhase() + 1) * 2;

	for (TPair<TSubclassOf<UFGSchematic>, TArray<FApNetworkItem>>& itemPerMilestone : locationsPerMilestone) {
		if (UFGSchematic::GetTechTier(itemPerMilestone.Key) <= maxAvailableTechTier) {
			for (FApNetworkItem item : itemPerMilestone.Value) {
				if (item.player != currentPlayerSlot && (item.flags & 0b011) > 0)
					locationHintsToPublish.insert(item.location);
			}
		}
	}

	if (SManager->IsSchematicPurchased(ItemSchematics[mappingSubsystem->GetMamItemId()])) {
		for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerMamNode : locationPerMamNode) {
			if (itemPerMamNode.Value.player != currentPlayerSlot 
				&& (itemPerMamNode.Value.flags & 0b011) > 0
				&& RManager->CanResearchBeInitiated(itemPerMamNode.Key))

				locationHintsToPublish.insert(itemPerMamNode.Value.location);
		}
	}

	if (SManager->IsSchematicPurchased(ItemSchematics[mappingSubsystem->GetAwesomeShopItemId()])) {
		for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerShopNode : locationPerShopNode) {
			if (itemPerShopNode.Value.player != currentPlayerSlot && (itemPerShopNode.Value.flags & 0b011) > 0)
				locationHintsToPublish.insert(itemPerShopNode.Value.location);
		}
	}

	CallOnGameThread<void>([locationHintsToPublish]() { AP_SendLocationScouts(locationHintsToPublish, 2); });
}

AApSubsystem* AApSubsystem::callbackTarget;

void AApSubsystem::ItemClearCallback() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ItemClearCallback()"));

	callbackTarget->currentItemIndex = 0;
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

	if (!callbackTarget->scoutedLocations.IsEmpty())
		return;

	callbackTarget->scoutedLocations = TArray<FApNetworkItem>();

	for (AP_NetworkItem apLocation : scoutedLocations) {
		FApNetworkItem location;
		location.item = apLocation.item;
		location.location = apLocation.location;
		location.player = apLocation.player;
		location.flags = apLocation.flags;
		location.itemName = UApUtils::FStr(apLocation.itemName);
		location.locationName = UApUtils::FStr(apLocation.locationName);
		location.playerName = UApUtils::FStr(apLocation.playerName);

		callbackTarget->scoutedLocations.Add(location);
	}
		
	callbackTarget->areScoutedLocationsReadyToParse = true;
}

void AApSubsystem::ParseSlotData(std::string json) {
	FString jsonString = UApUtils::FStr(json);

	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ParseSlotData(\"%s\")"), *jsonString);
	
	bool success = FApSlotData::ParseSlotData(jsonString, &callbackTarget->slotData);
	if (!success) {
		FText jsonText = FText::FromString(jsonString);
		AbortGame(FText::Format(LOCTEXT("SlotDataInvallid", "Archipelago SlotData Invalid! {0}"), jsonText));
	}
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
	if (roomSeed.IsEmpty())
		roomSeed = seedName;
	else if (roomSeed != seedName)
		AbortGame(LOCTEXT("RoomSeedMissmatch", "Room seed does not match seed of save, this save does not belong to the multiworld your connecting to"));

	hasLoadedRoomInfo = true;
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

		SetServerData(setDefaultAndRecieceUpdate);
	});
}

//TODO FIX ME
void AApSubsystem::SetServerData(AP_SetServerDataRequest& setDataRequest) {
	CallOnGameThread<void>([setDataRequest]() { 
		AP_SetServerDataRequest copy = setDataRequest;
		AP_SetServerData(&copy);
	});
}

void AApSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (portalSubsystem->IsInitialized())
		ReceiveItems();

	if (ConnectionState != EApConnectionState::Connected)
		return;

	HandleCheckedLocations();
	HandleAPMessages();
	HandleDeathLink();

	if (phaseManager->GetGamePhase() > lastGamePhase) {
		OnAvaiableSchematicsChanged();
		lastGamePhase = phaseManager->GetGamePhase();
	}
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

void AApSubsystem::ReceiveItems() {
	// Consider processing only one queue item per tick for performance reasons
	TTuple<int64, bool> item;
	while (ReceivedItems.Dequeue(item)) {
		if (ConnectionState == EApConnectionState::Connected && ++currentItemIndex < lastProcessedItemIndex)
			continue;

		AwardItem(item.Key, item.Value);

		lastProcessedItemIndex++;
	}
}

void AApSubsystem::AwardItem(int64 itemid, bool isFromServer) {
	if (ItemSchematics.Contains(itemid)) {
		SManager->GiveAccessToSchematic(ItemSchematics[itemid], nullptr);
	}
	else if (auto trapName = UApMappings::ItemIdToTrap.Find(itemid)) {
		trapSubsystem->SpawnTrap(*trapName, nullptr);
	}
	else if (mappingSubsystem->ApItems.Contains(itemid)) {
		if (mappingSubsystem->ApItems[itemid]->Type == EItemType::Item) {
			if (isFromServer && !IsRunningDedicatedServer()) { //TODO fix rewarding starter inventory to newly spawned clients 
				AFGCharacterPlayer* player = GetLocalPlayer();
				fgcheck(player);
				UFGInventoryComponent* inventory = player->GetInventory();
				TSubclassOf<UFGItemDescriptor> itemClass = StaticCastSharedRef<FApItem>(mappingSubsystem->ApItems[itemid])->Class;
				int stackSize = UFGItemDescriptor::GetStackSize(itemClass);
				FInventoryStack stack = FInventoryStack(stackSize, itemClass);

				int numAdded = inventory->AddStack(stack, true);
				if (numAdded < stackSize)
					portalSubsystem->Enqueue(itemClass, stackSize - numAdded);
			}
			else {
				TSubclassOf<UFGItemDescriptor> itemClass = StaticCastSharedRef<FApItem>(mappingSubsystem->ApItems[itemid])->Class;
				int stackSize = UFGItemDescriptor::GetStackSize(itemClass);
				portalSubsystem->Enqueue(itemClass, stackSize);
			}
		}
		else if (mappingSubsystem->ApItems[itemid]->Type == EItemType::Schematic) {
			SManager->GiveAccessToSchematic(StaticCastSharedRef<FApSchematicItem>(mappingSubsystem->ApItems[itemid])->Class, nullptr);
		}
		else if (mappingSubsystem->ApItems[itemid]->Type == EItemType::Special) {
			ESpecialItemType specialType = StaticCastSharedRef<FApSpecialItem>(mappingSubsystem->ApItems[itemid])->SpecialType;
			switch (specialType) {
			case ESpecialItemType::Inventory3:
			case ESpecialItemType::Inventory6: 
				unlockSubsystem->UnlockInventorySlots((specialType == ESpecialItemType::Inventory3) ? 3 : 6);
				break;
			case ESpecialItemType::Toolbelt1:
				for (int i = 0; i < inventorySlotRecipes.Num(); i++) {
					if (SManager->IsSchematicPurchased(inventorySlotRecipes[i], nullptr))
						continue;

					SManager->GiveAccessToSchematic(inventorySlotRecipes[i], nullptr);
					break;
				}
				break;
			}
		}
	}
}

void AApSubsystem::HandleCheckedLocations() {
	int64 location;

	//could use a while loop but we now handle only 1 per tick for perform reasons as this opperation is quite slow
	if (CheckedLocations.Dequeue(location)) {
		for (TPair<TSubclassOf<UFGSchematic>, TArray<FApNetworkItem>>& itemPerMilestone : locationsPerMilestone) {
			for (int index = 0; index < itemPerMilestone.Value.Num(); index++) {
				FApNetworkItem networkItem = itemPerMilestone.Value[index];
				if (networkItem.location == location) {
					UFGSchematic* schematic = Cast<UFGSchematic>(itemPerMilestone.Key->GetDefaultObject());

					if (schematic != nullptr && !IsCollected(schematic->mUnlocks[index])) {
						Collect(schematic->mUnlocks[index], networkItem);

						if (!schematic->mUnlocks.ContainsByPredicate([this](UFGUnlock* unlock) { return !IsCollected(unlock); }))
							SManager->GiveAccessToSchematic(itemPerMilestone.Key, nullptr);
					}
				}
			}
		}
		for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerMamNode : locationPerMamNode) {
			if (itemPerMamNode.Value.location == location) {
				UFGSchematic* schematic = Cast<UFGSchematic>(itemPerMamNode.Key->GetDefaultObject());

				if (schematic != nullptr && !IsCollected(schematic->mUnlocks[0])) {
					Collect(schematic->mUnlocks[0], itemPerMamNode.Value);

					SManager->GiveAccessToSchematic(itemPerMamNode.Key, nullptr);
				}
			}
		}
		for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerShopNode : locationPerShopNode) {
			if (itemPerShopNode.Value.location == location) {
				UFGSchematic* schematic = Cast<UFGSchematic>(itemPerShopNode.Key->GetDefaultObject());

				if (schematic != nullptr && !IsCollected(schematic->mUnlocks[0])) {
					Collect(schematic->mUnlocks[0], itemPerShopNode.Value);

					SManager->GiveAccessToSchematic(itemPerShopNode.Key, nullptr);
				}
			}
		}
	}
}

AFGCharacterPlayer* AApSubsystem::GetLocalPlayer() {
	if (IsRunningDedicatedServer())
		return nullptr;

	AFGPlayerController* playerController = UFGBlueprintFunctionLibrary::GetLocalPlayerController(GetWorld());
	return Cast<AFGCharacterPlayer>(playerController->GetControlledCharacter());
}

bool AApSubsystem::IsCollected(UFGUnlock* unlock) {
	UFGUnlockInfoOnly* unlockInfo = Cast<UFGUnlockInfoOnly>(unlock);
	return unlockInfo != nullptr && unlockInfo->mUnlockIconSmall == collectedIcon;
}

void AApSubsystem::Collect(UFGUnlock* unlock, FApNetworkItem& networkItem) {
	UFGUnlockInfoOnly* unlockInfo = Cast<UFGUnlockInfoOnly>(unlock);

	if (unlockInfo != nullptr) {
		unlockInfo->mUnlockIconSmall = collectedIcon;

		if (networkItem.player != currentPlayerSlot) {
			unlockInfo->mUnlockName = FText::Format(LOCTEXT("Collected", "Collected: {0}"), unlockInfo->mUnlockName);
			unlockInfo->mUnlockIconBig = collectedIcon;
		}
	}
}

void AApSubsystem::CheckConnectionState() {
	if (!IsInGameThread())
		return;

	if (ConnectionState == EApConnectionState::Connecting) {
		AP_ConnectionStatus status = AP_GetConnectionStatus();

		if (status == AP_ConnectionStatus::Authenticated) {
			ConnectionState = EApConnectionState::Connected;
			ConnectionStateDescription = LOCTEXT("AuthSuccess", "Authentication succeeded.");
			UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::Tick(), Successfully Authenticated"));
		} else if (status == AP_ConnectionStatus::ConnectionRefused) {
			ConnectionState = EApConnectionState::ConnectionFailed;
			ConnectionStateDescription = LOCTEXT("ConnectionRefused", "Connection refused by server. Check your connection details and load the save again.");
			UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::CheckConnectionState(), ConnectionRefused"));
		}
	}
}

void AApSubsystem::ParseScoutedItemsAndCreateRecipiesAndSchematics() {
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

void AApSubsystem::CreateSchematicBoundToItemId(int64 itemid, TSharedRef<FApRecipeItem> apitem) {
	FString name = FString::Printf(TEXT("AP_ItemId_%i"), itemid);

	TTuple<bool, TSubclassOf<UFGSchematic>> foundSchematic = UApUtils::FindOrCreateClass(TEXT("/Archipelago/"), *name, UFGSchematic::StaticClass());
	if (!foundSchematic.Key) {
		TArray<FString> recipesToUnlock;
		for (FApRecipeInfo& recipe : apitem->Recipes) {
			recipesToUnlock.Add(recipe.Class->GetName());
		}

		FString typePrefix = apitem->Type == EItemType::Building ? "Building: " : "Recipe: ";
		FString recipeName = apitem->Recipes[0].Recipe->GetDisplayName().ToString();

		FContentLib_Schematic schematic = FContentLib_Schematic();
		schematic.Name = typePrefix + recipeName;
		schematic.Type = "Custom";
		schematic.Recipes = recipesToUnlock;

		UCLSchematicBPFLib::InitSchematicFromStruct(schematic, foundSchematic.Value, contentLibSubsystem);

		contentRegistry->RegisterSchematic(FName(TEXT("Archipelago")), foundSchematic.Value);
	}
	
	ItemSchematics.Add(itemid, foundSchematic.Value);
}

void AApSubsystem::InitializaHubSchematic(FString name, TSubclassOf<UFGSchematic> factorySchematic, TArray<FApNetworkItem> items) {
	int delimeterPos;
	name.FindChar('-', delimeterPos);
	int32 tier = FCString::Atoi(*name.Mid(delimeterPos - 1, 1));
	int32 milestone = FCString::Atoi(*name.Mid(delimeterPos + 1, 1));

	FContentLib_Schematic schematic = FContentLib_Schematic();
	schematic.Name = name;
	schematic.Type = "Milestone";
	schematic.Time = 200;
	schematic.Tier = tier;
	schematic.MenuPriority = items[0].location;
	schematic.VisualKit = "Kit_AP_Logo";
	schematic.Cost = slotData.hubLayout[tier - 1][milestone - 1];

	for (auto& item : items)
		schematic.InfoCards.Add(CreateUnlockInfoOnly(item));

	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, contentLibSubsystem);
}

void AApSubsystem::InitializaSchematicForItem(TSubclassOf<UFGSchematic> factorySchematic, FApNetworkItem item, bool updateSchemaName) {
	FContentLib_UnlockInfoOnly unlockOnlyInfo = CreateUnlockInfoOnly(item);

	FContentLib_Schematic schematic = FContentLib_Schematic();
	schematic.Description = unlockOnlyInfo.mUnlockDescription.ToString();
	//schematic.VisualKit = unlockOnlyInfo.BigIcon; using visual kit didnt work here so we manually patch the icons below
	schematic.InfoCards = TArray<FContentLib_UnlockInfoOnly>{ unlockOnlyInfo };

	if (updateSchemaName)
		schematic.Name = unlockOnlyInfo.mUnlockName.ToString();

	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, contentLibSubsystem);

	UFGSchematic* factorySchematicCDO = Cast<UFGSchematic>(factorySchematic->GetDefaultObject());
	if (factorySchematicCDO != nullptr) {
		// ContentLib keeps adding new `InfoCards`
		if (factorySchematicCDO->mUnlocks.Num() > 1) {
			factorySchematicCDO->mUnlocks.RemoveAt(1, 1, true);
		}

		UTexture2D* bigText = LoadObject<UTexture2D>(nullptr, *unlockOnlyInfo.BigIcon);

		factorySchematicCDO->mSchematicIcon.SetResourceObject(bigText);
		factorySchematicCDO->mSmallSchematicIcon = bigText;

		if (!IsCollected(factorySchematicCDO->mUnlocks[0])) {
			UFGUnlockInfoOnly* unlockInfo = Cast<UFGUnlockInfoOnly>(factorySchematicCDO->mUnlocks[0]);

			if (unlockInfo != nullptr) {
				unlockInfo->mUnlockIconBig = bigText;
				unlockInfo->mUnlockIconSmall = bigText;
			}
		}
	}
}

FContentLib_UnlockInfoOnly AApSubsystem::CreateUnlockInfoOnly(FApNetworkItem item) {
	FFormatNamedArguments Args;
	if (item.flags == 0b001) {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeAdvancement", "progression item"));
	} else if (item.flags == 0b010) {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeUseful", "useful item"));
	} else if (item.flags == 0b100) {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeTrap", "trap"));
	} else {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeJunk", "normal item"));
	}

	Args.Add(TEXT("ApItemName"), FText::FromString(item.itemName));

	FContentLib_UnlockInfoOnly infoCard;

	if (item.player == currentPlayerSlot) {
		Args.Add(TEXT("ApPlayerName"), LOCTEXT("NetworkItemDescriptionYourOwnName", "your"));

		infoCard.mUnlockName = FText::FromString(item.itemName);

		if (item.player == currentPlayerSlot && mappingSubsystem->ApItems.Contains(item.item)) {
			TSharedRef<FApItemBase> apItem = mappingSubsystem->ApItems[item.item];

			if (apItem->Type == EItemType::Building) {
				UpdateInfoOnlyUnlockWithBuildingInfo(&infoCard, Args, &item, StaticCastSharedRef<FApBuildingItem>(apItem));
			} else if (apItem->Type == EItemType::Recipe) {
				UpdateInfoOnlyUnlockWithRecipeInfo(&infoCard, Args, &item, StaticCastSharedRef<FApRecipeItem>(apItem));
			} else if (apItem->Type == EItemType::Item) {
				UpdateInfoOnlyUnlockWithItemBundleInfo(&infoCard, Args, &item, StaticCastSharedRef<FApItem>(apItem));
			} else if (apItem->Type == EItemType::Schematic) {
				UpdateInfoOnlyUnlockWithSchematicInfo(&infoCard, Args, &item, StaticCastSharedRef<FApSchematicItem>(apItem));
			} else if (apItem->Type == EItemType::Special) {
				UpdateInfoOnlyUnlockWithSpecialInfo(&infoCard, Args, &item, StaticCastSharedRef<FApSpecialItem>(apItem));
			}
		} else {
			UpdateInfoOnlyUnlockWithGenericApInfo(&infoCard, Args, &item);
		}
	} else {
		Args.Add(TEXT("ApPlayerName"), FText::FormatNamed(LOCTEXT("NetworkItemPlayerOwnerPossessive", "{remotePlayerName}'s"),
			TEXT("remotePlayerName"), FText::FromString(item.playerName)
		));

		infoCard.mUnlockName = FText::Format(LOCTEXT("NetworkItemUnlockDisplayName", "{ApPlayerName} {ApItemName}"), Args);

		UpdateInfoOnlyUnlockWithGenericApInfo(&infoCard, Args, &item);
	}

	return infoCard;
}

void AApSubsystem::UpdateInfoOnlyUnlockWithBuildingInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApBuildingItem> itemInfo) {
	UFGRecipe* recipe = itemInfo->Recipes[0].Recipe;
	UFGItemDescriptor* itemDescriptor = recipe->GetProducts()[0].ItemClass.GetDefaultObject();

	infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
	infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Building.Recipe_Icon_Building");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalBuildingDescription", "This will unlock your {ApItemName}"), Args);
}

void AApSubsystem::UpdateInfoOnlyUnlockWithRecipeInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApRecipeItem> itemInfo) {
	UFGRecipe* recipe = itemInfo->Recipes[0].Recipe;
	UFGItemDescriptor* itemDescriptor = recipe->GetProducts()[0].ItemClass.GetDefaultObject();

	TArray<FString> BuildingArray;
	TArray<TSubclassOf<UObject>> buildings;
	recipe->GetProducedIn(buildings);
	for (TSubclassOf<UObject>& buildingObject : buildings) {
		if (buildingObject->IsChildOf(AFGBuildable::StaticClass())) {
			AFGBuildable* building = Cast<AFGBuildable>(buildingObject.GetDefaultObject());
			if (building != nullptr) {
				if (building->IsA(AFGBuildableAutomatedWorkBench::StaticClass()))
					BuildingArray.Add("Workbench");
				else
					BuildingArray.Add(building->mDisplayName.ToString());
			}
		} else if (buildingObject->IsChildOf(workshopComponent)) {
			BuildingArray.Add("Equipment Workshop");
		}
	}

	TArray<FString> CostsArray;
	for (FItemAmount cost : recipe->GetIngredients()) {
		UFGItemDescriptor* costItemDescriptor = cost.ItemClass.GetDefaultObject();
		CostsArray.Add(costItemDescriptor->GetItemNameFromInstanceAsString());
	}

	TArray<FString> OutputArray;
	for (FItemAmount product : recipe->GetProducts()) {
		UFGItemDescriptor* productItemDescriptor = product.ItemClass.GetDefaultObject();
		OutputArray.Add(productItemDescriptor->GetItemNameFromInstanceAsString());
	}

	Args.Add(TEXT("Building"), FText::FromString(FString::Join(BuildingArray, TEXT(", "))));
	Args.Add(TEXT("Costs"), FText::FromString(FString::Join(CostsArray, TEXT(", "))));
	Args.Add(TEXT("Output"), FText::FromString(FString::Join(OutputArray, TEXT(", "))));

	infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
	infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Decor.Recipe_Icon_Decor");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalRecipeDescription", "This will unlock {ApPlayerName} {ApItemName} which is considered a {ProgressionType}.\nProduced in: {Building}.\nCosts: {Costs}.\nProduces: {Output}."), Args);
}

void AApSubsystem::UpdateInfoOnlyUnlockWithItemBundleInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApItem> itemInfo) {
	UFGItemDescriptor* itemDescriptor = itemInfo->Descriptor;

	infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
	infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Item.Recipe_Icon_Item");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalItemDescription", "This will give {ApPlayerName} {ApItemName}. It can be collected by building an Archipelago Portal."), Args);
}

void AApSubsystem::UpdateInfoOnlyUnlockWithSchematicInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApSchematicItem> itemInfo) {
	UFGRecipe* recipe = nullptr;
	for (UFGUnlock* unlock : itemInfo->Schematic->mUnlocks) {
		UFGUnlockRecipe* recipeUnlockInfo = Cast<UFGUnlockRecipe>(unlock);
		if (recipeUnlockInfo == nullptr)
			continue;

		TArray<TSubclassOf<UFGRecipe>> recipes = recipeUnlockInfo->GetRecipesToUnlock();
		if (recipes.IsEmpty())
			continue;

		recipe = recipes[0].GetDefaultObject();
		break;
	}

	if (recipe != nullptr) {
		UFGItemDescriptor* itemDescriptor = recipe->GetProducts()[0].ItemClass.GetDefaultObject();

		infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
		infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Building.Recipe_Icon_Building");
		infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalBuildingDescription", "This will unlock your {ApItemName}"), Args);
	} else {
		UpdateInfoOnlyUnlockWithGenericApInfo(infoCard, Args, item);
	}
}

void AApSubsystem::UpdateInfoOnlyUnlockWithSpecialInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApSpecialItem> itemInfo) {
	switch (itemInfo->SpecialType) {
		case ESpecialItemType::Inventory3:
		case ESpecialItemType::Inventory6: {
				Args.Add(TEXT("Amount"), itemInfo->SpecialType == ESpecialItemType::Inventory3 ? FText::FromString("3") : FText::FromString("6"));

				infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Game/FactoryGame/Interface/UI/Assets/Shared/ThumbsUp_64.ThumbsUp_64");
				infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Upgrade.Recipe_Icon_Upgrade");
				infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalInventoryDescription", "This will inflate {ApPlayerName} pocket-dimension by {Amount}."), Args);
			}
			break;
		case ESpecialItemType::Toolbelt1:
			infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Game/FactoryGame/Interface/UI/Assets/Shared/ThumbsUp_64.ThumbsUp_64");
			infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Upgrade.Recipe_Icon_Upgrade");
			infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalHandSlotDescription", "This will expand {ApPlayerName} tool-chain by 1."), Args);
			break;
	}
}

void AApSubsystem::UpdateInfoOnlyUnlockWithGenericApInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item) {
	infoCard->CategoryIcon = TEXT("/Archipelago/Assets/SourceArt/ArchipelagoAssetPack/ArchipelagoIconWhite128.ArchipelagoIconWhite128");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockDescription", "This will unlock {ApPlayerName} {ApItemName} which is considered a {ProgressionType}."), Args);

	if (item->flags == 0b001) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/ApLogo/AP-Purple.AP-Purple");
	} else if (item->flags == 0b010) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/ApLogo/AP-Blue.AP-Blue");
	} else if (item->flags == 0b100) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/ApLogo/AP-Red.AP-Red");
	} else {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/ApLogo/AP-Cyan.AP-Cyan");
	}
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
	AApMessagingSubsystem* messaging = AApMessagingSubsystem::Get(this);
	fgcheck(messaging);
	messaging->DisplayMessage(Message, Color);
}

void AApSubsystem::ScoutArchipelagoItems() {
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
}

void AApSubsystem::TimeoutConnection() {
	ConnectionState = EApConnectionState::ConnectionFailed;
	ConnectionStateDescription = LOCTEXT("AuthFailed", "Authentication failed. Check your connection details and load the save again.");
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
}

//TODO fix is now fired when loading a fresh save
void AApSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::PostLoadGame_Implementation(saveVersion: %i, gameVersion: %i)"), saveVersion, gameVersion);

	if (!saveSlotDataHubLayout.IsEmpty() && slotData.numberOfChecksPerMilestone > 0)	{
		for (FApSaveableHubLayout hubLayout : saveSlotDataHubLayout) {
			if ((slotData.hubLayout.Num() - 1) < hubLayout.tier)
				slotData.hubLayout.Add(TArray<TMap<FString, int>>());

			if ((slotData.hubLayout[hubLayout.tier].Num() - 1) < hubLayout.milestone)
				slotData.hubLayout[hubLayout.tier].Add(hubLayout.costs);
		}

		slotData.hasLoadedSlotData = true;
	}

	if (!scoutedLocations.IsEmpty())
		areScoutedLocationsReadyToParse = true;

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
