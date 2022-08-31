#pragma once

#include "CoreMinimal.h"

#include "Subsystem/ModSubsystem.h"
#include "Patching/NativeHookManager.h"

#include "Buildables/FGBuildablePowerStorage.h"

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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	float GetPowerStore();

private:
	int localStorage;
	FTimerHandle timerHandle;

	void SecondThick();
};
