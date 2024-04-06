#pragma once

#include "CoreMinimal.h"

#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Patching/NativeHookManager.h"
#include "FGPowerInfoComponent.h"
#include "Buildables/FGBuildablePowerStorage.h"

#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApEnergyLink, Log, All);

#include "ApEnergyLinkSubsystem.generated.h"

UCLASS(Blueprintable)
class ARCHIPELAGO_API AApEnergyLinkSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AApEnergyLinkSubsystem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	bool apInitialized = false;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	const bool IsEnergyLinkEnabled();

private:
	bool hooksInitialized = false;
	bool energyLinkEnabled = false;

	long currentServerStorage = 0;

	float localStorage = 0.0f;

	TArray<AFGBuildablePowerStorage*> PowerStorages;

	ApConnectionInfoSubsystem apConnectionInfo = nullptr
	ApSubsystem* ap = nullptr;

	void EnergyLinkTick();
	void SendEnergyToServer(long amount);
	void OnEnergyLinkValueChanged(AP_SetReply setReply);

	const long maxPowerStorage = 999999;
	FString energyLinkDefault = "0";
};
