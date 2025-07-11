#include "Subsystem/ApGoalSubsystem.h"
#include "FGGamePhase.h"
#include "Patching/NativeHookManager.h"

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

	if (!hooksInitialized && !WITH_EDITOR) {
		hookHandler = SUBSCRIBE_METHOD_AFTER(AFGResourceSinkSubsystem::CalculateAccumulatedPointsPastInterval, [this](AFGResourceSinkSubsystem* self) {
			UpdateTotalRemainingPointHistory();
		});

		hooksInitialized = true;
	}
}

void AApGoalSubsystem::EndPlay(const EEndPlayReason::Type endPlayReason) {
	if (hookHandler.IsValid())
		UNSUBSCRIBE_METHOD(AFGResourceSinkSubsystem::CalculateAccumulatedPointsPastInterval, hookHandler);
}

void AApGoalSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (connectionInfoSubsystem->GetConnectionState() != EApConnectionState::Connected
		|| !slotData->HasLoadedSlotData())
		return;

	if (!isInitilized) {
		InitializeTotalRemainingPointHistory();

		isInitilized = true;
	}

	UpdateResourceSinkPerMinuteGoal();

	if (hasSentGoal)
		return;

	if (AreGoalsCompleted()) {
		UE_LOG(LogApSubsystem, Display, TEXT("Sending goal completion to server"));

		ap->MarkGameAsDone();

		hasSentGoal = true;
	}
}

TArray<FApGraphInfo> AApGoalSubsystem::GetResourceSinkGoalGraphs() {
	const FLinearColor perMinuteOnTrackColor(0, 0.5f, 0);
	const FLinearColor completedColor(0, 0.9f, 0);
	const FLinearColor perMinuteColor(0.9f, 0, 0.2f);
	const FLinearColor totalColor(0.1f, 0, 0.9f);

	TArray<FApGraphInfo> graphs;

	if (slotData->IsResourceSinkPerMinuteGoalEnabled()) {
		FApGraphInfo resourceSinkPerMinuteThresholdGraph;
		FString remaining;

		if (hasCompletedResourceSinkPerMinute)
			remaining = TEXT("done");
		else
			remaining = GetPerMinuteSinkGoalRemainingTime().ToString(TEXT("%m:%s")).RightChop(1);
		
		resourceSinkPerMinuteThresholdGraph.Id = FString(TEXT("RSPMG"));
		resourceSinkPerMinuteThresholdGraph.DisplayName = FText::FromString(FString::Format(TEXT("P/Min Goal ({0})"), { *remaining }));
		resourceSinkPerMinuteThresholdGraph.Suffix = FText::FromString(TEXT("p/min"));
		resourceSinkPerMinuteThresholdGraph.FullName = FText::FromString(TEXT("ResourceSink points per minute Threshold"));
		resourceSinkPerMinuteThresholdGraph.Description = 
			FText::FromString(TEXT("Maintain your standart points above this line for 10 minutes to complete your ResourceSink points per minute goal"));

		if (hasCompletedResourceSinkPerMinute)
			resourceSinkPerMinuteThresholdGraph.Color = completedColor;
		else if (countdownStartedTime.GetYear() > 2000)
			resourceSinkPerMinuteThresholdGraph.Color = perMinuteOnTrackColor;
		else
			resourceSinkPerMinuteThresholdGraph.Color = perMinuteColor;

		resourceSinkPerMinuteThresholdGraph.DataPoints.SetNum(resourceSinkSubsystem->mGlobalHistorySize);
		for (int i = 0; i < resourceSinkPerMinuteThresholdGraph.DataPoints.Num(); i++) {
			resourceSinkPerMinuteThresholdGraph.DataPoints[i] = slotData->GePerMinuteResourceSinkPoints();
		}

		graphs.Add(resourceSinkPerMinuteThresholdGraph);
	}

	if (slotData->IsResourceSinkGoalEnabled()) {
		FApGraphInfo totalResourceSinkGoal;

		totalResourceSinkGoal.Id = FString(TEXT("TPG"));
		totalResourceSinkGoal.DisplayName = FText::FromString(TEXT("Remaining for Goal"));
		totalResourceSinkGoal.Suffix = FText::FromString(TEXT(""));
		totalResourceSinkGoal.FullName = FText::FromString(TEXT("ResourceSink points per minute Threshold"));
		totalResourceSinkGoal.Description =
			FText::FromString(TEXT("Maintain your standart points above this line for 10 minutes to complete your ResourceSink points per minute goal"));

		int64 remaining = GetTotalRemainingPoints();

		if (remaining == 0)
			totalResourceSinkGoal.Color = completedColor;
		else
			totalResourceSinkGoal.Color = totalColor;

		totalResourceSinkGoal.DataPoints = totalRemainingPointHistory;

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

void AApGoalSubsystem::InitializeTotalRemainingPointHistory() {
	//if it didnt get set based on save game we will it current remaining points
	if (totalRemainingPointHistory.IsEmpty()) {
		int64 remaining = GetTotalRemainingPoints();

		totalRemainingPointHistory.SetNum(resourceSinkSubsystem->mGlobalHistorySize); //should be 10
		for (int i = 0; i < totalRemainingPointHistory.Num(); i++) {
			totalRemainingPointHistory[i] = remaining;
		}
	}
}

void AApGoalSubsystem::UpdateTotalRemainingPointHistory() {
	if (!totalRemainingPointHistory.IsEmpty())
		totalRemainingPointHistory.RemoveAt(0);
	totalRemainingPointHistory.Add(GetTotalRemainingPoints());
}

int64 AApGoalSubsystem::GetTotalRemainingPoints() {
	int64 totalSinkPoints = resourceSinkSubsystem->GetNumTotalPoints(EResourceSinkTrack::RST_Default);
	int64 finalSinkPoints = slotData->GetFinalResourceSinkPoints();

	int64 remaining = finalSinkPoints - totalSinkPoints;
	if (remaining < 0)
		remaining = 0;

	return remaining;
}

bool AApGoalSubsystem::AreGoalsCompleted() {
	if (slotData->RequireAllGoals())
		return   (!slotData->IsSpaceElevatorGoalEnabled() || CheckSpaceElevatorGoal())
				&& (!slotData->IsResourceSinkGoalEnabled() || CheckResourceSinkPointsGoal())
				&& (!slotData->IsResourceSinkPerMinuteGoalEnabled() || CheckResourceSinkPointPerMinuteGoal())
				&& (!slotData->IsExplorationGoalEnabled() || CheckExplorationGoal());
	else
		return   (slotData->IsSpaceElevatorGoalEnabled() && CheckSpaceElevatorGoal())
				|| (slotData->IsResourceSinkGoalEnabled() && CheckResourceSinkPointsGoal())
				|| (slotData->IsResourceSinkPerMinuteGoalEnabled() && CheckResourceSinkPointPerMinuteGoal())
				|| (slotData->IsExplorationGoalEnabled() && CheckExplorationGoal());
}

bool AApGoalSubsystem::CheckSpaceElevatorGoal() {
	return phaseManager->GetCurrentGamePhase()->mGamePhase >= slotData->GetFinalSpaceElevatorTier();
}

bool AApGoalSubsystem::CheckResourceSinkPointsGoal() {
	return resourceSinkSubsystem->GetNumTotalPoints(EResourceSinkTrack::RST_Default) >= slotData->GetFinalResourceSinkPoints();
}

bool AApGoalSubsystem::CheckResourceSinkPointPerMinuteGoal() {
	return hasCompletedResourceSinkPerMinute;
}

bool AApGoalSubsystem::CheckExplorationGoal() {
	return schematicManager->IsSchematicPurchased(explorationGoalSchematic);
}

FTimespan AApGoalSubsystem::GetPerMinuteSinkGoalRemainingTime() {
	if (countdownStartedTime.GetYear() < 2000) {
		return totalResourceSinkPerMinuteDuration;
	}
	
	return totalResourceSinkPerMinuteDuration - (FDateTime::UtcNow() - countdownStartedTime);
}

void AApGoalSubsystem::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) {
	remainingSecondsForPerMinuteGoal = GetRemainingSecondsForResourceSinkPerMinuteGoal();
};

void AApGoalSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) {
	int totalSeconds = GetTotalSecondsForResourceSinkPerMinuteGoal();

	if (remainingSecondsForPerMinuteGoal >= totalSeconds)
		countdownStartedTime = FDateTime::MinValue();
	else 
		countdownStartedTime = FDateTime(FDateTime::UtcNow().GetTicks() - FTimespan(0, 0, remainingSecondsForPerMinuteGoal).GetTicks());
}
