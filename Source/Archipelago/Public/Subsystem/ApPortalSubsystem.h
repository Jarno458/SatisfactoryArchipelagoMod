

#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"

#include "Buildable/ApPortal.h"

#include "ApPortalSubsystem.generated.h"

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
