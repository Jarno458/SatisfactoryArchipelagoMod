#include "Subsystem/ApGoalSubsystem.h"
#include "FGGamePhase.h"

//TODO REMOVE
#pragma optimize("", off)

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

AApGoalSubsystem* AApGoalSubsystem::Get(class UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

void AApGoalSubsystem::BeginPlay() {
	Super::BeginPlay();

	UWorld* world = GetWorld();

	phaseManager = AFGGamePhaseManager::Get(world);
	resourceSinkSubsystem = AFGResourceSinkSubsystem::Get(world);
	schematicManager = AFGSchematicManager::Get(world);

	ap = AApSubsystem::Get(world);
	connectionInfoSubsystem = AApConnectionInfoSubsystem::Get(world);
	slotData = AApSlotDataSubsystem::Get(world);

	AApSchematicPatcherSubsystem* schematicPatcher = AApSchematicPatcherSubsystem::Get(world);
	explorationGoalSchematic = schematicPatcher->GetExplorationSchematic();
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

FApExplorationGraphInfo AApGoalSubsystem::GetExplorationGoalInfo(FString graphId, TArray<float> dataPoints, FText suffix, FLinearColor color) {
	FApExplorationGraphInfo graph;

	graph.DisplayName = FText::FromString(TEXT("ExplGoal"));
	graph.FullName = FText::FromString(TEXT("Exploration Goal"));
	graph.Description = FText::FromString(TEXT("Maintain your sink points above this line for 10 minutes to complete your exploration goal"));
	graph.Color = FLinearColor::Red;
	
	graph.DataPoints.SetNum(dataPoints.Num());
	
	for (int i = 0; i < graph.DataPoints.Num(); i++) {
		graph.DataPoints[i] = 1000.0f;
	}

	return graph;
}

bool AApGoalSubsystem::AreGoalsCompleted() {
	if (slotData->RequireAllGoals())
		return   CheckSpaceElevatorGoal() 
				&& CheckResourceSinkPointsGoal()
				&& CheckResourceSinkPointPerMinuteGoal()
				&& CheckExplorationGoal();
	else
		return   CheckSpaceElevatorGoal() 
				|| CheckResourceSinkPointsGoal()
				|| CheckResourceSinkPointPerMinuteGoal()
				|| CheckExplorationGoal();
}

bool AApGoalSubsystem::CheckSpaceElevatorGoal() {
	return slotData->IsSpaceElevatorGoalEnabled()
		&& phaseManager->GetCurrentGamePhase()->mGamePhase >= slotData->GetFinalSpaceElevatorTier();
}

bool AApGoalSubsystem::CheckResourceSinkPointsGoal() {
	return slotData->IsResourceSinkGoalEnabled()
		&&	resourceSinkSubsystem->GetNumTotalPoints(EResourceSinkTrack::RST_Default) >= slotData->GetFinalResourceSinkPoints();
}

bool AApGoalSubsystem::CheckResourceSinkPointPerMinuteGoal() {
	if (!slotData->IsResourceSinkPerMinuteGoalEnabled())
		return false;

	TArray<int32> pointHistory = resourceSinkSubsystem->GetGlobalPointHistory(EResourceSinkTrack::RST_Default);
	if (pointHistory.Num() < 10)
		return false;

	for (int32 i = pointHistory.Num() - 1; i > pointHistory.Num() - 11; i--)
	{
		if (pointHistory[i] < slotData->GePerMinuteResourceSinkPoints())
			return false;
	}
		
	return true;
}

bool AApGoalSubsystem::CheckExplorationGoal() {
	return slotData->IsExplorationGoalEnabled()
		&& schematicManager->IsSchematicPurchased(explorationGoalSchematic);
}

#pragma optimize("", on)