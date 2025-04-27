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

#include "ApGoalSubsystem.generated.h"

USTRUCT(BlueprintType)
struct ARCHIPELAGO_API FApExplorationGraphInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite)
	FText FullName;

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
	FApExplorationGraphInfo GetExplorationGoalInfo(FString graphId, TArray<float> dataPoints, FText suffix, FLinearColor color);

private:
	bool hasSentGoal;

	bool CheckSpaceElevatorGoal();
	bool CheckResourceSinkPointsGoal();
	bool CheckResourceSinkPointPerMinuteGoal();
	bool CheckExplorationGoal();
};
