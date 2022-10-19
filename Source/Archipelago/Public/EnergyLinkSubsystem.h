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
	bool hooksInitialized = false;

	long currentServerStorage = 0;

	float localStorage = 0.0f;

	TArray<AFGBuildablePowerStorage*> PowerStorages;

	AApSubsystem* ap = nullptr;

	void SecondThick();
	void SendEnergyToServer(long amount);
	void OnEnergyLinkValueChanged(AP_SetReply setReply);

	const long maxPowerStorage = 999999;
	std::string energyLinkDefault = "0";
};
