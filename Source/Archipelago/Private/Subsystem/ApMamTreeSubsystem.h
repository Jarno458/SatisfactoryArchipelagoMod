#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/ApSubsystem.h"

#include "ApMamTreeSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApMamTreeSubsystem, Log, All);

UCLASS(Abstract, Blueprintable)
class ARCHIPELAGO_API AApMamTreeSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApMamTreeSubsystem();

	static AApMamTreeSubsystem* Get(class UWorld* world);

	UFUNCTION(BlueprintImplementableEvent)
	TSet<int64> GetVisibleMamNodeIds() const;
};
