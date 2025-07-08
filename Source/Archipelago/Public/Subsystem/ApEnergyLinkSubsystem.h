#pragma once

#include "CoreMinimal.h"
#include "Math/BigInt.h"

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

#define ENERGYLINK_MULTIPLIER 100000

UENUM(BlueprintType)
enum class EApEnergyLinkSuffix : uint8 {
	Mega UMETA(DisplayName = "Mw"),
	Giga UMETA(DisplayName = "Gw"),
	Tera UMETA(DisplayName = "Tw"),
	Peta UMETA(DisplayName = "Pw"),
	Exa UMETA(DisplayName = "Ew"),
	Zetta UMETA(DisplayName = "Zw"),
	Yotta UMETA(DisplayName = "Yw"),
	Overflow UMETA(DisplayName = "Overflow")
};

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
	FORCEINLINE bool IsEnergyLinkEnabled() const { return energyLinkEnabled; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE int GetCurrentServerStoredEnergy() const { return replicatedServerStorage; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE EApEnergyLinkSuffix GetCurrentServerStoredEnergySuffix() const { return replicatedServerStorageSuffix; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetGlobalChargeRate() const { return replicatedGlobalChargeRatePerSecond; };

private:
	bool hooksInitialized = false;
	bool energyLinkEnabled = false;

	float currentServerStorage = 0;

	UPROPERTY(Replicated)
	uint16 replicatedServerStorage = 0;
	UPROPERTY(Replicated)
	EApEnergyLinkSuffix replicatedServerStorageSuffix = EApEnergyLinkSuffix::Mega;
	UPROPERTY(Replicated)
	float replicatedGlobalChargeRatePerSecond = 0;

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

	static int256 Int256FromDecimal(FString decimal);
};
