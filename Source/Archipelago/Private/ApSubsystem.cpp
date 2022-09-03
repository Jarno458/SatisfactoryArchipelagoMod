#include "ApSubsystem.h"

DEFINE_LOG_CATEGORY(ApSubsystem);

// Sets default values
AApSubsystem::AApSubsystem() : Super()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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
}

// Called every frame
void AApSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

