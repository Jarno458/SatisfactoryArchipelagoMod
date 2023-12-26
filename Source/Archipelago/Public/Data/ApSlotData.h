

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

	UPROPERTY(BlueprintReadOnly, SaveGame)
	int numberOfChecksPerMilestone;

	TArray<TArray<TMap<FString, int>>> hubLayout;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	int finalSpaceElevatorTier;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	int64 finalResourceSinkPoints;

public:
	// Parse slot data from server. Returns false if invalid.
	static bool ParseSlotData(std::string json, FApSlotData* data);
};

USTRUCT(BlueprintType)
struct ARCHIPELAGO_API FApSaveableHubLayout
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	int tier;

	UPROPERTY(SaveGame)
	int milestone;

	UPROPERTY(SaveGame)
	TMap<FString, int> costs;
};
