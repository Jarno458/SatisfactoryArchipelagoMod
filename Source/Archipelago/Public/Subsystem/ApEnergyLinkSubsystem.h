#pragma once

#include "CoreMinimal.h"

#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Patching/NativeHookManager.h"
#include "FGPowerInfoComponent.h"
#include "Buildables/FGBuildablePowerStorage.h"

#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApServerRandomizerSubsystem.h"
#include "Subsystem/ApSlotDataSubsystem.h"
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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:
	bool isInitialized = false;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool IsInitialized() const { return isInitialized; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE int64 GetCurrentServerStoredEnergy() const { return currentServerStorage; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool IsEnergyLinkEnabled() const { return energyLinkEnabled; };

private:
	bool hooksInitialized = false;
	bool energyLinkEnabled = false;

	UPROPERTY(Replicated)
	int64 currentServerStorage = 0;

	float localStorage = 0.0f;

	FDelegateHandle hookHandler;

	TArray<AFGBuildablePowerStorage*> PowerStorages;

	AApSubsystem* ap;
	AApServerRandomizerSubsystem* randomizerSubsystem;
	AApSlotDataSubsystem* slotDataSubsystem;
	AApConnectionInfoSubsystem* apConnectionInfo;

	void EnergyLinkTick();
	void SendEnergyToServer(long amount);
	void OnEnergyLinkValueChanged(AP_SetReply setReply);

	const long maxPowerStorage = 999999;
	FString energyLinkDefault = "0";
};
