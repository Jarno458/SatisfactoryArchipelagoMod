#include "Subsystem/ApGoalSubsystem.h"
#include "FGGamePhase.h"

AApGoalSubsystem::AApGoalSubsystem() : Super() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 1.0f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

AApGoalSubsystem* AApGoalSubsystem::Get(UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApGoalSubsystem>();
}

void AApGoalSubsystem::BeginPlay() {
	Super::BeginPlay();

	UWorld* world = GetWorld();

	phaseManager = AFGGamePhaseManager::Get(world);
	resourceSinkSubsystem = AFGResourceSinkSubsystem::Get(world);

	ap = AApSubsystem::Get(world);
	connectionInfoSubsystem = AApConnectionInfoSubsystem::Get(world);
	slotData = AApSlotDataSubsystem::Get(world);
}

void AApGoalSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (hasSentGoal 
		|| connectionInfoSubsystem->GetConnectionState() != EApConnectionState::Connected
		|| !slotData->HasLoadedSlotData())
			return;

	if (AreGoalsCompleted()) {
		UE_LOG(LogApSubsystem, Display, TEXT("Sending goal completion to server"));

		ap->MarkGameAsDone();

		hasSentGoal = true;
	}
}

bool AApGoalSubsystem::AreGoalsCompleted() {
	if (slotData->RequireAllGoals())
		return CheckSpaceElevatorGoal() && CheckResourceSinkPointsGoal();
	else
		return CheckSpaceElevatorGoal() || CheckResourceSinkPointsGoal();
}

bool AApGoalSubsystem::CheckSpaceElevatorGoal() {
	return slotData->IsSpaceElevatorGoalEnabled()
		&& phaseManager->GetCurrentGamePhase()->mGamePhase >= slotData->GetFinalSpaceElevatorTier();
}

bool AApGoalSubsystem::CheckResourceSinkPointsGoal() {
	return slotData->IsResourceSinkGoalEnabled()
		&&	resourceSinkSubsystem->GetNumTotalPoints(EResourceSinkTrack::RST_Default) >= slotData->GetFinalResourceSinkPoints();
}
