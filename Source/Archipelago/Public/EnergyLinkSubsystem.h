#pragma once

#include "CoreMinimal.h"

#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Patching/NativeHookManager.h"
#include "FGPowerInfoComponent.h"
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
	bool apInitialized = false;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	long currentServerStorage = 999999;

	float localStorage = 0.0f;
	FTimerHandle timerHandle;

	TArray<AFGBuildablePowerStorage*> PowerStorages;

	AApSubsystem* ap = nullptr;

	void SecondThick();
	void SendEnergyToServer(long amount);
};
