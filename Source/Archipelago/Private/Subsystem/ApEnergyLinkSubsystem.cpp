#include "Subsystem/ApEnergyLinkSubsystem.h"

#include "EngineUtils.h"
#include "Misc/Char.h"
#include "Misc/ScopeLock.h"

#include "Subsystem/ApSlotDataSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Subsystem/ApServerRandomizerSubsystem.h"
#include "ApUtils.h"
#include "UnrealNetwork.h"

#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(LogApEnergyLink);

#define LOCTEXT_NAMESPACE "Archipelago"

AApEnergyLinkSubsystem::AApEnergyLinkSubsystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

AApEnergyLinkSubsystem* AApEnergyLinkSubsystem::Get(UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApEnergyLinkSubsystem>();
}

AApEnergyLinkSubsystem* AApEnergyLinkSubsystem::Get(class UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

void AApEnergyLinkSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AApEnergyLinkSubsystem, energyLinkState);
	DOREPLIFETIME(AApEnergyLinkSubsystem, replicatedServerStorageMegaWattHour);
	DOREPLIFETIME(AApEnergyLinkSubsystem, replicatedServerStorageSuffix);

	//should be FGReplicated, however that does not work across different actors, so instead we only replicate once every tick of this subsystem
	FDoRepLifetimeParams replicationParams;
	replicationParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(AApEnergyLinkSubsystem, replicatedGlobalChargeRateMegaWatt, replicationParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AApEnergyLinkSubsystem, replicatedServerStorageJoules, replicationParams);
}

void AApEnergyLinkSubsystem::BeginPlay() {
	Super::BeginPlay();

	if (!HasAuthority())
		return;
		
	UE_LOGFMT(LogApEnergyLink, Display, "AEnergyLinkSubsystem:BeginPlay()");

	ap = AApSubsystem::Get(GetWorld());
	fgcheck(ap);

	if (!hooksInitialized && !WITH_EDITOR) {
		hookHandlerPowerCircuitTick = SUBSCRIBE_METHOD_AFTER(UFGPowerCircuitGroup::TickPowerCircuitGroup, [this](UFGPowerCircuitGroup* self, float deltaTime) {
			TickPowerCircuits(self, deltaTime);
		});
		AFGCircuitSubsystem* circuitSubsystem = GetMutableDefault<AFGCircuitSubsystem>(); // For UObject derived classes, use SUBSCRIBE_UOBJECT_METHOD instead
		hookHandlerCircuitSubsystemTick = SUBSCRIBE_METHOD_VIRTUAL(AFGCircuitSubsystem::Tick, circuitSubsystem, [this](auto& func, AFGCircuitSubsystem* self, float deltaTime) {
			TickCircuitSubsystem(func, self, deltaTime);
		});

		hooksInitialized = true;
	}
}

void AApEnergyLinkSubsystem::EndPlay(const EEndPlayReason::Type endPlayReason) {
	if (hookHandlerPowerCircuitTick.IsValid())
		UNSUBSCRIBE_METHOD(UFGPowerCircuitGroup::TickPowerCircuitGroup, hookHandlerPowerCircuitTick);
	if (hookHandlerCircuitSubsystemTick.IsValid())
		UNSUBSCRIBE_METHOD(AFGCircuitSubsystem::Tick, hookHandlerCircuitSubsystemTick);
}

void AApEnergyLinkSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (!HasAuthority())
		return;

	if (energyLinkState == EApEnergyLinkState::Initializing) {
		UWorld* world = GetWorld();

		AApSlotDataSubsystem* slotDataSubsystem = AApSlotDataSubsystem::Get(world);
		AApConnectionInfoSubsystem* apConnectionInfo = AApConnectionInfoSubsystem::Get(world);
		AApServerRandomizerSubsystem* apRandoSubsystem = AApServerRandomizerSubsystem::Get(world);

		if (!IsValid(slotDataSubsystem) 
			|| !IsValid(apConnectionInfo) 
			|| !IsValid(apRandoSubsystem) 
			|| !slotDataSubsystem->HasLoadedSlotData()
			|| !apRandoSubsystem->IsInitialized())
			return;

		if (slotDataSubsystem->EnergyLink) {
			EApConnectionState connectionState = apConnectionInfo->GetConnectionState();
			if (connectionState == EApConnectionState::Connecting || connectionState == EApConnectionState::NotYetAttempted) {
				energyLinkState = EApEnergyLinkState::Initializing;
			} else if (connectionState == EApConnectionState::Connected) {
				energyLinkState = EApEnergyLinkState::Enabled;
				PrimaryActorTick.TickInterval = 5.0f;
			} else {
				energyLinkState = EApEnergyLinkState::Unavailable;
				PrimaryActorTick.TickInterval = 5.0f;
			}
		} else {
			energyLinkState = EApEnergyLinkState::Disabled;
			SetActorTickEnabled(false);
		}

		if (energyLinkState == EApEnergyLinkState::Enabled) {
			FString key = FString("EnergyLink") + UApUtils::FStr(apConnectionInfo->GetCurrentPlayerTeam());
			ap->MonitorUnboundedIntergerDataStoreValue(key, [&](AP_SetReply setReply) {
				OnEnergyLinkValueChanged(setReply);
			});
		}
	}
	else if (energyLinkState == EApEnergyLinkState::Enabled) {
		EnergyLinkTick(DeltaTime);
	}
}

void AApEnergyLinkSubsystem::OnEnergyLinkValueChanged(AP_SetReply setReply) {
	FString intergerValueString = UApUtils::YankParseValueString(setReply);

	replicatedServerStorageJoules = intergerValueString;

	//int256 is max python can store
	// correct conversion from energylink's J to battery capacity MWh * 1.000.000 * 3600, 
	// however for balance reasons use a different conversion here (dont tell the players)
	int256 remainder;
	int256 serverValueMegaWattHour = UApUtils::Int256FromBigIntString(intergerValueString);
	serverValueMegaWattHour.DivideWithRemainder(ENERGYLINK_MULTIPLIER * 3600, remainder);

	double remainderMegaWattHour = (double)remainder.ToInt() / (ENERGYLINK_MULTIPLIER * 3600);

	//should never happen, but we cant trust other games
	if (serverValueMegaWattHour.IsNegative()) {
		serverValueMegaWattHour = 0;
		remainderMegaWattHour = 0;
	}

	int256 valueForClientMegaWattHour;
	int devisions = 0;

	if (serverValueMegaWattHour > 0) {
		valueForClientMegaWattHour = serverValueMegaWattHour;
		devisions = (int)EApUnitSuffix::Mega;
	} else if (remainderMegaWattHour > 0) {
		valueForClientMegaWattHour = 1000000.0f * remainderMegaWattHour;
		devisions = (int)EApUnitSuffix::Deci;
	}

	while (valueForClientMegaWattHour > 10000 && devisions < 7) {
		valueForClientMegaWattHour /= 1000;
		devisions++;
	}

	if (devisions >= 7) {
		replicatedServerStorageMegaWattHour = 0;
		replicatedServerStorageSuffix = EApUnitSuffix::Overflow;
	} else {
		// since this is interger division, it wont show decimals ever
		replicatedServerStorageMegaWattHour = valueForClientMegaWattHour.ToInt();
		replicatedServerStorageSuffix = static_cast<EApUnitSuffix>(devisions);
	}

	// if there is enough energy to keep us going for an other minute then we dont need to know the exact value
	if (serverValueMegaWattHour > (ENERGYLINK_STORE_CAPACITY - 100))
		serverAvailableMegaWattHour = (ENERGYLINK_STORE_CAPACITY - 100);
	else
		serverAvailableMegaWattHour = serverValueMegaWattHour.ToInt() + remainderMegaWattHour;
}

void AApEnergyLinkSubsystem::TickCircuitSubsystem(TCallScope<void(*)(AFGCircuitSubsystem*, float)>& func, AFGCircuitSubsystem* self, float deltaTime) {
	globalChargeRateMegaWattRunningTotal = 0;

	func(self, deltaTime);

	replicatedGlobalChargeRateMegaWatt = globalChargeRateMegaWattRunningTotal;
}

void AApEnergyLinkSubsystem::TickPowerCircuits(UFGPowerCircuitGroup* instance, float deltaTime) {
	double netMegaWatt = 0.0f;

	if (energyLinkState != EApEnergyLinkState::Enabled
		|| !IsValid(instance))
		return;

	for (UFGPowerCircuit* cicuit : instance->mCircuits) {
		if (!IsValid(cicuit)
			|| cicuit->IsFuseTriggered()
			|| !cicuit->HasBatteries())
			continue;

		float sumPowerInput = cicuit->GetmBatterySumPowerInput();

		netMegaWatt += sumPowerInput;

		if (sumPowerInput > 0.0f)
			cicuit->SetmTimeToBatteriesFull(0.0f);

		cicuit->SetmBatterySumPowerStoreCapacity(0.0f);
	}

	globalChargeRateMegaWattRunningTotal += netMegaWatt;

	UE::TScopeLock<FCriticalSection> lock(localStorageLock);
	localAvailableMegaJoule += deltaTime * netMegaWatt; // MW * timePassedInSeconds = MJ
}

void AApEnergyLinkSubsystem::EnergyLinkTick(float deltaTime) {
	double localAvailableMegaWattHour = ProcessLocalStorage();

	float stored = serverAvailableMegaWattHour + localAvailableMegaWattHour;
	if (serverAvailableMegaWattHour + localAvailableMegaWattHour > ENERGYLINK_STORE_CAPACITY)
		serverAvailableMegaWattHour -= localAvailableMegaWattHour;

	//TODO if globalChargeRateMegaWattRunningTotal > 0 then a fuse should not pop

	for (TActorIterator<AFGBuildablePowerStorage> actorItterator(GetWorld()); actorItterator; ++actorItterator) {
		AFGBuildablePowerStorage* powerStorage = *actorItterator;
		if (!IsValid(powerStorage))
			continue;

		//current building stored power
		powerStorage->mPowerStore = stored;
		//grid stored power
		powerStorage->mBatteryInfo->mPowerStore = stored;

		// serverAvailableMegaWattHour is capped at 99900.0f
		//current building capacity
		powerStorage->mPowerStoreCapacity = ENERGYLINK_STORE_CAPACITY;
		//gird capacity
		powerStorage->mBatteryInfo->mPowerStoreCapacity = ENERGYLINK_STORE_CAPACITY;
	}

	//hack to not have to constantly send this to all clients
	MARK_PROPERTY_DIRTY_FROM_NAME(AApEnergyLinkSubsystem, replicatedGlobalChargeRateMegaWatt, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(AApEnergyLinkSubsystem, replicatedServerStorageJoules, this);
}

double AApEnergyLinkSubsystem::ProcessLocalStorage() {
	int wholeIntsToSend = 0;
	double returnValue = 0.0f;

	{
		UE::TScopeLock<FCriticalSection> lock(localStorageLock);

		if (localAvailableMegaJoule <= 0 && serverAvailableMegaWattHour == 0) {
			localAvailableMegaJoule = 0.0f; //trip powah
			returnValue = 0.0f;
		}
		else if (localAvailableMegaJoule != 0.0f) {
			wholeIntsToSend = (int)(localAvailableMegaJoule * ENERYLINK_DEPOSIT_REDUCTION_FACTOR);

			localAvailableMegaJoule -= wholeIntsToSend * (1 / ENERYLINK_DEPOSIT_REDUCTION_FACTOR);

			returnValue = localAvailableMegaJoule / 3600; // MJ > MWh
		}
		else {
			returnValue = localAvailableMegaJoule / 3600; // MJ > MWh
		}
	}

	if (wholeIntsToSend != 0)
		SendEnergyToServer(wholeIntsToSend);

	return returnValue;
}

void AApEnergyLinkSubsystem::SendEnergyToServer(long amountMegaJoule) const {
	if (energyLinkState != EApEnergyLinkState::Enabled)
		return;

	// correct conversion from MJ to energylink's J is * 1.000.000, 
	// however for balance reasons use a different conversion here (dont tell the players)
	ap->ModifyEnergyLink(static_cast<int64>(amountMegaJoule) * ENERGYLINK_MULTIPLIER); 
}

TArray<FApGraphInfo> AApEnergyLinkSubsystem::GetEnergyLinkGraphs(UFGPowerCircuit* circuit) const {
	const FLinearColor circuitChargeColor(0.2f, 0.5f, 0.1f);
	const FLinearColor circuitDrainColor(1.0f, 0.2f, 0.1f);

	TArray<FApGraphInfo> graphs;

	if (energyLinkState != EApEnergyLinkState::Enabled || !IsValid(circuit) || !circuit->HasBatteries())
		return graphs;
	
	FPowerCircuitStats powerGraph;
	circuit->GetStats(powerGraph);

	TArray<float> batteryDataPoints;
	powerGraph.GetPoints<&FPowerGraphPoint::BatteryPowerInput>(batteryDataPoints);

	TArray<float> chargeDataPoints;
	TArray<float> drainDataPoints;

	bool hasCharged = false;
	bool hasDrained = false;

	for (float dataPoint : batteryDataPoints) {
		if (dataPoint >= 0.0f) {
			hasCharged = true;

			chargeDataPoints.Add(dataPoint);
			drainDataPoints.Add(0.0f);
		} else {
			hasDrained = true;

			chargeDataPoints.Add(0.0f);
			drainDataPoints.Add(dataPoint * -1);
		}
	}

	if (hasCharged) {
		FApGraphInfo graph;

		graph.Id = FString(TEXT("ELC"));
		graph.Suffix = FText::FromString(TEXT("MW"));
		graph.FullName = FText::FromString(TEXT("Enerylink Charge"));
		graph.Description = FText::FromString(TEXT("How much power is added to the EnergyLink"));
		graph.Color = circuitChargeColor;
		graph.DisplayName = FText::FromString(TEXT("EnergyLink Charging"));
		graph.DataPoints = chargeDataPoints;

		graphs.Add(graph);
	}
	if (hasDrained) {
		FApGraphInfo graph;

		graph.Id = FString(TEXT("ELD"));
		graph.Suffix = FText::FromString(TEXT("MW"));
		graph.FullName = FText::FromString(TEXT("Enerylink Drain"));
		graph.Description = FText::FromString(TEXT("How much power is drained from the EnergyLink"));
		graph.Color = circuitDrainColor;
		graph.DisplayName = FText::FromString(TEXT("EnergyLink Draining"));
		graph.DataPoints = drainDataPoints;

		graphs.Add(graph);
	}

	return graphs;
}

#undef LOCTEXT_NAMESPACE
