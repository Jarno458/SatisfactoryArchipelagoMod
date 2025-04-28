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

	countdownStartedTime = FDateTime::MinValue();
}

void AApGoalSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (connectionInfoSubsystem->GetConnectionState() != EApConnectionState::Connected
		|| !slotData->HasLoadedSlotData())
		return;

	UpdateResourceSinkPerMinuteGoal();

	if (hasSentGoal)
		return;

	if (AreGoalsCompleted()) {
		UE_LOG(LogApSubsystem, Display, TEXT("Sending goal completion to server"));

		ap->MarkGameAsDone();

		hasSentGoal = true;
	}
}

TArray<FApGoalGraphInfo> AApGoalSubsystem::GetResourceSinkGoalGraphs(int nunDataPoints) {
	TArray<FApGoalGraphInfo> graphs;

	if (slotData->IsResourceSinkPerMinuteGoalEnabled()) {
		FApGoalGraphInfo resourceSinkPerMinuteThresholdGraph;

		FString remaining = GetPerMinuteSinkGoalRemainingTime().ToString(TEXT("%m:%s")).RightChop(1);
		
		resourceSinkPerMinuteThresholdGraph.Id = FString(TEXT("RSPMG"));
		resourceSinkPerMinuteThresholdGraph.DisplayName = FText::FromString(FString::Format(TEXT("P/Min Goal ({0})"), { *remaining }));
		resourceSinkPerMinuteThresholdGraph.Suffix = FText::FromString(TEXT("p/min"));
		resourceSinkPerMinuteThresholdGraph.FullName = FText::FromString(TEXT("ResourceSink points per minute Threshold"));
		resourceSinkPerMinuteThresholdGraph.Description = 
			FText::FromString(TEXT("Maintain your standart points above this line for 10 minutes to complete your ResourceSink points per minute goal"));

		if (countdownStartedTime.GetYear() > 2000)
			resourceSinkPerMinuteThresholdGraph.Color = FLinearColor::Green;
		else
			resourceSinkPerMinuteThresholdGraph.Color = FLinearColor::Red;

		resourceSinkPerMinuteThresholdGraph.DataPoints.SetNum(nunDataPoints);
		for (int i = 0; i < nunDataPoints; i++) {
			resourceSinkPerMinuteThresholdGraph.DataPoints[i] = slotData->GePerMinuteResourceSinkPoints();
		}

		graphs.Add(resourceSinkPerMinuteThresholdGraph);
	}

	if (slotData->IsResourceSinkGoalEnabled()) {
		FApGoalGraphInfo totalResourceSinkGoal;

		int64 totalSinkPoints = resourceSinkSubsystem->GetNumTotalPoints(EResourceSinkTrack::RST_Default);
		int64 finalSinkPoints = slotData->GetFinalResourceSinkPoints();

		int64 remaining = finalSinkPoints - totalSinkPoints;
		if (remaining < 0)
			remaining = 0;

		totalResourceSinkGoal.Id = FString(TEXT("TPG"));
		totalResourceSinkGoal.DisplayName = FText::FromString(TEXT("Remaining for Goal"));
		totalResourceSinkGoal.Suffix = FText::FromString(TEXT(""));
		totalResourceSinkGoal.FullName = FText::FromString(TEXT("ResourceSink points per minute Threshold"));
		totalResourceSinkGoal.Description =
			FText::FromString(TEXT("Maintain your standart points above this line for 10 minutes to complete your ResourceSink points per minute goal"));

		if (remaining == 0)
			totalResourceSinkGoal.Color = FLinearColor::Green;
		else
			totalResourceSinkGoal.Color = FLinearColor::Blue;

		totalResourceSinkGoal.DataPoints.SetNum(nunDataPoints);
		for (int i = 0; i < nunDataPoints; i++) {
			totalResourceSinkGoal.DataPoints[i] = remaining;
		}

		graphs.Add(totalResourceSinkGoal);
	}

	return graphs;
}

int AApGoalSubsystem::GetRemainingSecondsForResourceSinkPerMinuteGoal() {
	return GetPerMinuteSinkGoalRemainingTime().GetTotalSeconds();
}

int AApGoalSubsystem::GetTotalSecondsForResourceSinkPerMinuteGoal() {
	return totalResourceSinkPerMinuteDuration.GetTotalSeconds();
}

void AApGoalSubsystem::UpdateResourceSinkPerMinuteGoal() {
	if (!slotData->IsResourceSinkPerMinuteGoalEnabled())
		return;

	if (hasCompletedResourceSinkPerMinute)
		return;

	int currentPointsPerMinute = 0;
	TArray<int32> standartPointHistory = resourceSinkSubsystem->GetGlobalPointHistory(EResourceSinkTrack::RST_Default);
	if (!standartPointHistory.IsEmpty())
		currentPointsPerMinute = standartPointHistory[standartPointHistory.Num() - 1];

	if (currentPointsPerMinute > slotData->GePerMinuteResourceSinkPoints()) {
		if (countdownStartedTime.GetYear() < 2000) {
			countdownStartedTime = FDateTime::UtcNow();
		}
		else if (GetPerMinuteSinkGoalRemainingTime().GetTicks() <= 0) {
			hasCompletedResourceSinkPerMinute = true;
		}
	}
	else {
		countdownStartedTime = FDateTime::MinValue();
	}
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
	return slotData->IsResourceSinkPerMinuteGoalEnabled()
		&& hasCompletedResourceSinkPerMinute;
}

FTimespan AApGoalSubsystem::GetPerMinuteSinkGoalRemainingTime() {
	if (countdownStartedTime.GetYear() < 2000) {
		return totalResourceSinkPerMinuteDuration;
	}
	
	return totalResourceSinkPerMinuteDuration - (FDateTime::UtcNow() - countdownStartedTime);
}

bool AApGoalSubsystem::CheckExplorationGoal() {
	return slotData->IsExplorationGoalEnabled()
		&& schematicManager->IsSchematicPurchased(explorationGoalSchematic);
}

#pragma optimize("", on)