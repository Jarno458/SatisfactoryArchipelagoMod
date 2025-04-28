#pragma once

#include "CoreMinimal.h"
#include "FGGamePhaseManager.h"
#include "FGResourceSinkSubsystem.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Subsystem/ApSlotDataSubsystem.h"
#include "Subsystem/ApSchematicPatcherSubsystem.h"
#include "Misc/DateTime.h"

#include "ApGoalSubsystem.generated.h"

USTRUCT(BlueprintType)
struct ARCHIPELAGO_API FApGoalGraphInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	FString Id;

	UPROPERTY(BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite)
	FText FullName;

	UPROPERTY(BlueprintReadWrite)
	FText Suffix;

	UPROPERTY(BlueprintReadWrite)
	FText Description;

	UPROPERTY(BlueprintReadWrite)
	FLinearColor Color;

	UPROPERTY(BlueprintReadWrite)
	TArray<float> DataPoints;
};

UCLASS()
class ARCHIPELAGO_API AApGoalSubsystem : public AModSubsystem
{
	GENERATED_BODY()

private:
	AFGGamePhaseManager* phaseManager;
	AFGResourceSinkSubsystem* resourceSinkSubsystem;
	AFGSchematicManager* schematicManager;

	AApSubsystem* ap;
	AApConnectionInfoSubsystem* connectionInfoSubsystem;
	AApSlotDataSubsystem* slotData;

	TSubclassOf<class UFGSchematic> explorationGoalSchematic;

public:
	AApGoalSubsystem();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	static AApGoalSubsystem* Get(class UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Ap Goal Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApGoalSubsystem* Get(UObject* worldContext);

	bool AreGoalsCompleted();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FApGoalGraphInfo> GetResourceSinkGoalGraphs(int nunDataPoints);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetRemainingSecondsForResourceSinkPerMinuteGoal();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetTotalSecondsForResourceSinkPerMinuteGoal();

private:
	const FTimespan totalResourceSinkPerMinuteDuration = FTimespan(0, 0, 10, 0, 0);

	bool hasSentGoal = false;

	UPROPERTY(SaveGame)
	FDateTime countdownStartedTime;

	UPROPERTY(SaveGame)
	bool hasCompletedResourceSinkPerMinute = false;

	bool CheckSpaceElevatorGoal();
	bool CheckResourceSinkPointsGoal();
	bool CheckResourceSinkPointPerMinuteGoal();
	bool CheckExplorationGoal();

	void UpdateResourceSinkPerMinuteGoal();

	FTimespan GetPerMinuteSinkGoalRemainingTime();
};
