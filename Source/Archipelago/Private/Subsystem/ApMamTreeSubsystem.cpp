#include "Subsystem/ApMamTreeSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "ApUtils.h"

DEFINE_LOG_CATEGORY(LogApMamTreeSubsystem);

//TODO REMOVE
#pragma optimize("", off)

AApMamTreeSubsystem::AApMamTreeSubsystem() : Super() {
	PrimaryActorTick.bCanEverTick = false;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

AApMamTreeSubsystem* AApMamTreeSubsystem::Get(UWorld* world) {
	return UApUtils::GetSubsystemActorIncludingParentClases<AApMamTreeSubsystem>(world);
}

void AApMamTreeSubsystem::Initialize() {
	UE_LOG(LogApMamTreeSubsystem, Display, TEXT("AApMamTreeSubsystem()::Initialize()"));
}

void AApMamTreeSubsystem::BeginPlay() {
	UE_LOG(LogApMamTreeSubsystem, Display, TEXT("AApMamTreeSubsystem::BeginPlay()"));
	Super::BeginPlay();

	UWorld* world = GetWorld();

	//RManager = AFGResearchManager::Get(world);
	//fgcheck(RManager);
	//AFGSchematicManager* SManager = AFGSchematicManager::Get(world);
	//fgcheck(SManager);

	//SManager->PurchasedSchematicDelegate.AddDynamic(this, &AApHardDriveGachaSubsystem::OnSchematicCompleted);
}

#pragma optimize("", on)

