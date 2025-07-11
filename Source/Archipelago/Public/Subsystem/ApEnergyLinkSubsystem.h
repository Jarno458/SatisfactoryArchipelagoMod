#pragma once

#include "CoreMinimal.h"
#include "Math/BigInt.h"

#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "FGPowerInfoComponent.h"
#include "Buildables/FGBuildablePowerStorage.h"
#include "FGPowerCircuit.h"

#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApServerRandomizerSubsystem.h"
#include "Subsystem/ApSlotDataSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Data/ApGraphs.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApEnergyLink, Log, All);

#include "ApEnergyLinkSubsystem.generated.h"

#define ENERGYLINK_MULTIPLIER 1000

UENUM(BlueprintType)
enum class EApUnitSuffix : uint8 {
	Deci,
	Kilo,
	Mega,
	Giga,
	Tera,
	Peta,
	Exa,
	Overflow UMETA(DisplayName = "Overflow")
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
	bool isInitialized = false;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool IsInitialized() const { return isInitialized; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool IsEnergyLinkEnabled() const { return energyLinkEnabled; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE int GetCurrentServerStoredEnergy() const { return replicatedServerStorageJoules; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE EApUnitSuffix GetCurrentServerStoredEnergySuffix() const { return replicatedServerStorageSuffix; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetGlobalChargeRate() const { return replicatedGlobalChargeRateMegaWattHour; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FApGraphInfo> GetEnergyLinkGraphs(UFGPowerCircuit* circuit);

private:
	bool hooksInitialized = false;
	bool energyLinkEnabled = false;

	mutable FCriticalSection localStorageLock;

	FDelegateHandle hookHandlerBatteryTick;

	//int is enough as we hard cap this value
	UPROPERTY(SaveGame)
	int serverAvailableMegaWattHour = 0;
	UPROPERTY(SaveGame)
	double localAvailableMegaJoule = 0.0f;

	UPROPERTY(Replicated)
	float replicatedServerStorageJoules = 0;
	UPROPERTY(Replicated)
	EApUnitSuffix replicatedServerStorageSuffix = EApUnitSuffix::Deci;
	UPROPERTY(Replicated)
	float replicatedGlobalChargeRateMegaWattHour = 0;

	FDelegateHandle beginPlayHookHandler;
	FDelegateHandle endPlayHookHandler;

	AApSubsystem* ap;
	AApServerRandomizerSubsystem* randomizerSubsystem;
	AApSlotDataSubsystem* slotDataSubsystem;
	AApConnectionInfoSubsystem* apConnectionInfo;

	void EnergyLinkTick(float deltaTime);
	void TickPowerCircuits(UFGPowerCircuitGroup* instance, float deltaTime);

	void HandleExcessEnergy();
	void SendEnergyToServer(long amount);
	void OnEnergyLinkValueChanged(AP_SetReply setReply);

	static int256 Int256FromDecimal(FString decimal);

};
