

#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "Resources/FGItemDescriptor.h"

#include "ApTrapSubsystem.generated.h"

/**
 * Blueprint implemented subsystem for spawning Archipelago traps on players
 */
UCLASS(Abstract, Blueprintable)
class ARCHIPELAGO_API AApTrapSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	// Registry of available traps, there could be nullptr entries for custom traps that aren't Actors 
	// Populated in the blueprint implementation of this subsystem
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
		TMap<FName, TSubclassOf<AActor>> TrapTypes;

public:
	// Spawn a trap on a player, or all players if targetPlayer is nullptr
	// The trap name should be an item in TrapTypes
	// Returns true on success, false if the spawning failed somehow on at least one target player or if the trapName didn't exist
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		bool SpawnTrap(FName trapName, APlayerState* targetPlayer = nullptr);

	// Spawn an "item trap" on a player, or all players if targetPlayer is nullptr
	// Returns true on success, false if the spawning failed somehow on at least one target player
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		bool SpawnItemTrap(TSubclassOf<UFGItemDescriptor> item, int quantity = 1, APlayerState* targetPlayer = nullptr);
};
