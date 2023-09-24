

#pragma once

#include "CoreMinimal.h"

#include "Engine/UserDefinedStruct.h"

#include "ApMappings.h"

#include "ApSlotData.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct ARCHIPELAGO_API FApSlotData
{
	GENERATED_BODY()
public:
	FApSlotData();

	bool hasLoadedSlotData;

	UPROPERTY(BlueprintReadOnly)
	int numberOfChecksPerMilestone;

	TArray<TArray<TMap<FString, int>>> hubLayout;

	UPROPERTY(BlueprintReadOnly)
	int finalSpaceElevatorTier;

	UPROPERTY(BlueprintReadOnly)
	int64 finalResourceSinkPoints;

public:
	// Parse slot data from server. Returns false if invalid.
	static bool ParseSlotData(std::string json, FApSlotData* data);
};
