#include "Subsystem/ApExplorationSubsystem.h"

DEFINE_LOG_CATEGORY(LogApExplorationSubsystem);

FApExplorationData::FApExplorationData() {
}

TMap<int64_t, FVector_NetQuantize> AApExplorationSubsystem::IdToDropPodLocation = {
	// Regenerate via /Script/Blutility.EditorUtilityWidgetBlueprint'/Archipelago/Debug/EU_GenerateTrapIds.EU_GenerateTrapIds'
	{1338999, FVector_NetQuantize(1, 2, 3)}
};

AApExplorationSubsystem* AApExplorationSubsystem::Get(UObject* WorldContext) {
	if (!WorldContext) {
		return nullptr;
	}
	if (WorldContext->GetWorld()) {
		TArray<AActor*> arr;
		// Relies on the fact that nothing has spawned the C++ version before
		// The blueprint one descends from this so it is found instead
		// This would break if a C++ version was spawned or persisted via save game
		UGameplayStatics::GetAllActorsOfClass(WorldContext->GetWorld(), AApExplorationSubsystem::StaticClass(), arr);
		if (arr.IsValidIndex(0)) {
			return Cast<AApExplorationSubsystem>(arr[0]);
		} else {
			return nullptr;
		}
	} else {
		return nullptr;
	}
}

AApExplorationSubsystem* AApExplorationSubsystem::Get() {
	return Get(GEngine->GameViewport->GetWorld());
}

void AApExplorationSubsystem::GetDropPodLocationMap(TMap<int, FVector_NetQuantize> in_idToLocation) {
	for (auto& entry : IdToDropPodLocation) {
		in_idToLocation.Add((int) entry.Key, entry.Value);
	}
}

void AApExplorationSubsystem::PopulateIdToDropPod() {
	UE_LOG(LogApExplorationSubsystem, Error, TEXT("TODO AApExplorationSubsystem::PopulateIdToDropPod"));

	TArray<AActor*> arr;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFGDropPod::StaticClass(), arr);
	if (arr.IsEmpty()) {
		UE_LOG(LogApExplorationSubsystem, Error, TEXT("No drop pods found? Called too early?"));
		return;
	}

	for (const auto& dropPod : arr) {
		// Check location against IdToDropPodLocation, complain if not present
		auto location = dropPod->GetActorLocation();
		if (const auto id = IdToDropPodLocation.FindKey(location)) {
			UE_LOG(LogApExplorationSubsystem, Display, TEXT("ID %d matched drop pod at location %s"), id, *location.ToCompactString());
			IdToDropPod.Add(*id, Cast<AFGDropPod>(dropPod));
		} else {
			UE_LOG(LogApExplorationSubsystem, Warning, TEXT("Found a drop pod in the world we that don't have data for? Skipping. At location %s"), *location.ToCompactString());
		}
	}
}
