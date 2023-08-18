#include "Subsystem/ApTrapSubsystem.h"
#include "ApUtils.h"

bool AApTrapSubsystem::SpawnTrap_Implementation(FName trapName, APlayerState* targetPlayer) {
	UE_LOG(LogArchipelagoCpp, Error, TEXT("SpawnTrap_Implementation should be implemented in Blueprint"));
	return false;
}

bool AApTrapSubsystem::SpawnItemTrap_Implementation(TSubclassOf<UFGItemDescriptor> item, int quantity, APlayerState* targetPlayer) {
	UE_LOG(LogArchipelagoCpp, Error, TEXT("SpawnItemTrap_Implementation should be implemented in Blueprint"));
	return false;
}

AApTrapSubsystem* AApTrapSubsystem::Get(UObject* WorldContext) {
	if (!WorldContext) {
		return nullptr;
	}
	if (WorldContext->GetWorld()) {
		TArray<AActor*> arr;
		// Relies on the fact that nothing has spawned the C++ version before
		// The blueprint one descends from this so it is found instead
		// This would break if a C++ version was spawned or persisted via save game
		UGameplayStatics::GetAllActorsOfClass(WorldContext->GetWorld(), AApTrapSubsystem::StaticClass(), arr);
		if (arr.IsValidIndex(0)) {
			return Cast<AApTrapSubsystem>(arr[0]);
		} else {
			return nullptr;
		}
	} else {
		return nullptr;
	}
}

AApTrapSubsystem* AApTrapSubsystem::Get() {
	return Get(GEngine->GameViewport->GetWorld());
}
