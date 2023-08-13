#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"

#include "Buildable/ApPortal.h"

#include "ApPortalSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApPortalSubsystem, Log, All);

/**
 * 
 */
UCLASS()
class ARCHIPELAGO_API AApPortalSubsystem : public AModSubsystem
{
	GENERATED_BODY()

		AApPortalSubsystem();

	virtual void BeginPlay() override;

	virtual void Tick(float dt) override;

public:
	UPROPERTY(BlueprintReadOnly)
		TArray< AApPortal* > BuiltPortals;
};
