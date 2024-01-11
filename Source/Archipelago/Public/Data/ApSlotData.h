

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
	bool hasLoadedSlotData;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	int numberOfChecksPerMilestone;

	TArray<TArray<TMap<FString, int>>> hubLayout;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	int finalSpaceElevatorTier;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	int64 finalResourceSinkPoints;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	bool freeSampleEnabled;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	int freeSampleEquipment;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	int freeSampleBuildings;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	int freeSampleParts;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	bool freeSampleRadioactive;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	bool energyLink;

public:
	// Parse slot data from server. Returns false if invalid.
	static bool ParseSlotData(FString jsonString, FApSlotData* data);
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
