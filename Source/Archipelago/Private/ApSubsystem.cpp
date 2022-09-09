#include "ApSubsystem.h"

DEFINE_LOG_CATEGORY(ApSubsystem);

// Sets default values
AApSubsystem::AApSubsystem() : Super()
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

	AP_SetItemClearCallback([](){});
	AP_SetItemRecvCallback([](int id, bool notify) {
	});
	AP_SetLocationCheckedCallback([](int id) {
	});

	AP_Start();

	AP_RegisterSetReplyCallback([](AP_SetReply setReply) {


		std::string x = setReply.key;

		FString json(x.c_str());

		UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::AP_RegisterSetReplyCallback(setReply), %s"), *json);
	});

	std::map<std::string, AP_DataType> keylist = { { "EnergyLink", AP_DataType::Raw } };
	AP_SetNotify(keylist);
}

void AApSubsystem::MonitorDataStoreValue(std::string key, AP_DataType dataType, void (*callback)(AP_SetReply)) {
	//TODO have a mapping for callbacks per key

	AP_RegisterSetReplyCallback([](AP_SetReply setReply) {
		//if (key == setReply.key)
		//	callback(setReply);
	});

	std::map<std::string, AP_DataType> keylist = { { key, dataType } };
	AP_SetNotify(keylist);

	AP_SetServerDataRequest setDefaultAndRecieceUpdate;
	setDefaultAndRecieceUpdate.key = key;

	AP_DataStorageOperation setDefault;
	setDefault.operation = "default";
	setDefault.value = 0;

	std::vector<AP_DataStorageOperation> operations;
	operations.push_back(setDefault);

	setDefaultAndRecieceUpdate.operations = operations;
	setDefaultAndRecieceUpdate.default_value = 0;
	setDefaultAndRecieceUpdate.type = dataType;
	setDefaultAndRecieceUpdate.want_reply = true;

	AP_SetServerData(&setDefaultAndRecieceUpdate);
}

// Called every frame
void AApSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}