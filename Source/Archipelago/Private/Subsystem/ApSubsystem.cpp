#include "Subsystem/ApSubsystem.h"
#include "ApUtils.h"
#include "Data/ApItemIdMapping.h"

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

void AApSubsystem::BeginPlay() {
	Super::BeginPlay();

	UWorld* world = GetWorld();
	RManager = AFGResearchManager::Get(world);
	SManager = AFGSchematicManager::Get(world);
	PManager  = AFGGamePhaseManager::Get(world);
	resourceSinkSubsystem = AFGResourceSinkSubsystem::Get(world);
	
	RManager->ResearchCompletedDelegate.AddDynamic(this, &AApSubsystem::OnMamResearchCompleted);
	SManager->PurchasedSchematicDelegate.AddDynamic(this, &AApSubsystem::OnSchematicCompleted);
}

void AApSubsystem::DispatchLifecycleEvent(ELifecyclePhase phase) {
	FApConfigurationStruct config = GetActiveConfig();
	UE_LOG(LogApSubsystem, Warning, TEXT("Archipelago manually disabled by user config"));
	if (!config.Enabled) {
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
		contentRegistry = AModContentRegistry::Get(GetWorld());
		check(contentRegistry)

		UE_LOG(LogApSubsystem, Display, TEXT("Initiating Archipelago server connection in background..."));
		ConnectToArchipelago(config);

		UE_LOG(LogApSubsystem, Display, TEXT("Generating schematics from AP Item IDs..."));
		for (auto& item : UApItemIdMapping::ItemIdToGameRecipe)
			CreateSchematicBoundToItemId(item.Key);
		for (auto& item : UApItemIdMapping::ItemIdToGameBuilding)
			CreateSchematicBoundToItemId(item.Key);

		auto trapSystem = AApTrapSubsystem::Get();

			
		FDateTime connectingStartedTime = FDateTime::Now();
		FGenericPlatformProcess::ConditionalSleep([this, config, connectingStartedTime]() { return InitializeTick(config, connectingStartedTime); }, 0.5);
	} else if (phase == ELifecyclePhase::POST_INITIALIZATION) {
		if (ConnectionState != EApConnectionState::Connected) {
			FString message = FString::Printf(TEXT("Failed to connect to Archipelago server: \"%s\", for user \"%s\""), *config.Url, *config.Login);

			ChatMessageQueue.Enqueue(TPair<FString, FLinearColor>(message, FLinearColor::Red));
		}
	}
}

bool AApSubsystem::InitializeTick(FApConfigurationStruct config, FDateTime connectingStartedTime) {
	if (ConnectionState == EApConnectionState::Connecting) {
		if ((FDateTime::Now() - connectingStartedTime).GetSeconds() > 10)
			TimeoutConnection();
		else
			CheckConnectionState(config);
	} else if (ConnectionState == EApConnectionState::Connected) {
		if (!shouldParseItemsToScout) {
			if (slotData.hasLoadedSlotData) {
				HintUnlockedHubRecipies();
			} else {
				// awaiting slot data callback
			}
		} else {
			ParseScoutedItems();
			return true;
		}
	}

	return ConnectionState == EApConnectionState::ConnectionFailed;
}

void AApSubsystem::ConnectToArchipelago(FApConfigurationStruct config) {
	std::string const uri = TCHAR_TO_UTF8(*config.Url);
	std::string const user = TCHAR_TO_UTF8(*config.Login);
	std::string const password = TCHAR_TO_UTF8(*config.Password);

	AP_Init(uri.c_str(), "Satisfactory", user.c_str(), password.c_str());

	AP_SetItemClearCallback(AApSubsystem::ItemClearCallback);
	AP_SetItemRecvCallback(AApSubsystem::ItemReceivedCallback);
	AP_SetLocationCheckedCallback(AApSubsystem::LocationCheckedCallback);
	AP_RegisterSetReplyCallback(AApSubsystem::SetReplyCallback);
	AP_SetLocationInfoCallback(AApSubsystem::LocationScoutedCallback);
	AP_RegisterSlotDataRawCallback("Data", AApSubsystem::ParseSlotData);

	ConnectionState = EApConnectionState::Connecting;
	ConnectionStateDescription = LOCTEXT("Connecting", "Connecting...");

	AP_Start();
}

void AApSubsystem::OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubSystem::OnResearchCompleted(schematic), MAM Research Completed"));

	//if (schematic.) //if name is Archipelago #xxxx send check to server
}


void AApSubsystem::OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubSystem::OnSchematicCompleted(schematic)"));

	ESchematicType type = UFGSchematic::GetType(schematic);

	if (type != ESchematicType::EST_Milestone || !locationsPerMilestone.Contains(schematic))
		return;

	for (auto& location : locationsPerMilestone[schematic])
		AP_SendItem(location.location);
}

void AApSubsystem::ItemClearCallback() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ItemClearCallback()"));

}

void AApSubsystem::ItemReceivedCallback(int64_t item, bool notify) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ItemReceivedCallback(%i, %s)"), item, (notify ? TEXT("true") : TEXT("false")));

	AApSubsystem* self = AApSubsystem::Get();
	self->ReceivedItems.Enqueue(item);
}

void AApSubsystem::LocationCheckedCallback(int64_t id) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::LocationCheckedCallback(%i)"), id);

}

void AApSubsystem::SetReplyCallback(AP_SetReply setReply) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::SetReplyCallback(%s)"), *UApUtils::FStr(setReply.key));

	if (callbacks.count(setReply.key))
		callbacks[setReply.key](setReply);
}

void AApSubsystem::LocationScoutedCallback(std::vector<AP_NetworkItem> scoutedLocations) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::HintUnlockedHubRecipies(vector[%i])"), scoutedLocations.size());

	AApSubsystem* self = AApSubsystem::Get();

	self->scoutedLocations = TArray<AP_NetworkItem>();

	for (auto location : scoutedLocations)
		self->scoutedLocations.Add(location);
	
	self->shouldParseItemsToScout = true;
}

void AApSubsystem::ParseSlotData(std::string json) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ParseSlotData(%s)"), *UApUtils::FStr(json));

	AApSubsystem* self = AApSubsystem::Get();
	bool success = FApSlotData::ParseSlotData(json, &self->slotData);
	if (!success) {
		FString jsonString(json.c_str());
		UE_LOG(LogApSubsystem, Fatal, TEXT("Archipelago SlotData Invalid! %s"), *jsonString);
		// TODO kick people out to the main menu screen or something, this keeps them hanging forever on the loading screen with no clear indication
		// Switched to Fatal for now so it closes the game, but there must be a better way
	}
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

TEnumAsByte<EApConnectionState> AApSubsystem::GetConnectionState() {
	return TEnumAsByte<EApConnectionState>(ConnectionState);
}

// Called every frame
void AApSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (ConnectionState != EApConnectionState::Connected)
		return;

	// Consider processing only one queue item per tick for performance reasons
	int64_t item;
	while (ReceivedItems.Dequeue(item)) {
		if (ItemSchematics.Contains(item)) {
			SManager->GiveAccessToSchematic(ItemSchematics[item], nullptr);
		} else if (UApItemIdMapping::ItemIdToGameItemDescriptor.Contains(item)) {
			TMap<FName, FAssetData> itemDescriptorAssets = UApUtils::GetBlueprintAssetsIn("/Game/FactoryGame/Resource", TArray<FString>{ "Desc_", "BP_" });
			itemDescriptorAssets.Append(UApUtils::GetBlueprintAssetsIn("/Game/FactoryGame/Equipment", TArray<FString>{ "Desc_", "BP_" }));

			UFGItemDescriptor* itemDescriptor = GetItemDescriptorByName(itemDescriptorAssets, UApItemIdMapping::ItemIdToGameItemDescriptor[item]);

			PortalItems.Enqueue(itemDescriptor->GetClass());
		} else if (auto trapName = UApItemIdMapping::ItemIdToTrap.Find(item)) {
			AApTrapSubsystem::Get()->SpawnTrap(*trapName, nullptr);
		}
	}

	HandleAPMessages();

	if (!hasSentGoal) {
		const bool spElevatorGoalReached = slotData.finalSpaceElevatorTier > 0 && PManager->GetGamePhase() >= slotData.finalSpaceElevatorTier;
		const bool sinkPointsGoalReached = slotData.finalResourceSinkPoints > 0 && resourceSinkSubsystem->GetNumTotalPoints(EResourceSinkTrack::RST_Default) >= slotData.finalResourceSinkPoints;
		if (spElevatorGoalReached || sinkPointsGoalReached) {
			UE_LOG(LogApSubsystem, Display, TEXT("Sending goal completion to server"));
			AP_StoryComplete();
			hasSentGoal = true;
		}
	}
}

void AApSubsystem::CheckConnectionState(FApConfigurationStruct config) {
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
			FString message = FString::Printf(TEXT("Failed to connect to Archipelago server: \"%s\", for user \"%s\""), *config.Url, *config.Login);

			ChatMessageQueue.Enqueue(TPair<FString, FLinearColor>(message, FLinearColor::Red));
		}
	}
}

void AApSubsystem::ParseScoutedItems() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ParseScoutedItems(vector[%i])"), scoutedLocations.Num());

	TMap<FString, TSubclassOf<UFGSchematic>> schematicsPerMilestone = TMap<FString, TSubclassOf<UFGSchematic>>();

	for (auto& apLocation : scoutedLocations) {
		if (apLocation.locationName.starts_with("Hub")) {
			std::string milestoneString = apLocation.locationName.substr(0, apLocation.locationName.find(","));
			FString milestone = UApUtils::FStr(milestoneString);

			if (!schematicsPerMilestone.Contains(milestone)) {
				TSubclassOf<UFGSchematic> schematic = UApUtils::FindOrCreateClass(TEXT("/Archipelago/"), *milestone, UFGSchematic::StaticClass());
				schematicsPerMilestone.Add(milestone, schematic);
			}

			if (!locationsPerMilestone.Contains(schematicsPerMilestone[milestone])) {
				locationsPerMilestone.Add(schematicsPerMilestone[milestone], TArray<AP_NetworkItem>{ apLocation });
			} else {
				locationsPerMilestone[schematicsPerMilestone[milestone]].Add(apLocation);
			}
		} else if (apLocation.locationName.starts_with("Mam")) {
			UE_LOG(LogApSubsystem, Verbose, TEXT("AP Location %s"), *UApUtils::FStr(apLocation.locationName));
		}
	}

	UE_LOG(LogApSubsystem, Display, TEXT("Generating HUB milestones"));

	TMap<FName, FAssetData> recipeAssets = UApUtils::GetBlueprintAssetsIn("/Game/FactoryGame/Recipes", TArray<FString>{ "Recipe_" });
	recipeAssets.Append(UApUtils::GetBlueprintAssetsIn("/Game/FactoryGame/Equipment", TArray<FString>{ "Recipe_" }));
	
	TMap<FName, FAssetData> itemDescriptorAssets = UApUtils::GetBlueprintAssetsIn("/Game/FactoryGame/Resource", TArray<FString>{ "Desc_", "BP_" });
	itemDescriptorAssets.Append(UApUtils::GetBlueprintAssetsIn("/Game/FactoryGame/Equipment", TArray<FString>{ "Desc_", "BP_" }));
	// BP_WAT1 and BP_WAT2 (alien artifacts)
	itemDescriptorAssets.Append(UApUtils::GetBlueprintAssetsIn("/Game/FactoryGame/Prototype", TArray<FString>{ "Desc_", "BP_" }));

	for (auto& itemPerMilestone : locationsPerMilestone) {
		FString schematicName;
		for (auto schematicAndName : schematicsPerMilestone) {
			if (itemPerMilestone.Key == schematicAndName.Value) {
				schematicName = schematicAndName.Key;
				break;
			}
		}

		CreateHubSchematic(recipeAssets, itemDescriptorAssets, schematicName, itemPerMilestone.Key, itemPerMilestone.Value);
	}

	scoutedLocations.Empty();
	shouldParseItemsToScout = false;
}

void AApSubsystem::CreateSchematicBoundToItemId(int64_t item) {
	FString recipy = UApItemIdMapping::ItemIdToGameBuilding.Contains(item) ? UApItemIdMapping::ItemIdToGameBuilding[item] : UApItemIdMapping::ItemIdToGameRecipe[item];
	FString name = UApUtils::FStr("AP_ItemId_" + std::to_string(item));
	// https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_Schematic.json
	FString json = FString::Printf(TEXT(R"({
		"Name": "%s",
		"Type": "Custom",
		"Recipes": [ "%s" ]
	})"), *name, *recipy);

	FContentLib_Schematic schematic = UCLSchematicBPFLib::GenerateCLSchematicFromString(json);
	TSubclassOf<UFGSchematic> factorySchematic = UApUtils::FindOrCreateClass(TEXT("/Archipelago/"), *name, UFGSchematic::StaticClass());
	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, contentLibSubsystem);

	contentRegistry->RegisterSchematic(FName(TEXT("Archipelago")), factorySchematic);

	ItemSchematics.Add(item, factorySchematic);
}

void AApSubsystem::CreateRecipe(AP_NetworkItem item) {
	FString name(("AP_ITEM_RECIPE_" + item.playerName + " - " + item.itemName).c_str());
	FString uniqueId = UApUtils::FStr(item.location);
	// https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_Recipe.json
	FString json = FString::Printf(TEXT(R"({
		 "Name": "%s",
		 "Ingredients": [],
		 "Products": [
			  {
					"Item": "AP_Logo_Item",
					"Amount": 1
			  }
		 ],
		 "ManufacturingDuration": 1,
		 "ProducedIn": [
			  "Build_HadronCollider"
		 ]
	})"), *name);

	FContentLib_Recipe clRecipy = UCLRecipeBPFLib::GenerateCLRecipeFromString(json);
	TSubclassOf<UFGRecipe> factoryRecipy = UApUtils::FindOrCreateClass(TEXT("/Archipelago/"), *uniqueId, UFGRecipe::StaticClass());
	UCLRecipeBPFLib::InitRecipeFromStruct(contentLibSubsystem, clRecipy, factoryRecipy);

	contentRegistry->RegisterRecipe(FName(TEXT("Archipelago")), factoryRecipy);
}

void AApSubsystem::CreateDescriptor(AP_NetworkItem item) {
	FString name(("AP_ITEM_DESC_" + item.playerName + " " + item.itemName).c_str());
	FString uniqueId = UApUtils::FStr(item.location);
	// https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_Item.json
	FString json = FString::Printf(TEXT(R"({
		 "Name": "%s",
		 "Description": "TODO: Implement",
		 "StackSize": "One",
		 "Category": "AP",
		 "VisualKit": "Kit_AP_Logo",
		 "NameShort": "APITM",
		 "CanBeDiscarded": false,
		 "RememberPickUp": false,
		 "EnergyValue": 0,
		 "RadioactiveDecay": 0,
		 "ResourceSinkPoints": 0
	})"), *name);

	FContentLib_Item clItem = UCLItemBPFLib::GenerateCLItemFromString(json);
	TSubclassOf<UFGItemDescriptor> factoryItem = UApUtils::FindOrCreateClass(TEXT("/Archipelago/"), *uniqueId, UFGItemDescriptor::StaticClass());
	UCLItemBPFLib::InitItemFromStruct(factoryItem, clItem, contentLibSubsystem);

	//contentRegistry->RegisterItem(FName(TEXT("Archipelago")), factoryItem); //no idea how/where to register items
}

void AApSubsystem::CreateHubSchematic(TMap<FName, FAssetData> recipeAssets, TMap<FName, FAssetData> itemDescriptorAssets, FString name, TSubclassOf<UFGSchematic> factorySchematic, TArray<AP_NetworkItem> items) {
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
		schematic.InfoCards.Add(CreateUnlockInfoOnly(recipeAssets, itemDescriptorAssets, item));

	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, contentLibSubsystem);

	contentRegistry->RegisterSchematic(FName(TEXT("Archipelago")), factorySchematic);
}

FContentLib_UnlockInfoOnly AApSubsystem::CreateUnlockInfoOnly(TMap<FName, FAssetData> recipeAssets, TMap<FName, FAssetData> itemDescriptorAssets, AP_NetworkItem item) {
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

	Args.Add(TEXT("ApItemName"), UApUtils::FText(item.itemName));

	FContentLib_UnlockInfoOnly infoCard;

	if (item.player == slotData.currentPlayerSlot) {
		Args.Add(TEXT("ApPlayerName"), FText::FromString(TEXT("your")));

		infoCard.mUnlockName = UApUtils::FText(item.itemName);

		if (UApItemIdMapping::ItemIdToGameBuilding.Contains(item.item)) {
			UpdateInfoOnlyUnlockWithBuildingInfo(&infoCard, Args, recipeAssets, &item);
		} else if (UApItemIdMapping::ItemIdToGameRecipe.Contains(item.item)) {
			UpdateInfoOnlyUnlockWithRecipeInfo(&infoCard, Args, recipeAssets, &item);
		} else if (UApItemIdMapping::ItemIdToGameItemDescriptor.Contains(item.item)) {
			UpdateInfoOnlyUnlockWithItemInfo(&infoCard, Args, itemDescriptorAssets, &item);
		} else {
			UpdateInfoOnlyUnlockWithGenericApInfo(&infoCard, Args, &item);
		}
	} else {
		Args.Add(TEXT("ApPlayerName"), UApUtils::FText(item.playerName + "'s"));

		infoCard.mUnlockName = FText::Format(LOCTEXT("NetworkItemUnlockDisplayName", "{ApPlayerName} {ApItemName}"), Args);

		UpdateInfoOnlyUnlockWithGenericApInfo(&infoCard, Args, &item);
	}

	return infoCard;
}

void AApSubsystem::UpdateInfoOnlyUnlockWithBuildingInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, TMap<FName, FAssetData> buildingRecipyAssets, AP_NetworkItem* item) {
	UFGRecipe* recipe = GetRecipeByName(buildingRecipyAssets, UApItemIdMapping::ItemIdToGameBuilding[item->item]);
	UFGItemDescriptor* itemDescriptor = recipe->GetProducts()[0].ItemClass.GetDefaultObject();

	infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
	infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Building.Recipe_Icon_Building");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalBuildingDescription", "This will unlock your {ApItemName}"), Args);
}

void AApSubsystem::UpdateInfoOnlyUnlockWithRecipeInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, TMap<FName, FAssetData> recipeAssets, AP_NetworkItem* item) {
	UFGRecipe* recipe = GetRecipeByName(recipeAssets, UApItemIdMapping::ItemIdToGameRecipe[item->item]);
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

void AApSubsystem::UpdateInfoOnlyUnlockWithItemInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, TMap<FName, FAssetData> itemDescriptorAssets, AP_NetworkItem* item) {
	UFGItemDescriptor* itemDescriptor = GetItemDescriptorByName(itemDescriptorAssets, UApItemIdMapping::ItemIdToGameItemDescriptor[item->item]);

	infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
	infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Item.Recipe_Icon_Item");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalItemDescription", "This will give {ApPlayerName} {ApItemName}"), Args);
}

void AApSubsystem::UpdateInfoOnlyUnlockWithGenericApInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, AP_NetworkItem* item) {
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
	FChatMessageStruct MessageStruct;
	MessageStruct.MessageString = Message;
	MessageStruct.MessageType = EFGChatMessageType::CMT_SystemMessage;
	MessageStruct.ServerTimeStamp = GetWorld()->TimeSeconds;
	MessageStruct.CachedColor = Color;
	ChatManager->AddChatMessageToReceived(MessageStruct);

	UE_LOG(LogApChat, Display, TEXT("Archipelago Chat Message: %s"), *Message);
}

void AApSubsystem::HintUnlockedHubRecipies() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::HintUnlockedHubRecipies()"));

	std::vector<int64_t> locations;

	int maxMilestones = 5;
	int maxSlots = 10;

	int64_t hubBaseId = 1338000;

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
}

UFGRecipe* AApSubsystem::GetRecipeByName(TMap<FName, FAssetData> recipeAssets, FString name) {
	return Cast<UFGRecipe>(UApUtils::FindAssetByName(recipeAssets, name.Append("_C")));
}

UFGItemDescriptor* AApSubsystem::GetItemDescriptorByName(TMap<FName, FAssetData> itemDescriptorAssets, FString name) {
	UObject* obj = UApUtils::FindAssetByName(itemDescriptorAssets, name);
	verify(obj != nullptr);
	return Cast<UFGItemDescriptor>(obj);
}

void AApSubsystem::TimeoutConnection() {
	ConnectionState = EApConnectionState::ConnectionFailed;
	ConnectionStateDescription = LOCTEXT("AuthFailed", "Authentication failed. Check your connection details and load the save again.");
	UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::TimeoutConnectionIfNotConnected(), Authenticated Failed"));

	SetActorTickEnabled(false);
}

FApConfigurationStruct AApSubsystem::GetActiveConfig() {
	UConfigManager* ConfigManager = GEngine->GetEngineSubsystem<UConfigManager>();
	FConfigId ConfigId { "Archipelago", "" };
	auto Config = ConfigManager->GetConfigurationById(ConfigId);
	auto ConfigProperty = URuntimeBlueprintFunctionLibrary::GetModConfigurationPropertyByClass(Config);
	auto CPSection = Cast<UConfigPropertySection>(ConfigProperty);

	FApConfigurationStruct config;
	config.Enabled = Cast<UConfigPropertyBool>(CPSection->SectionProperties["Enabled"])->Value;
	config.Url = Cast<UConfigPropertyString>(CPSection->SectionProperties["Url"])->Value;
	config.Game = Cast<UConfigPropertyString>(CPSection->SectionProperties["Game"])->Value;
	config.Login = Cast<UConfigPropertyString>(CPSection->SectionProperties["Login"])->Value;
	config.Password = Cast<UConfigPropertyString>(CPSection->SectionProperties["Password"])->Value;

	return config;
}

#pragma optimize("", on)

#undef LOCTEXT_NAMESPACE
