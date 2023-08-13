#pragma once

#include "CoreMinimal.h"

#include "Module/ModModule.h"
#include "Module/GameWorldModule.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "ApSubsystem.h"
#include "EnergyLinkSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApGameWorldModule, Log, All);

#include "ApGameWorldModule.generated.h"

UCLASS(Abstract, Blueprintable)
class ARCHIPELAGO_API UApGameWorldModule : public UGameWorldModule
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	UApGameWorldModule();

	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
};
