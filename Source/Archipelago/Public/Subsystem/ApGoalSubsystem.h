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
class ARCHIPELAGO_API AApGoalSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	AApGoalSubsystem();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	// Begin IFGSaveInterface
	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void GatherDependencies_Implementation(TArray<UObject*>& out_dependentObjects) override {};
	virtual bool NeedTransform_Implementation() override { return false; };
	virtual bool ShouldSave_Implementation() const override { return true; };
	// End IFSaveInterface

	static AApGoalSubsystem* Get(class UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Ap Goal Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApGoalSubsystem* Get(UObject* worldContext);

private:
	AFGGamePhaseManager* phaseManager;
	AFGResourceSinkSubsystem* resourceSinkSubsystem;
	AFGSchematicManager* schematicManager;

	AApSubsystem* ap;
	AApConnectionInfoSubsystem* connectionInfoSubsystem;
	AApSlotDataSubsystem* slotData;

	TSubclassOf<class UFGSchematic> explorationGoalSchematic;

public:
	bool AreGoalsCompleted();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FApGoalGraphInfo> GetResourceSinkGoalGraphs();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetRemainingSecondsForResourceSinkPerMinuteGoal();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetTotalSecondsForResourceSinkPerMinuteGoal();



private:
	const FTimespan totalResourceSinkPerMinuteDuration = FTimespan(0, 0, 10, 0, 0);

	bool isInitilized = false;

	bool hasSentGoal = false;

	bool hooksInitialized = false;
	FDelegateHandle hookHandler;

	UPROPERTY(SaveGame)
	TArray<float> totalRemainingPointHistory;

	UPROPERTY(SaveGame)
	int remainingSecondsForPerMinuteGoal;

	FDateTime countdownStartedTime;

	UPROPERTY(SaveGame)
	bool hasCompletedResourceSinkPerMinute = false;
	/*UPROPERTY(Replicated) //we might need to replicate these as a client otherwise wont know when a goal is reached
	bool hasCompletedResourceSinkTotal = false;
	UPROPERTY(Replicated)
	bool hasCompletedElevator = false;
	UPROPERTY(Replicated)
	bool hasCompletedExploration = false;*/

	void InitializeTotalRemainingPointHistory();

	bool CheckSpaceElevatorGoal();
	bool CheckResourceSinkPointsGoal();
	bool CheckResourceSinkPointPerMinuteGoal();
	bool CheckExplorationGoal();

	void UpdateResourceSinkPerMinuteGoal();
	void UpdateTotalRemainingPointHistory();

	FTimespan GetPerMinuteSinkGoalRemainingTime();

	int64 GetTotalRemainingPoints();
};
