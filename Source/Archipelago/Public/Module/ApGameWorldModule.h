#pragma once

#include "CoreMinimal.h"

#include "Module/ModModule.h"
#include "Module/GameWorldModule.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"

#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "Subsystem/ApServerRandomizerSubsystem.h"
#include "Subsystem/ApSchematicPatcherSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApGameWorldModule, Log, All);

#include "ApGameWorldModule.generated.h"

/**
 * Blueprint implemented Mod Module
*/
UCLASS(Abstract, Blueprintable)
class ARCHIPELAGO_API UApGameWorldModule : public UGameWorldModule
{
	GENERATED_BODY()
	
public:	
	/**
	* Used for recipes that are used by the ApServerRandomizerSubsystem but need to explicitly go through content registration
	* Such as MAM tree nodes that are registered indirectly
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSubclassOf<class UFGSchematic>> mTreeNodeSchematics;

	// Sets default values for this actor's properties
	UApGameWorldModule();

	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
};
