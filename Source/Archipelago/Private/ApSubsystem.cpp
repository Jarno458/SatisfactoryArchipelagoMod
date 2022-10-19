#include "ApSubsystem.h"

DEFINE_LOG_CATEGORY(ApSubsystem);

#pragma optimize("", off)

AApSubsystem::AApSubsystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f;
}

void AApSubsystem::BeginPlay()
{
	Super::BeginPlay();



	SManager = AFGSchematicManager::Get(GetWorld());
	RManager = AFGResearchManager::Get(GetWorld());

	RManager->ResearchCompletedDelegate.AddDynamic(this, &AApSubsystem::OnResearchCompleted);


}

void AApSubsystem::DispatchLifecycleEvent(ELifecyclePhase phase) {
	if (phase == ELifecyclePhase::POST_INITIALIZATION) {
		FApConfigurationStruct config = GetActiveConfig();

		if (!config.Enabled)
		{
			SetActorTickEnabled(false);
			return;
		}

		ConnectToArchipelago(config);

		TArray<TSubclassOf<UFGSchematic>> availableSchematics;
		TArray<TSubclassOf<UFGSchematic>> allSchematics;
		TArray<TSubclassOf<UFGSchematic>> milestones;

		SManager->GetAvailableSchematics(availableSchematics);
		SManager->GetAllSchematics(allSchematics);
		SManager->GetAllSchematicsOfType(ESchematicType::EST_Milestone, milestones);
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

	AP_Start();

	GetWorldTimerManager().SetTimer(connectionTimeoutHandler, this, &AApSubsystem::TimeoutConnectionIfNotConnected, 5.0f, false);
}

void AApSubsystem::OnResearchCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubSystem::OnResearchCompleted(schematic), Schematic Completed"));

	//if (schematic.) //if name is Archipelago #xxxx send check to server

}

void AApSubsystem::ItemClearCallback() {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::ItemClearCallback()"));

}

void AApSubsystem::ItemReceivedCallback(int id, bool notify) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::ItemReceivedCallback(%i, %s)"), id, (notify ? TEXT("true") : TEXT("false")));




	//map to Schematic and unlock it
}

void AApSubsystem::LocationCheckedCallback(int id) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::LocationCheckedCallback(%i)"), id);

}

void AApSubsystem::SetReplyCallback(AP_SetReply setReply) {
	if (callbacks.count(setReply.key))
		callbacks[setReply.key](setReply);
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

	if (!isInitialized && AP_GetConnectionStatus() == AP_ConnectionStatus::Authenticated) {
		isInitialized = true;

		UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::Tick(), Successfully Authenticated"));

		FApConfigurationStruct config = GetActiveConfig();

		FString message = FString::Printf(TEXT("Congratulation you somehow managed to connect to Archipelago server: \"%s\", for user \"%s\""), *config.Url, *config.Login);

		SendChatMessage(message, FLinearColor::Green);
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

std::map<std::string, std::function<void(AP_SetReply)>> AApSubsystem::callbacks;
TMap<long long, std::string> ItemIdToSchematicName = {
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

#pragma optimize("", on)

//#include "CLCDOBPFLib.h"
/*FString changeBaseClassJson = TEXT(R"({
	"$schema": "https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_CDO.json",
	"Class" : "/Game/FactoryGame/Buildable/Factory/PowerStorage/Build_PowerStorageMk1.Build_PowerStorageMk1",
	"Edits" : [
		{
			"Property": "Parent",
			"Value" : "/Archipelago/Buildables/EnergyLinkBuildable.EnergyLinkBuildable"
		}
	]
})");

if (UCLCDOBPFLib::GenerateCLCDOFromString(changeBaseClassJson, true)) {
	UE_LOG(ApSubsystem, Display, TEXT("AEnergyLinkSubsystem:BeginPlay(), Patched BuildEnergyLink baseclass"));
}
else
{
	UE_LOG(ApSubsystem, Error, TEXT("AEnergyLinkSubsystem:BeginPlay(), Failed to patch BuildEnergyLink baseclass"));
}*/