#include "Subsystem/ApSubsystem.h"
#include "ApUtils.h"

DEFINE_LOG_CATEGORY(LogApSubsystem);

DEFINE_LOG_CATEGORY(LogApChat);

//TODO REMOVE
#pragma optimize("", off)

#define LOCTEXT_NAMESPACE "Archipelago"

std::map<std::string, std::function<void(AP_SetReply)>> AApSubsystem::callbacks;

AApSubsystem::AApSubsystem() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.5f;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer; // TODO_MULTIPLAYER is this what we want long term?

	ConnectionState = NotYetAttempted;
	ConnectionStateDescription = LOCTEXT("NotYetAttempted", "A connection has not yet been attempted. Load a save file to attempt to connect.");
}

AApSubsystem* AApSubsystem::Get() {
	return Get(GEngine->GameViewport->GetWorld());
}

AApSubsystem* AApSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	check(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApSubsystem>();
}

void AApSubsystem::DispatchLifecycleEvent(ELifecyclePhase phase) {
	if (phase == ELifecyclePhase::CONSTRUCTION) {
		if (!config.IsLoaded())
			config = FApConfigurationStruct::GetActiveConfig(GetWorld());
	}
		
	if (!config.Enabled) {
		UE_LOG(LogApSubsystem, Warning, TEXT("Archipelago manually disabled by user config"));
		return;
	}

	if (phase == ELifecyclePhase::INITIALIZATION) {
		// TODO_MULTIPLAYER calling HasAuthority crashes multiplayer client? too early?
		// but we're using SpawnOnServer so why/how is client running this anyways
		if (HasAuthority()) {
			// Calling SetActorTickEnabled on client crashes regardless of true or false? related to above issue?
			SetActorTickEnabled(true);
		} else {
			UE_LOG(LogApSubsystem, Warning, TEXT("Archipelago Subsystem spawned/replicated on client, this is untested behavior. Keeping tick disabled."));
		}

		contentLibSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UContentLibSubsystem>();
		check(contentLibSubsystem)
		contentRegistry = UModContentRegistry::Get(GetWorld());
		check(contentRegistry)

		UE_LOG(LogApSubsystem, Display, TEXT("Initiating Archipelago server connection in background..."));
		ConnectToArchipelago();

		mappingSubsystem = AApMappingsSubsystem::Get(GetWorld());

		UE_LOG(LogApSubsystem, Display, TEXT("Generating schematics from AP Item IDs..."));
		for (TPair<int64, TSharedRef<FApItemBase>> apitem : mappingSubsystem->ApItems) {
			if (apitem.Value->Type == EItemType::Recipe || apitem.Value->Type == EItemType::Building)
				CreateSchematicBoundToItemId(apitem.Key, StaticCastSharedRef<FApRecipeItem>(apitem.Value));
		}

		FDateTime connectingStartedTime = FDateTime::Now();
		FGenericPlatformProcess::ConditionalSleep([this, connectingStartedTime]() { return InitializeTick(connectingStartedTime); }, 0.5);
	} else if (phase == ELifecyclePhase::POST_INITIALIZATION) {
		if (ConnectionState == EApConnectionState::Connected) {\
			TArray<TSubclassOf<UFGSchematic>> unlockedSchematics;

			SManager->GetAllPurchasedSchematics(unlockedSchematics);

			std::vector<int64> unlockedChecks;

			for (TSubclassOf<UFGSchematic> schematic : unlockedSchematics) {
				if (locationsPerMilestone.Contains(schematic)) {
					for (FApNetworkItem item : locationsPerMilestone[schematic]) {
						unlockedChecks.emplace_back(item.location);
					}
				}

				//TODO add other location sources such as Mam, Shops etc
			}

			AP_SendItem(unlockedChecks);
		}
	}
}

bool AApSubsystem::InitializeTick(FDateTime connectingStartedTime) {
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

		if (!mappingSubsystem->IsInitialized())
			mappingSubsystem->InitializeAfterConnectingToAp();
	}

	if (!areRecipiesAndSchematicsInitialized 
		&& areScoutedLocationsReadyToParse 
		&& slotData.hasLoadedSlotData 
		&& mappingSubsystem->IsInitialized())
			ParseScoutedItemsAndCreateRecipiesAndSchematics();
	
	return ConnectionState == EApConnectionState::ConnectionFailed 
		|| (areRecipiesAndSchematicsInitialized && ConnectionState == EApConnectionState::Connected);
}

void AApSubsystem::ConnectToArchipelago() {
	std::string const uri = TCHAR_TO_UTF8(*config.Url);
	std::string const user = TCHAR_TO_UTF8(*config.Login);
	std::string const password = TCHAR_TO_UTF8(*config.Password);

	AP_Init(uri.c_str(), "Satisfactory", user.c_str(), password.c_str());

	AP_SetItemClearCallback(AApSubsystem::ItemClearCallback);
	AP_SetItemRecvCallback(AApSubsystem::ItemReceivedCallback);
	AP_SetLocationCheckedCallback(AApSubsystem::LocationCheckedCallback);
	AP_RegisterSetReplyCallback(AApSubsystem::SetReplyCallback);

	if (scoutedLocations.IsEmpty())
		AP_SetLocationInfoCallback(AApSubsystem::LocationScoutedCallback);

	if (!slotData.hasLoadedSlotData)
		AP_RegisterSlotDataRawCallback("Data", AApSubsystem::ParseSlotData);

	ConnectionState = EApConnectionState::Connecting;
	ConnectionStateDescription = LOCTEXT("Connecting", "Connecting...");

	AP_Start();
}

void AApSubsystem::BeginPlay() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::BeginPlay()"));

	Super::BeginPlay();

	UWorld* world = GetWorld();
	RManager = AFGResearchManager::Get(world);
	SManager = AFGSchematicManager::Get(world);

	portalSubsystem = AApPortalSubsystem::Get(world);
	mappingSubsystem = AApMappingsSubsystem::Get(world);
	trapSubsystem = AApTrapSubsystem::Get(world);
	goalSubsystem = AApGoalSubsystem::Get(world);

	RManager->ResearchCompletedDelegate.AddDynamic(this, &AApSubsystem::OnMamResearchCompleted);
	SManager->PurchasedSchematicDelegate.AddDynamic(this, &AApSubsystem::OnSchematicCompleted);
}

void AApSubsystem::EndPlay(const EEndPlayReason::Type endPlayReason) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::EndPlay(%i)"), endPlayReason);

	Super::EndPlay(endPlayReason);

	AP_Shutdown();
}

void AApSubsystem::OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubSystem::OnResearchCompleted(schematic), MAM Research Completed"));


}

void AApSubsystem::OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubSystem::OnSchematicCompleted(schematic)"));

	ESchematicType type = UFGSchematic::GetType(schematic);

	if (type != ESchematicType::EST_Milestone || !locationsPerMilestone.Contains(schematic))
		return;

	std::vector<int64> unlockedChecks;

	for (auto& location : locationsPerMilestone[schematic])
		unlockedChecks.emplace_back(location.location);
	
	AP_SendItem(unlockedChecks);
}

void AApSubsystem::ItemClearCallback() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ItemClearCallback()"));
}

void AApSubsystem::ItemReceivedCallback(int64 item, bool notify, bool isFromServer) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ItemReceivedCallback(%i, \"%s\")"), item, (notify ? TEXT("true") : TEXT("false")));

	AApSubsystem* self = AApSubsystem::Get();

	TTuple<int64, bool> receivedItem = TTuple<int64, bool>(item, isFromServer);
	self->ReceivedItems.Enqueue(receivedItem);
}

void AApSubsystem::LocationCheckedCallback(int64 id) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::LocationCheckedCallback(%i)"), id);

	// TODO, can be used to sync in coop or handling !collects of other games
}

void AApSubsystem::SetReplyCallback(AP_SetReply setReply) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::SetReplyCallback(\"%s\")"), *UApUtils::FStr(setReply.key));

	if (callbacks.count(setReply.key))
		callbacks[setReply.key](setReply);
}

void AApSubsystem::LocationScoutedCallback(std::vector<AP_NetworkItem> scoutedLocations) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::LocationScoutedCallback(vector[%i])"), scoutedLocations.size());

	AApSubsystem* self = AApSubsystem::Get();

	self->scoutedLocations = TArray<FApNetworkItem>();

	for (AP_NetworkItem apLocation : scoutedLocations) {
		FApNetworkItem location;
		location.item = apLocation.item;
		location.location = apLocation.location;
		location.player = apLocation.player;
		location.flags = apLocation.flags;
		location.itemName = UApUtils::FStr(apLocation.itemName);
		location.locationName = UApUtils::FStr(apLocation.locationName);
		location.playerName = UApUtils::FStr(apLocation.playerName);

		self->scoutedLocations.Add(location);
	}
		
	self->areScoutedLocationsReadyToParse = true;
}

void AApSubsystem::ParseSlotData(std::string json) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ParseSlotData(\"%s\")"), *UApUtils::FStr(json));
	
	AApSubsystem* self = AApSubsystem::Get();
	bool success = FApSlotData::ParseSlotData(json, &self->slotData);
	if (!success) {
		FText jsonText = UApUtils::FText(json);
		AbortGame(FText::Format(LOCTEXT("SlotDataInvallid", "Archipelago SlotData Invalid! %s"), jsonText));
	}
}

void AApSubsystem::LoadRoomInfo() {
	AP_RoomInfo roomInfo;
	AP_GetRoomInfo(&roomInfo);

	FString seedName = UApUtils::FStr(roomInfo.seed_name);
	if (roomSeed.IsEmpty())
		roomSeed = seedName;
	else if (roomSeed != seedName)
		AbortGame(LOCTEXT("RoomSeedMissmatch", "Room seed does not match seed of save, this save does not belong to the multiworld your connecting to"));

	hasLoadedRoomInfo = true;
}

void AApSubsystem::MonitorDataStoreValue(std::string key, AP_DataType dataType, std::string defaultValue, std::function<void(AP_SetReply)> callback) {
	callbacks[key] = callback;

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
}

void AApSubsystem::SetServerData(AP_SetServerDataRequest* setDataRequest) {
	AP_SetServerData(setDataRequest);
}

// Called every frame
void AApSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (ConnectionState != EApConnectionState::Connected)
		return;

	ReceiveItems();

	HandleAPMessages();
	
	if (!hasSentGoal) {
		hasSentGoal = goalSubsystem->AreGoalsCompleted(&slotData);
		if (hasSentGoal) {
			UE_LOG(LogApSubsystem, Display, TEXT("Sending goal completion to server"));
			AP_StoryComplete();
		}
	}
}

void AApSubsystem::ReceiveItems()
{
	// Consider processing only one queue item per tick for performance reasons
	TTuple<int64, bool> item;
	while (ReceivedItems.Dequeue(item)) {
		int64 itemid = item.Key;
		bool isFromServer = item.Value;

		if (ItemSchematics.Contains(itemid)) {
			SManager->GiveAccessToSchematic(ItemSchematics[itemid], nullptr);
		} else if (auto trapName = UApMappings::ItemIdToTrap.Find(itemid)) {
			trapSubsystem->SpawnTrap(*trapName, nullptr);
		} else if (mappingSubsystem->ApItems.Contains(itemid)) {
			if (mappingSubsystem->ApItems[itemid]->Type == EItemType::Item) {
				if (isFromServer) {
					//send directly to inventory

				} else {
					//send to portal
					TSubclassOf<UFGItemDescriptor> itemClass = StaticCastSharedRef<FApItem>(mappingSubsystem->ApItems[itemid])->Class;
					int stackSize = UFGItemDescriptor::GetStackSize(itemClass);
					portalSubsystem->Enqueue(itemClass, stackSize);
				}
			} else if (mappingSubsystem->ApItems[itemid]->Type == EItemType::Schematic) {
				SManager->GiveAccessToSchematic(StaticCastSharedRef<FApSchematicItem>(mappingSubsystem->ApItems[itemid])->Class, nullptr);
			} else if (mappingSubsystem->ApItems[itemid]->Type == EItemType::Specail) {
				//TODO: find out how to award inventroy slots...
			}
		}
	}
}

void AApSubsystem::CheckConnectionState() {
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

	TMap<FString, TSubclassOf<UFGSchematic>> schematicsPerMilestone = TMap<FString, TSubclassOf<UFGSchematic>>();

	for (auto& location : scoutedLocations) {
		if (location.locationName.StartsWith("Hub")) {
			FString milestone = location.locationName.Left(location.locationName.Find(","));
			FString uniqueMilestone = milestone + TEXT("_") + roomSeed;

			if (!schematicsPerMilestone.Contains(milestone)) {
				TSubclassOf<UFGSchematic> schematic = UApUtils::FindOrCreateClass(TEXT("/Archipelago/"), *uniqueMilestone, UFGSchematic::StaticClass());
				schematicsPerMilestone.Add(milestone, schematic);
			}

			if (!locationsPerMilestone.Contains(schematicsPerMilestone[milestone])) {
				locationsPerMilestone.Add(schematicsPerMilestone[milestone], TArray<FApNetworkItem>{ location });
			} else {
				locationsPerMilestone[schematicsPerMilestone[milestone]].Add(location);
			}
		}
	}

	UE_LOG(LogApSubsystem, Display, TEXT("Generating HUB milestones"));

	for (auto& itemPerMilestone : locationsPerMilestone) {
		FString schematicName;
		for (auto schematicAndName : schematicsPerMilestone) {
			if (itemPerMilestone.Key == schematicAndName.Value) {
				schematicName = schematicAndName.Key;
				break;
			}
		}

		CreateHubSchematic(schematicName, itemPerMilestone.Key, itemPerMilestone.Value);
	}

	areRecipiesAndSchematicsInitialized = true;
}

void AApSubsystem::CreateSchematicBoundToItemId(int64 itemid, TSharedRef<FApRecipeItem> apitem) {
	TArray<FString> recipesToUnlock;
	for (FApRecipeInfo recipe : apitem->Recipes) {
		recipesToUnlock.Add(recipe.Class->GetName());
	}

	FString recipes = FString::Join(recipesToUnlock, TEXT("\", \""));
	FString name = FString::Printf(TEXT("AP_ItemId_%i_%s"), itemid, *roomSeed);
	// https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_Schematic.json
	FString json = FString::Printf(TEXT(R"({
		"Name": "%s",
		"Type": "Custom",
		"Recipes": [ "%s" ]
	})"), *name, *recipes);

	FContentLib_Schematic schematic = UCLSchematicBPFLib::GenerateCLSchematicFromString(json);
	TSubclassOf<UFGSchematic> factorySchematic = UApUtils::FindOrCreateClass(TEXT("/Archipelago/"), *name, UFGSchematic::StaticClass());
	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, contentLibSubsystem);
	
	contentRegistry->RegisterSchematic(FName(TEXT("Archipelago")), factorySchematic);
	
	ItemSchematics.Add(itemid, factorySchematic);
}

void AApSubsystem::CreateHubSchematic(FString name, TSubclassOf<UFGSchematic> factorySchematic, TArray<FApNetworkItem> items) {
	int delimeterPos;
	name.FindChar('-', delimeterPos);
	int32 tier = FCString::Atoi(*name.Mid(delimeterPos - 1, 1));
	int32 milestone = FCString::Atoi(*name.Mid(delimeterPos + 1, 1));

	FString costs = "";
	for (auto& cost : slotData.hubLayout[tier - 1][milestone - 1]) {
		FString costJson = FString::Printf(TEXT(R"({
			"Item": "%s",
			"Amount": %i
		},)"), *cost.Key, cost.Value);

		costs += costJson;
	}

	// https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_Schematic.json
	FString json = FString::Printf(TEXT(R"({
		"Name": "%s",
		"Type": "Milestone",
		"Time": 200,
		"Tier": %i,
		"MenuPriority": %i,
		"VisualKit": "Kit_AP_Logo",
		"Cost": [ %s ]
	})"), *name, tier, milestone, *costs);

	FContentLib_Schematic schematic = UCLSchematicBPFLib::GenerateCLSchematicFromString(json);

	for (auto& item : items)
		schematic.InfoCards.Add(CreateUnlockInfoOnly(item));

	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, contentLibSubsystem);

	contentRegistry->RegisterSchematic(FName(TEXT("Archipelago")), factorySchematic);
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
			} else if (apItem->Type == EItemType::Specail) {
				//TODO: implement how we will display them
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
	for (TSubclassOf<UObject> buildingObject : buildings) {
		if (buildingObject->IsChildOf(AFGBuildable::StaticClass())) {
			AFGBuildable* building = Cast<AFGBuildable>(buildingObject.GetDefaultObject());
			if (building != nullptr) {
				if (building->IsA(AFGBuildableAutomatedWorkBench::StaticClass()))
					BuildingArray.Add("Workbench");
				else
					BuildingArray.Add(building->mDisplayName.ToString());
			}
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
	// TODO move "Item Bundle" to the name of the AP Item for clarity when hinting and similar
	// No not needed, item names are unique, "Plastic" is just that a stack of plastic, where "Recipe: Plastic" is the recipy to craft it
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalItemDescription", "This will give {ApPlayerName} Item Bundle: {ApItemName}. It can be collected by building an Archipelago Portal."), Args);
}

void AApSubsystem::UpdateInfoOnlyUnlockWithSchematicInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApSchematicItem> itemInfo) {
	UFGSchematic* schematic = itemInfo->Schematic;

	//infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
	//infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Item.Recipe_Icon_Item");
	// TODO move "Item Bundle" to the name of the AP Item for clarity when hinting and similar
	//infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalItemDescription", "This will give {ApPlayerName} Item Bundle: {ApItemName}. It can be collected by building an Archipelago Portal."), Args);
}

void AApSubsystem::UpdateInfoOnlyUnlockWithGenericApInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item) {
	infoCard->CategoryIcon = TEXT("/Archipelago/Assets/SourceArt/ArchipelagoAssetPack/ArchipelagoIconWhite128.ArchipelagoIconWhite128");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockDescription", "This will unlock {ApPlayerName} {ApItemName} which is considered a {ProgressionType}."), Args);

	if (item->flags == 0b001) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/AP-Purple.AP-Purple");
	} else if (item->flags == 0b010) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/AP-Blue.AP-Blue");
	} else if (item->flags == 0b100) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/AP-Red.AP-Red");
	} else {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/AP-Cyan.AP-Cyan");
	}
}

void AApSubsystem::HandleAPMessages() {
	for (int i = 0; i < 10; i++) {
		TPair<FString, FLinearColor> queuedMessage;
		if (ChatMessageQueue.Dequeue(queuedMessage)) {
			SendChatMessage(queuedMessage.Key, queuedMessage.Value);
		} else {
			if (!AP_IsMessagePending())
				return;

			AP_Message* message = AP_GetLatestMessage();
			SendChatMessage(UApUtils::FStr(message->text), FLinearColor::White);

			AP_ClearLatestMessage();
		}
	}
}

void AApSubsystem::SendChatMessage(const FString& Message, const FLinearColor& Color) {
	// TODO this does not replicate to multiplayer clients
	AFGChatManager* ChatManager = AFGChatManager::Get(GetWorld());
	if (!ChatManager) {
		UE_LOG(LogApChat, Error, TEXT("Too early to send chat message. Would have been: Archipelago Chat Message: %s"), *Message);
		return;
	}
	FChatMessageStruct MessageStruct;
	MessageStruct.MessageString = Message;
	MessageStruct.MessageType = EFGChatMessageType::CMT_SystemMessage;
	MessageStruct.ServerTimeStamp = GetWorld()->TimeSeconds;
	MessageStruct.CachedColor = Color;
	ChatManager->AddChatMessageToReceived(MessageStruct);

	UE_LOG(LogApChat, Display, TEXT("Archipelago Chat Message: %s"), *Message);
}

void AApSubsystem::ScoutArchipelagoItems() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::HintUnlockedHubRecipies()"));

	std::vector<int64> locations;

	int maxMilestones = 5;
	int maxSlots = 10;

	int64 hubBaseId = 1338000;

	for (int tier = 1; tier <= slotData.hubLayout.Num(); tier++)
	{
		for (int milestone = 1; milestone <= maxMilestones; milestone++)
		{
			for (int slot = 1; slot <= maxSlots; slot++)
			{
				if (milestone <= slotData.hubLayout[tier - 1].Num() && slot <= slotData.numberOfChecksPerMilestone)
					locations.push_back(hubBaseId);

				hubBaseId++;
			}
		}
	}

	AP_SendLocationScouts(locations, 0);

	hasScoutedLocations = true;
}

void AApSubsystem::TimeoutConnection() {
	ConnectionState = EApConnectionState::ConnectionFailed;
	ConnectionStateDescription = LOCTEXT("AuthFailed", "Authentication failed. Check your connection details and load the save again.");
	UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::TimeoutConnectionIfNotConnected(), Authenticated Failed"));

	SetActorTickEnabled(false);
}

FString AApSubsystem::GetApItemName(int64 id) {
	return UApUtils::FStr(AP_GetItemName("Satisfactory", id));
}

void AApSubsystem::SetGiftBoxState(bool open) {
	AP_UseGiftAutoReject(false);

	AP_GiftBoxProperties giftbox;
	giftbox.AcceptsAnyGift = true;
	giftbox.DesiredTraits = std::vector<std::string>();
	giftbox.IsOpen = open;

	AP_RequestStatus result = AP_SetGiftBoxProperties(giftbox);

	if (result != AP_RequestStatus::Done)
		UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::SetGiftBoxState(\"%s\") Updating giftbox metadata failed"), (open ? TEXT("true") : TEXT("false")));
}

TMap<FApPlayer, FApGiftBoxMetaData> AApSubsystem::GetAcceptedTraitsPerPlayer() {
	std::map<std::pair<int, std::string>, AP_GiftBoxProperties> giftboxes = AP_QueryGiftBoxes();

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

	AP_RequestStatus result = AP_SendGift(gift);

	if (result != AP_RequestStatus::Done)
		UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::SendGift({Name: \"%s\", Amount: %i}) Sending gift failed"), *giftToSend.ItemName, giftToSend.Amount);

	return result != AP_RequestStatus::Error;
}

TArray<FApReceiveGift> AApSubsystem::GetGifts() {
	std::vector<AP_Gift> gifts = AP_CheckGifts();

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

void AApSubsystem::RejectGift(FString id) {
	std::string giftId = TCHAR_TO_UTF8(*id);

	AP_RequestStatus result = AP_RejectGift(giftId);

	if (result != AP_RequestStatus::Done)
		UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::RejectGift(\"%s\") Rejecting gift failed"), *id);
}

void AApSubsystem::AcceptGift(FString id) {
	std::string giftId = TCHAR_TO_UTF8(*id);
	AP_Gift gift;

	AP_RequestStatus result = AP_AcceptGift(giftId, &gift);

	if (result != AP_RequestStatus::Done)
		UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::AcceptGift(\"%s\") Accepting gift failed"), *id);
}

TArray<FApPlayer> AApSubsystem::GetAllApPlayers() {
	std::vector<std::pair<int, std::string>> apPlayers = AP_GetAllPlayers();

	TArray<FApPlayer> players;

	for (std::pair<int, std::string> apPlayer : apPlayers) {
		FApPlayer player;
		player.Team = apPlayer.first;
		player.Name = UApUtils::FStr(apPlayer.second);

		players.Add(player);
	}

	return players;
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

void AApSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::PostLoadGame_Implementation(saveVersion: %i, gameVersion: %i)"), saveVersion, gameVersion);

	if (!saveSlotDataHubLayout.IsEmpty() && slotData.numberOfChecksPerMilestone > 0)
	{
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
}

void AApSubsystem::AbortGame(FText reason) {
	UWorld* world = GEngine->GameViewport->GetWorld();
	UApGameInstanceModule* gameInstance = UApUtils::GetGameInstanceModule(world);
	APlayerController* player = world->GetFirstPlayerController();

	gameInstance->YeetToMainMenu(player, reason);
}

#pragma optimize("", on)

#undef LOCTEXT_NAMESPACE
