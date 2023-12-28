#include "Subsystem/ApGoalSubsystem.h"

#include "FGResourceSinkSubsystem.h"

#include "Subsystem/SubsystemActorManager.h"

#include "Data/ApSlotData.h"


AApGoalSubsystem::AApGoalSubsystem() : Super() {
	PrimaryActorTick.bCanEverTick = false;
}

AApGoalSubsystem* AApGoalSubsystem::Get() {
	return Get(GEngine->GameViewport->GetWorld());
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
