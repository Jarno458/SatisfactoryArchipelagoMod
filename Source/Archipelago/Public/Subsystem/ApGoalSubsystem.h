#pragma once

#include "CoreMinimal.h"

#include "FGSchematicManager.h"
#include "FGGamePhaseManager.h"
#include "FGResearchManager.h"

#include "Subsystem/ModSubsystem.h"

#include "ApGoalSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class ARCHIPELAGO_API AApGoalSubsystem : public AModSubsystem
{
	GENERATED_BODY()

	AFGSchematicManager* schematicManager;
	AFGResearchManager* researchManager;
	AFGGamePhaseManager* phaseManager;
	AFGResourceSinkSubsystem* resourceSinkSubsystem;

public:
	virtual void BeginPlay() override;

	static AApGoalSubsystem* Get();
	static AApGoalSubsystem* Get(class UWorld* world);

	bool AreGoalsCompleted(const FApSlotData* slotData);

private:
	bool CheckSpaceElevatorGoal(const FApSlotData* slotData);
	bool CheckResourceSinkPointsGoal(const FApSlotData* slotData);
};
