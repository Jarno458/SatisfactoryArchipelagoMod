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
		
		//UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::BeginPlay(), %s"), x);
	});

	std::map<std::string, AP_DataType> keylist = { { "EnergyLink", AP_DataType::Raw } };
	AP_SetNotify(keylist);
}

void AApSubsystem::SetReply_Callback(AP_SetReply setReply) {

}


// Called every frame
void AApSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

