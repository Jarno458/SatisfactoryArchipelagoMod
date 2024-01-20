#include "Subsystem/ApGoalSubsystem.h"

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
}

void AApGoalSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (hasSentGoal || ap->ConnectionState != EApConnectionState::Connected)
		return;

	FApSlotData slotData = ap->GetSlotData();

	if (AreGoalsCompleted(&slotData)) {
		UE_LOG(LogApSubsystem, Display, TEXT("Sending goal completion to server"));

		ap->MarkGameAsDone();

		hasSentGoal = true;
	}
}

bool AApGoalSubsystem::AreGoalsCompleted(const FApSlotData* slotData) {
	// TODO do we want to && goals and make them return True when they're disabled?
	// This would make it easy to have multiple goals selected at once
	return CheckSpaceElevatorGoal(slotData) || CheckResourceSinkPointsGoal(slotData);
}

bool AApGoalSubsystem::CheckSpaceElevatorGoal(const FApSlotData* slotData) {
	return slotData->finalSpaceElevatorTier > 0 && phaseManager->GetGamePhase() >= slotData->finalSpaceElevatorTier;
}

bool AApGoalSubsystem::CheckResourceSinkPointsGoal(const FApSlotData* slotData) {
	return slotData->finalResourceSinkPoints > 0 && resourceSinkSubsystem->GetNumTotalPoints(EResourceSinkTrack::RST_Default) >= slotData->finalResourceSinkPoints;
}
