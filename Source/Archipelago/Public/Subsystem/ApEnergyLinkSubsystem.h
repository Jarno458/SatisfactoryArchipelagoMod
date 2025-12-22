#pragma once

#include "CoreMinimal.h"
#include "Math/BigInt.h"

#include "Subsystem/ModSubsystem.h"
#include "Patching/NativeHookManager.h"
#include "FGPowerInfoComponent.h"
#include "Buildables/FGBuildablePowerStorage.h"
#include "FGCircuitSubsystem.h"

#include "Subsystem/ApSubsystem.h"
#include "Data/ApGraphs.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApEnergyLink, Log, All);

#include "ApEnergyLinkSubsystem.generated.h"

#define ENERGYLINK_MULTIPLIER 1000
#define ENERYLINK_DEPOSIT_REDUCTION_FACTOR 0.75f
#define ENERGYLINK_STORE_CAPACITY 99999.0f

UENUM(BlueprintType)
enum class EApUnitSuffix : uint8 {
	Deci UMETA(DisplayName = "Deci"),
	Kilo UMETA(DisplayName = "Kilo"),
	Mega UMETA(DisplayName = "Mega"),
	Giga UMETA(DisplayName = "Giga"),
	Tera UMETA(DisplayName = "Tera"),
	Peta UMETA(DisplayName = "Peta"),
	Exa UMETA(DisplayName = "Exa"),
	Overflow UMETA(DisplayName = "Overflow")
};

UENUM(BlueprintType)
enum class EApEnergyLinkState : uint8 {
	Initializing UMETA(DisplayName = "Initializing"),
	Disabled UMETA(DisplayName = "Disabled"),
	Enabled UMETA(DisplayName = "Enabled"),
	Unavailable UMETA(DisplayName = "Unavailable")
};

UCLASS(Blueprintable)
class ARCHIPELAGO_API AApEnergyLinkSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApEnergyLinkSubsystem();

	static AApEnergyLinkSubsystem* Get(class UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Energy Link Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApEnergyLinkSubsystem* Get(UObject* worldContext);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void Tick(float DeltaTime) override;

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE EApEnergyLinkState GetEnergyLinkState() const { return energyLinkState; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetEnergyLinkDepositReductionCost() const { return ENERYLINK_DEPOSIT_REDUCTION_FACTOR; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE int GetCurrentServerStoredEnergy() const { return replicatedServerStorageMegaWattHour; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE FString GetActualServerStoredJoules() const { return replicatedServerStorageJoules; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE EApUnitSuffix GetCurrentServerStoredEnergySuffix() const { return replicatedServerStorageSuffix; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetGlobalChargeRate() const { return replicatedGlobalChargeRateMegaWatt; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FApGraphInfo> GetEnergyLinkGraphs(UFGPowerCircuit* circuit) const;

private:
	UPROPERTY(Replicated)
	EApEnergyLinkState energyLinkState = EApEnergyLinkState::Initializing;
	bool hooksInitialized = false;

	mutable FCriticalSection localStorageLock;

	FDelegateHandle hookHandlerPowerCircuitTick;
	FDelegateHandle hookHandlerCircuitSubsystemTick;

	//int is enough as we hard cap this value
	UPROPERTY(SaveGame)
	double serverAvailableMegaWattHour = 0.0f;
	UPROPERTY(SaveGame)
	double localAvailableMegaJoule = 0.0f;

	UPROPERTY(Replicated)
	FString replicatedServerStorageJoules = "0";
	UPROPERTY(Replicated)
	float replicatedServerStorageMegaWattHour = 0;
	UPROPERTY(Replicated)
	EApUnitSuffix replicatedServerStorageSuffix = EApUnitSuffix::Deci;
	UPROPERTY(Replicated)
	float replicatedGlobalChargeRateMegaWatt = 0;
	float globalChargeRateMegaWattRunningTotal = 0;

	FDelegateHandle beginPlayHookHandler;
	FDelegateHandle endPlayHookHandler;

	AApSubsystem* ap;

	void EnergyLinkTick(float deltaTime);
	void TickPowerCircuits(UFGPowerCircuitGroup* instance, float deltaTime);
	void TickCircuitSubsystem(TCallScope<void(*)(AFGCircuitSubsystem*, float)>& func, AFGCircuitSubsystem* self, float deltaTime);

	double ProcessLocalStorage();
	void SendEnergyToServer(long amount);
	void OnEnergyLinkValueChanged(AP_SetReply setReply);
	FString YankParseValueString(AP_SetReply& setReply);

	static int256 Int256FromDecimal(FString decimal);

};
