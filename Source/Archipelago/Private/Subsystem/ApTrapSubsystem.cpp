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
