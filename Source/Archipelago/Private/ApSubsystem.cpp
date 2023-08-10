#include "ApSubsystem.h"

DEFINE_LOG_CATEGORY(ApSubsystem);

//TODO REMOVE
#pragma optimize("", off)

std::map<std::string, std::function<void(AP_SetReply)>> AApSubsystem::callbacks;
UContentLibSubsystem* AApSubsystem::ContentLibSubsystem;
std::vector<AP_NetworkItem> AApSubsystem::ScoutedLocations;
bool AApSubsystem::ShouldParseItemsToScout = false;

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
		ContentLibSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UContentLibSubsystem>();
		check(ContentLibSubsystem)

		FApConfigurationStruct config = GetActiveConfig();

		if (!config.Enabled)
		{
			SetActorTickEnabled(false);
			return;
		}

		ConnectToArchipelago(config);
	
		FGenericPlatformProcess::ConditionalSleep([this, config]() { 
			if (isConnecting)
				CheckConnectionState(config);
			else
				ParseScoutedItems();

			return hasFininshedInitialization;
		}, 1);
	}
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
	
	isConnecting = true;

	AP_Start();

	GetWorldTimerManager().SetTimer(connectionTimeoutHandler, this, &AApSubsystem::TimeoutConnectionIfNotConnected, 5.0f, false);
}

void AApSubsystem::OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubSystem::OnResearchCompleted(schematic), Mam Research Completed"));

	//if (schematic.) //if name is Archipelago #xxxx send check to server
}


void AApSubsystem::OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubSystem::OnSchematicCompleted(schematic), Schematic Completed"));

	ESchematicType type = UFGSchematic::GetType(schematic);

	if (type != ESchematicType::EST_Milestone || !locationsPerMileStone.Contains(schematic))
		return;

	for (auto location : locationsPerMileStone[schematic])
		AP_SendItem(location);
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

	ScoutedLocations = scoutedLocations;
	ShouldParseItemsToScout = true;
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

	if (!isInitialized)
		return;

	HandleAPMessages();
}

void AApSubsystem::CheckConnectionState(FApConfigurationStruct config) {
	if (isConnecting) {
		AP_ConnectionStatus status = AP_GetConnectionStatus();

		if (status == AP_ConnectionStatus::Authenticated) {
			isInitialized = true;
			isConnecting = false;

			UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::Tick(), Successfully Authenticated"));

			HintUnlockedHubRecipies();
		}
		else if (status == AP_ConnectionStatus::ConnectionRefused) {
			isConnecting = false;

			FString message = FString::Printf(TEXT("Failed to connect to Archipelago server: \"%s\", for user \"%s\""), *config.Url, *config.Login);

			SendChatMessage(message, FLinearColor::Green);
		}
	}
}

void AApSubsystem::ParseScoutedItems() {
	if (!ShouldParseItemsToScout)
		return;

	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::ParseScoutedItems(vector[%i])"), ScoutedLocations.size());

	AModContentRegistry* contentRegistry = AModContentRegistry::Get(GetWorld());

	std::map<std::string, std::vector<AP_NetworkItem>> itmesPerMilestone;

	for (auto& item : ScoutedLocations) {
		if (item.locationName.starts_with("Hub"))
		{
			std::string milestoneString = item.locationName.substr(0, item.locationName.find(","));

			if (!itmesPerMilestone.contains(milestoneString)) {
				itmesPerMilestone.insert(std::pair<std::string, std::vector<AP_NetworkItem>>(milestoneString, { item }));
			} else {
				itmesPerMilestone[milestoneString].push_back(item);
			}
		}
	}

	for (auto& itemPerMilestone : itmesPerMilestone) {
		for (auto& item : itemPerMilestone.second) {
			CreateRecipe(contentRegistry, item);
		}

		CreateHubSchematic(contentRegistry, itemPerMilestone.first, itemPerMilestone.second);
	}

	//UWorld* world = GEngine->GameViewport->GetWorld();

	ScoutedLocations.clear();
	ShouldParseItemsToScout = false;
	hasFininshedInitialization = true;
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
	UCLRecipeBPFLib::InitRecipeFromStruct(ContentLibSubsystem, clRecipy, factoryRecipy);

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
	UCLItemBPFLib::InitItemFromStruct(factoryItem, clItem, ContentLibSubsystem);

	//contentRegistry->RegisterItem(FName(TEXT("Archipelago")), factoryItem);
}

void AApSubsystem::CreateHubSchematic(AModContentRegistry* contentRegistry, std::string milestoneName, std::vector<AP_NetworkItem> items) {
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

	int delimierPos = milestoneName.find("-");
	std::string tierString = milestoneName.substr(delimierPos - 1, 1);

	FString name(milestoneName.c_str());
	FString tier(tierString.c_str());
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

	TSubclassOf<UFGSchematic> factorySchematic = FClassGenerator::GenerateSimpleClass(TEXT("/Archipelago/"), *name, UFGSchematic::StaticClass());

	locationsPerMileStone.Add(factorySchematic, TArray<int64_t>());
	for (auto& item : items) {
		locationsPerMileStone[factorySchematic].Add(item.location);
	}

	FContentLib_Schematic schematic = UCLSchematicBPFLib::GenerateCLSchematicFromString(json);
	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, ContentLibSubsystem);

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

	for (auto const& item : ItemIdToSchematicName) {
		locations.push_back(item.Key);
	}

	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::HintUnlockedHubRecipies() Scouting..."));
	AP_SendLocationScouts(locations, 0); //idally this we created a hint without spamming
}

void AApSubsystem::TimeoutConnectionIfNotConnected() {
	if (!isInitialized)
	{
		SetActorTickEnabled(false);

		UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::TimeoutConnectionIfNotConnected(), Authenticated Failed"));

		FApConfigurationStruct config = GetActiveConfig();

		FString message = FString::Printf(TEXT("Failed to connect to Archipelago server: \"%s\", for user \"%s\""), *config.Url, *config.Login);

		SendChatMessage(message, FLinearColor::Red);
	}
}

FApConfigurationStruct AApSubsystem::GetActiveConfig() {
	UConfigManager* ConfigManager = GEngine->GetEngineSubsystem<UConfigManager>();
	FConfigId ConfigId{ "Archipelago", "" };
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