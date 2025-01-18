#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "Resources/FGItemDescriptor.h"

#include "ApTrapSubsystem.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FApTrapData {
	GENERATED_BODY()

	// Some things handled by the trap subsystem are not actually "traps" in the AP sense, they should have this as false
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool IsDangerous = true;

	// Triggering the trap will spawn this actor.
	// If null, the trap subsystem requires a custom implementation to spawn it.
	// If subclass of AFGItemPickup, treated as an "Item Trap"
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<AActor> ActorToSpawn;

	// The end-user-facing name of the trap, as it appears in the Archipelago options page and hint text
	// Intentionally not FText because it should not be translated (must match hard-coded AP server data)
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString UserFacingName;
};


/**
 * Blueprint implemented subsystem for spawning Archipelago traps on players
 */
UCLASS(Abstract, Blueprintable)
class ARCHIPELAGO_API AApTrapSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	// Registry of available traps, map of internal trap Name to trap data
	// Populated in the blueprint implementation of this subsystem
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
		TMap<FName, FApTrapData> TrapTypes;

public:
	UFUNCTION(BlueprintCallable)
		bool IsValidTrapType(FName trapName);

	// Spawn a trap on a player, or all players if targetPlayer is nullptr
	// The trap name should be an item in TrapTypes
	// Returns true on success, false if the spawning failed somehow on at least one target player or if the trapName didn't exist
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		bool SpawnTrap(FName trapName, APlayerState* targetPlayer = nullptr);

	// Spawn an "item trap" on a player, or all players if targetPlayer is nullptr
	// Returns true on success, false if the spawning failed somehow on at least one target player
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		bool SpawnItemTrap(TSubclassOf<UFGItemDescriptor> item, int quantity = 1, APlayerState* targetPlayer = nullptr);

	// C++ getter for the subsystem - the real one is implemented in Blueprint,
	// so calling this function gets you the "real one" that inherits from the
	// C++ class and not the C++ abstract class.
	UFUNCTION(BlueprintPure, Category = "Archipelago", DisplayName = "GetArchipelagoTrapSubsystem", Meta = (DefaultToSelf = "WorldContext"))
		static AApTrapSubsystem* Get(UWorld* WorldContext);

	// Auto world context version
		static AApTrapSubsystem* Get();

};
