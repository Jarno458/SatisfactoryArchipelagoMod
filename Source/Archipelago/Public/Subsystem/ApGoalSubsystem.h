#pragma once

#include "CoreMinimal.h"

#include "FGGamePhaseManager.h"

#include "Subsystem/ModSubsystem.h"

#include "ApGoalSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class ARCHIPELAGO_API AApGoalSubsystem : public AModSubsystem
{
	GENERATED_BODY()

	AFGGamePhaseManager* phaseManager;
	AFGResourceSinkSubsystem* resourceSinkSubsystem;

public:
	AApGoalSubsystem();

	virtual void BeginPlay() override;

	static AApGoalSubsystem* Get();
	static AApGoalSubsystem* Get(class UWorld* world);

	bool AreGoalsCompleted(const FApSlotData* slotData);

private:
	bool CheckSpaceElevatorGoal(const FApSlotData* slotData);
	bool CheckResourceSinkPointsGoal(const FApSlotData* slotData);
};
