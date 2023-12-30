#pragma once

#include "CoreMinimal.h"
#include "FGGamePhaseManager.h"
#include "FGResourceSinkSubsystem.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Subsystem/ApSubsystem.h"
#include "Data/ApSlotData.h"


#include "ApGoalSubsystem.generated.h"

UCLASS()
class ARCHIPELAGO_API AApGoalSubsystem : public AModSubsystem
{
	GENERATED_BODY()

	AFGGamePhaseManager* phaseManager;
	AFGResourceSinkSubsystem* resourceSinkSubsystem;

	AApSubsystem* ap;

public:
	AApGoalSubsystem();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	static AApGoalSubsystem* Get();
	static AApGoalSubsystem* Get(class UWorld* world);

	bool AreGoalsCompleted(const FApSlotData* slotData);

private:
	bool hasSentGoal;

	bool CheckSpaceElevatorGoal(const FApSlotData* slotData);
	bool CheckResourceSinkPointsGoal(const FApSlotData* slotData);
};
