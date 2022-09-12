#include "ApSubsystem.h"

DEFINE_LOG_CATEGORY(ApSubsystem);

// Sets default values
AApSubsystem::AApSubsystem()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f;
}

// Called when the game starts or when spawned
void AApSubsystem::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::BeginPlay()"));

	SManager = AFGSchematicManager::Get(GetWorld());
	RManager = AFGResearchManager::Get(GetWorld());

	AP_Init("localhost:38281", "Timespinner", "Jarno", "");

	AP_SetItemClearCallback(AApSubsystem::ItemClearCallback);
	AP_SetItemRecvCallback(AApSubsystem::ItemReceivedCallback);
	AP_SetLocationCheckedCallback(AApSubsystem::LocationCheckedCallback);
	AP_RegisterSetReplyCallback(AApSubsystem::SetReplyCallback);

	AP_Start();

	isInitialized = true;
}

void AApSubsystem::ItemClearCallback() {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::ItemClearCallback()"));

}

void AApSubsystem::ItemReceivedCallback(int id, bool notify) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::ItemReceivedCallback(%i, %s)"), id, (notify ? TEXT("true") : TEXT("false")));

}

void AApSubsystem::LocationCheckedCallback(int id) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::LocationCheckedCallback(%i)"), id);

}

void AApSubsystem::SetReplyCallback(AP_SetReply setReply) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::SetReplyCallback()"));

	if (callbacks.count(setReply.key))
		callbacks[setReply.key](setReply);
}

void AApSubsystem::MonitorDataStoreValue(std::string key, AP_DataType dataType, std::function<void(AP_SetReply)> callback) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::MonitorDataStoreValue()"));

	callbacks[key] = callback;

	std::map<std::string, AP_DataType> keylist = { { key, dataType } };
	AP_SetNotify(keylist);

	std::string defaultValue = "0";

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
}

std::map<std::string, std::function<void(AP_SetReply)>> AApSubsystem::callbacks;


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