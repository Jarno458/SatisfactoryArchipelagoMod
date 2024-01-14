#pragma once

#include "CoreMinimal.h"
#include "FGDropPod.h"
#include "Subsystem/ModSubsystem.h"
#include "ApExplorationSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApExplorationSubsystem, Log, All);

/**
 * TODO finish implementing
 * Represents data about an exploration location.
 * The mod runtime doesn't use most of it, but the data is needed for progression logic on the Archipelago server.
 * Data is manually entered by traveling between the locations and recording the information.
 */
USTRUCT(BlueprintType)
struct ARCHIPELAGO_API FApExplorationData {
	GENERATED_BODY()
public:
	FApExplorationData();

	UPROPERTY(BlueprintReadWrite)
		int id;

	UPROPERTY(BlueprintReadWrite)
		FVector_NetQuantize location;

	// Hazards?

	// Combat difficulty?

	// Underground y/n?

	// Access cost? (drop pod unlock items)

	// Access power? (drop pod power usage)
};

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class ARCHIPELAGO_API AApExplorationSubsystem : public AModSubsystem
{
	GENERATED_BODY()

	static TMap<int64, FVector_NetQuantize> IdToDropPodLocation;

	// TODO populate this array instead of IdToDropPodLocation once we decide what's needed and the debug tools can auto generate it
	static TMap<int64, FApExplorationData> IdToDropPodData;
	
public:
	UPROPERTY(BlueprintReadOnly)
	TMap<int, AFGDropPod*> IdToDropPod;

public:
	// C++ getter for the subsystem - the real one is implemented in Blueprint,
	// so calling this function gets you the "real one" that inherits from the
	// C++ class and not the C++ abstract class.
	UFUNCTION(BlueprintPure, Category = "Archipelago", DisplayName = "GetArchipelagoExplorationSubsystem", Meta = (DefaultToSelf = "WorldContext"))
	static AApExplorationSubsystem* Get(UObject* WorldContext);

	// This is a static field so blueprint access needs a getter to convert
	// TODO still needed?
	UFUNCTION(BlueprintCallable)
		void GetDropPodLocationMap(TMap<int, FVector_NetQuantize> in_idToLocation);

	UFUNCTION(BlueprintCallable)
		void PopulateIdToDropPod();
};
