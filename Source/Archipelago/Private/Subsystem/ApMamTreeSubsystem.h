#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Subsystem/ApSubsystem.h"
#include "FGSchematicManager.h"
#include "FGResearchManager.h"

#include "ApMamTreeSubsystem.generated.h"

//TODO REMOVE
#pragma optimize("", off)

DECLARE_LOG_CATEGORY_EXTERN(LogApMamTreeSubsystem, Log, All);

UCLASS(Abstract, Blueprintable)
class ARCHIPELAGO_API AApMamTreeSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApMamTreeSubsystem();

	virtual void BeginPlay() override;

	static AApMamTreeSubsystem* Get(class UWorld* world);

	void Initialize();

	UFUNCTION(BlueprintImplementableEvent)
	TSet<int64> GetVisibleMamNodeIds() const;

protected:


private:

	//UFUNCTION() //required for event binding
	//void OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic);


};

#pragma optimize("", on)
