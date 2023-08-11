#include "ApSubsystem.h"

DEFINE_LOG_CATEGORY(ApSubsystem);

//TODO REMOVE
#pragma optimize("", off)

std::map<std::string, std::function<void(AP_SetReply)>> AApSubsystem::callbacks;

TMap<int64_t, std::string> AApSubsystem::ItemIdToSchematicName = {
	{1337500, "Schematic_HUB_Schematic_1-1" },
	{1337501, "Schematic_HUB_Schematic_1-2" },
	{1337502, "Schematic_HUB_Schematic_1-3" },
	{1337503, "Schematic_HUB_Schematic_1-4" },
	{1337504, "Schematic_HUB_Schematic_1-5" },
	{1337505, "Schematic_HUB_Schematic_2-1" },
	{1337506, "Schematic_HUB_Schematic_2-2" },
	{1337507, "Schematic_HUB_Schematic_2-3" },
	{1337508, "Schematic_HUB_Schematic_2-4" },
	{1337509, "Schematic_HUB_Schematic_2-5" }
};

AApSubsystem::AApSubsystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
}

AApSubsystem* AApSubsystem::Get() {
	return Get(GEngine->GameViewport->GetWorld());
}

AApSubsystem* AApSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	check(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApSubsystem>();
}

void AApSubsystem::BeginPlay()
{
	Super::BeginPlay();

	RManager = AFGResearchManager::Get(GetWorld());
	SManager = AFGSchematicManager::Get(GetWorld());

	RManager->ResearchCompletedDelegate.AddDynamic(this, &AApSubsystem::OnMamResearchCompleted);
	SManager->PurchasedSchematicDelegate.AddDynamic(this, &AApSubsystem::OnSchematicCompleted);
}

void AApSubsystem::DispatchLifecycleEvent(ELifecyclePhase phase) {
	if (phase == ELifecyclePhase::INITIALIZATION) {
		contentLibSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UContentLibSubsystem>();
		check(contentLibSubsystem)

		FApConfigurationStruct config = GetActiveConfig();

		if (!config.Enabled)	{
			SetActorTickEnabled(false);
			return;
		}

		ConnectToArchipelago(config);

		FDateTime connectingStartedTime = FDateTime::Now();
	
		FGenericPlatformProcess::ConditionalSleep([this, config, connectingStartedTime]() { return InitializeTick(config, connectingStartedTime); }, 1);
	}
	else if (phase == ELifecyclePhase::INITIALIZATION) {
		if (ConnectionState != EApConnectionState::Connected) {
			FApConfigurationStruct config = GetActiveConfig();

			FString message = FString::Printf(TEXT("Failed to connect to Archipelago server: \"%s\", for user \"%s\""), *config.Url, *config.Login);

			SendChatMessage(message, FLinearColor::Red);
		}
	}
}

bool AApSubsystem::InitializeTick(FApConfigurationStruct config, FDateTime connectingStartedTime) {
	if (ConnectionState == EApConnectionState::Connecting) {
		if ((FDateTime::Now() - connectingStartedTime).GetSeconds() > 5)
			TimeoutConnectionIfNotConnected();
		else
			CheckConnectionState(config);
	} else if (ConnectionState == EApConnectionState::Connected) {
		if (!shouldParseItemsToScout) {
			if (firstHubLocation != 0 && lastHubLocation != 0) {
				HintUnlockedHubRecipies();
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
	std::string const game = TCHAR_TO_UTF8(*config.Game);
	std::string const user = TCHAR_TO_UTF8(*config.Login);
	std::string const password = TCHAR_TO_UTF8(*config.Password);

	AP_Init(uri.c_str(), game.c_str(), user.c_str(), password.c_str());

	AP_SetItemClearCallback(AApSubsystem::ItemClearCallback);
	AP_SetItemRecvCallback(AApSubsystem::ItemReceivedCallback);
	AP_SetLocationCheckedCallback(AApSubsystem::LocationCheckedCallback);
	AP_RegisterSetReplyCallback(AApSubsystem::SetReplyCallback);
	AP_SetLocationInfoCallback(AApSubsystem::LocationScoutedCallback);
	AP_RegisterSlotDataIntCallback("FirstHubLocation", AApSubsystem::SlotDataFirstHubLocation);
	AP_RegisterSlotDataIntCallback("LastHubLocation", AApSubsystem::SlotDataLastHubLocation);

	ConnectionState = EApConnectionState::Connecting;

	AP_Start();
}

void AApSubsystem::OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubSystem::OnResearchCompleted(schematic), Mam Research Completed"));

	//if (schematic.) //if name is Archipelago #xxxx send check to server
}


void AApSubsystem::OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubSystem::OnSchematicCompleted(schematic)"));

	ESchematicType type = UFGSchematic::GetType(schematic);

	if (type != ESchematicType::EST_Milestone || !locationsPerMileStone.Contains(schematic))
		return;

	for (auto location : locationsPerMileStone[schematic])
		AP_SendItem(location.location);
}

void AApSubsystem::ItemClearCallback() {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::ItemClearCallback()"));

}

void AApSubsystem::ItemReceivedCallback(int64_t id, bool notify) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::ItemReceivedCallback(%i, %s)"), id, (notify ? TEXT("true") : TEXT("false")));




	//map to Schematic and unlock it
}

void AApSubsystem::LocationCheckedCallback(int64_t id) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::LocationCheckedCallback(%i)"), id);

}

void AApSubsystem::SetReplyCallback(AP_SetReply setReply) {
	FString fstringKey(setReply.key.c_str());
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::SetReplyCallback(%s)"), *fstringKey);

	if (callbacks.count(setReply.key))
		callbacks[setReply.key](setReply);
}

void AApSubsystem::LocationScoutedCallback(std::vector<AP_NetworkItem> scoutedLocations) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::HintUnlockedHubRecipies(vector[%i])"), scoutedLocations.size());

	AApSubsystem* self = AApSubsystem::Get();

	self->scoutedLocations = TArray<AP_NetworkItem>();

	for (auto location : scoutedLocations)
		self->scoutedLocations.Add(location);
	
	self->shouldParseItemsToScout = true;
}

void AApSubsystem::SlotDataFirstHubLocation(int locationId) {
	AApSubsystem* self = AApSubsystem::Get();

	self->firstHubLocation = locationId;
}

void AApSubsystem::SlotDataLastHubLocation(int locationId) {
	AApSubsystem* self = AApSubsystem::Get();

	self->lastHubLocation = locationId;
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
void AApSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ConnectionState != EApConnectionState::Connected)
		return;

	HandleAPMessages();
}

void AApSubsystem::CheckConnectionState(FApConfigurationStruct config) {
	if (ConnectionState == EApConnectionState::Connecting) {
		AP_ConnectionStatus status = AP_GetConnectionStatus();

		if (status == AP_ConnectionStatus::Authenticated) {
			ConnectionState = EApConnectionState::Connected;

			UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::Tick(), Successfully Authenticated"));
		}
		else if (status == AP_ConnectionStatus::ConnectionRefused) {
			ConnectionState = EApConnectionState::ConnectionFailed;

			FString message = FString::Printf(TEXT("Failed to connect to Archipelago server: \"%s\", for user \"%s\""), *config.Url, *config.Login);

			SendChatMessage(message, FLinearColor::Green);
		}
	}
}

void AApSubsystem::ParseScoutedItems() {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::ParseScoutedItems(vector[%i])"), scoutedLocations.Num());

	TMap<FString, TSubclassOf<UFGSchematic>> schematicsPerMilestone = TMap<FString, TSubclassOf<UFGSchematic>>();

	for (auto& location : scoutedLocations) {
		if (location.locationName.starts_with("Hub"))
		{
			std::string milestoneString = location.locationName.substr(0, location.locationName.find(","));
			FString milestone = FString(milestoneString.c_str());

			if (!schematicsPerMilestone.Contains(milestone)) {
				TSubclassOf<UFGSchematic> schematic = FClassGenerator::GenerateSimpleClass(TEXT("/Archipelago/"), *milestone, UFGSchematic::StaticClass());

				schematicsPerMilestone.Add(milestone, schematic);
			}

			if (!locationsPerMileStone.Contains(schematicsPerMilestone[milestone])) {
				locationsPerMileStone.Add(schematicsPerMilestone[milestone], TArray<AP_NetworkItem>{ location });
			} else {
				locationsPerMileStone[schematicsPerMilestone[milestone]].Add(location);
			}
		}
	}

	AModContentRegistry* contentRegistry = AModContentRegistry::Get(GetWorld());

	for (auto& itemPerMilestone : locationsPerMileStone) {
		for (auto& item : itemPerMilestone.Value) {
			CreateRecipe(contentRegistry, item);
		}

		FString schematicName;
		for (auto schematicAndName : schematicsPerMilestone)
		{
			if (itemPerMilestone.Key == schematicAndName.Value)
			{
				schematicName = schematicAndName.Key;
				break;
			}
		}

		CreateHubSchematic(contentRegistry, schematicName, itemPerMilestone.Key, itemPerMilestone.Value);
	}

	scoutedLocations.Empty();
	shouldParseItemsToScout = false;
}

void AApSubsystem::CreateRecipe(AModContentRegistry* contentRegistry, AP_NetworkItem item) {
	FString name((item.playerName + " - " + item.itemName).c_str());
	FString uniqueId(std::to_string(item.location).c_str());
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
	TSubclassOf<UFGRecipe> factoryRecipy = FClassGenerator::GenerateSimpleClass(TEXT("/Archipelago/"), *uniqueId, UFGRecipe::StaticClass());
	UCLRecipeBPFLib::InitRecipeFromStruct(contentLibSubsystem, clRecipy, factoryRecipy);

	contentRegistry->RegisterRecipe(FName(TEXT("Archipelago")), factoryRecipy);
}

void AApSubsystem::CreateItem(AModContentRegistry* contentRegistry, AP_NetworkItem item) {
	FString name((item.playerName + " " + item.itemName).c_str());
	FString uniqueId(std::to_string(item.location).c_str());
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
	TSubclassOf<UFGItemDescriptor> factoryItem = FClassGenerator::GenerateSimpleClass(TEXT("/Archipelago/"), *uniqueId, UFGItemDescriptor::StaticClass());
	UCLItemBPFLib::InitItemFromStruct(factoryItem, clItem, contentLibSubsystem);

	//contentRegistry->RegisterItem(FName(TEXT("Archipelago")), factoryItem); //no idea how/where to register items
}

void AApSubsystem::CreateHubSchematic(AModContentRegistry* contentRegistry, FString name, TSubclassOf<UFGSchematic> factorySchematic, TArray<AP_NetworkItem> items) {
	std::string buildRecipies = "";
	/*for (auto& item : items) {
		if (buildRecipies.length() > 0)
			buildRecipies = +"\", \"" + item.itemName;
		else
			buildRecipies = item.itemName;
	}*/

	/*for (auto& item : items) {
		if (buildRecipies.length() > 0)
			buildRecipies += "\", \"/Archipelago/" + std::to_string(item.location);
		else
			buildRecipies = "/Archipelago/" + std::to_string(item.location);
	}*/

	for (auto& item : items) {
		if (buildRecipies.length() > 0)
			buildRecipies += "\", \"" + std::to_string(item.location);
		else
			buildRecipies = std::to_string(item.location);
	}

	int delimeterPos;
	name.FindChar('-', delimeterPos);

	FString tier = name.RightChop(delimeterPos + 1);
	FString recipies(buildRecipies.c_str());
	// https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_Schematic.json
	FString json = FString::Printf(TEXT(R"({
		"Name": "%s",
		"Type": "Milestone",
		"Time": 100,
		"Tier": %s,
		"VisualKit": "Kit_AP_Logo",
		"Cost": [
			{
				"Item": "Desc_CopperSheet",
				"Amount": 1
			},
		],
		"Recipes": [ "%s" ]
	})"), *name, *tier, *recipies);

	FContentLib_Schematic schematic = UCLSchematicBPFLib::GenerateCLSchematicFromString(json);
	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, contentLibSubsystem);

	contentRegistry->RegisterSchematic(FName(TEXT("Archipelago")), factorySchematic);
}

void AApSubsystem::HandleAPMessages() {
	for (int i = 0; i < 10; i++)
	{
		if (!AP_IsMessagePending())
			return;

		AP_Message* message = AP_GetLatestMessage();
		FString fStringMessage(message->text.c_str());

		SendChatMessage(fStringMessage, FLinearColor::White);

		AP_ClearLatestMessage();
	}
}

void AApSubsystem::SendChatMessage(const FString& Message, const FLinearColor& Color) {
	AFGChatManager* ChatManager = AFGChatManager::Get(GetWorld());
	FChatMessageStruct MessageStruct;
	MessageStruct.MessageString = Message;
	MessageStruct.MessageType = EFGChatMessageType::CMT_SystemMessage;
	MessageStruct.ServerTimeStamp = GetWorld()->TimeSeconds;
	MessageStruct.CachedColor = Color;
	ChatManager->AddChatMessageToReceived(MessageStruct);
}

void AApSubsystem::HintUnlockedHubRecipies() {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::HintUnlockedHubRecipies()"));

	std::vector<int64_t> locations;

	for (int64_t i = firstHubLocation; i <= lastHubLocation; i++)
		locations.push_back(i);

	AP_SendLocationScouts(locations, 0);
}

void AApSubsystem::TimeoutConnectionIfNotConnected() {
	if (ConnectionState != EApConnectionState::Connecting)
		return;
	
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::TimeoutConnectionIfNotConnected(), Authenticated Failed"));

	SetActorTickEnabled(false);

	ConnectionState = EApConnectionState::ConnectionFailed;
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