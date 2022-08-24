#pragma once

#include "CoreMinimal.h"

#include "Subsystem/ModSubsystem.h"

#include "ApSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN(ApEnergyLink, Log, All);

#include "EnergyLinkSubsystem.generated.h"

UCLASS(Blueprintable)
class ARCHIPELAGO_API AEnergyLinkSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AEnergyLinkSubsystem();

	FTimerHandle timerHandle;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void SecondThick();

};
