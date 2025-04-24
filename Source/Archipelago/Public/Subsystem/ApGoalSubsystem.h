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

	bool AreGoalsCompleted();

private:
	bool hasSentGoal;

	bool CheckSpaceElevatorGoal();
	bool CheckResourceSinkPointsGoal();
	bool CheckResourceSinkPointPerMinuteGoal();
	bool CheckExplorationGoal();
};
