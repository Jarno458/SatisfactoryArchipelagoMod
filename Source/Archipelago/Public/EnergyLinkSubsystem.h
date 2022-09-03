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

private:
	const int max_capacity = 9999999;
	int currentServerStorage = 9999999;

	int localStorage;
	FTimerHandle timerHandle;

	TArray<AFGBuildablePowerStorage*> PowerStorages;

	void SecondThick();
};
