#include "Subsystem/ApEnergyLinkSubsystem.h"

#include "EngineUtils.h"
#include "Misc/Char.h"
#include "Misc/ScopeLock.h"

#include "Subsystem/ApSlotDataSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Subsystem/ApServerRandomizerSubsystem.h"
#include "ApUtils.h"

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
	DOREPLIFETIME(AApEnergyLinkSubsystem, replicatedServerStorageJoules);
	DOREPLIFETIME(AApEnergyLinkSubsystem, replicatedServerStorageSuffix);

	//TODO should be FGReplicated, however that does not work across different actors, so instead we only replicate once every tick of this subsystem
	FDoRepLifetimeParams replicationParams;
	replicationParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(AApEnergyLinkSubsystem, replicatedGlobalChargeRateMegaWatt, replicationParams);
}

/*
void AApEnergyLinkSubsystem::GetConditionalReplicatedProps(const AFGBuildablePowerStorage* self, TArray<FFGCondReplicatedProperty>& outProps) const {
	FG_DOREPCONDITIONAL(ThisClass, replicatedGlobalChargeRateMegaWatt);
}
*/

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
		/*
		AFGBuildablePowerStorage* powerStorage = GetMutableDefault<AFGBuildablePowerStorage>(); // For UObject derived classes, use SUBSCRIBE_UOBJECT_METHOD instead
		hookHandlerPowerStorageGetConditionalReplicatedProps = SUBSCRIBE_METHOD_VIRTUAL_AFTER(AFGBuildablePowerStorage::GetConditionalReplicatedProps, powerStorage, [this](const AFGBuildablePowerStorage* self, TArray<FFGCondReplicatedProperty>& outProps) {
			GetConditionalReplicatedProps(self, outProps);
		});
		*/

		hooksInitialized = true;
	}
}

void AApEnergyLinkSubsystem::EndPlay(const EEndPlayReason::Type endPlayReason) {
	if (hookHandlerPowerCircuitTick.IsValid())
		UNSUBSCRIBE_METHOD(UFGPowerCircuitGroup::TickPowerCircuitGroup, hookHandlerPowerCircuitTick);
	if (hookHandlerCircuitSubsystemTick.IsValid())
		UNSUBSCRIBE_METHOD(AFGCircuitSubsystem::Tick, hookHandlerCircuitSubsystemTick);
	//if (hookHandlerPowerStorageGetConditionalReplicatedProps.IsValid())
	//	UNSUBSCRIBE_METHOD(AFGBuildablePowerStorage::GetConditionalReplicatedProps, hookHandlerPowerStorageGetConditionalReplicatedProps);
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
			ap->MonitorDataStoreValue(FString("EnergyLink") + UApUtils::FStr(apConnectionInfo->GetCurrentPlayerTeam()), AP_DataType::Raw, "0", [&](AP_SetReply setReply) {
				OnEnergyLinkValueChanged(setReply);
			});
		}
	}
	else if (energyLinkState == EApEnergyLinkState::Enabled) {
		EnergyLinkTick(DeltaTime);
	}
}

void AApEnergyLinkSubsystem::OnEnergyLinkValueChanged(AP_SetReply setReply) {
	//example 54737402054455566
	std::string valueCstr = (*(std::string*)setReply.value);
	FString rawValue = UApUtils::FStr(valueCstr).TrimStartAndEnd();

	FString intergerValueString;
	if (rawValue.IsEmpty()) {
		intergerValueString = "0";
	}
	else {
		//cut off decimals, should not happen, but we cant trust other games

		if (rawValue.Contains(".")) {
			UE_LOGFMT(LogApEnergyLink, Error, "AApEnergyLinkSubsystem::OnEnergyLinkValueChanged() received non numeric input {0}", rawValue);
			intergerValueString = rawValue.Left(rawValue.Find(".", ESearchCase::IgnoreCase, ESearchDir::FromEnd));
		}
		else {
			intergerValueString = rawValue;
		}

		if (!intergerValueString.IsNumeric()) {
			UE_LOGFMT(LogApEnergyLink, Error, "AApEnergyLinkSubsystem::OnEnergyLinkValueChanged() received non numeric input {0}", intergerValueString);
			return;
		}
	}

	//int256 is max python can store
	int256 serverValueJoules = Int256FromDecimal(intergerValueString);

	//should never happen, but we cant trust other games
	if (serverValueJoules < 0)
		serverValueJoules = 0;
	
	int256 valueForClientJoules = serverValueJoules;
	int devisions = 0;
	while (valueForClientJoules > 10000 && devisions < 7) {
		valueForClientJoules /= 1000;
		devisions++;
	}
	if (devisions >= 7) {
		replicatedServerStorageJoules = 0;
		replicatedServerStorageSuffix = EApUnitSuffix::Overflow;
	}
	else {
		// since this is interger division, it wont show decimals ever
		replicatedServerStorageJoules = valueForClientJoules.ToInt();
		replicatedServerStorageSuffix = static_cast<EApUnitSuffix>(devisions);
	}

	// correct conversion from energylink's J to battery capacity MWh * 1.000.000 * 3600, 
	// however for balance reasons use a different conversion here (dont tell the players)
	int256 serverValueMegaWattHour = serverValueJoules / (ENERGYLINK_MULTIPLIER * 3600);

	// if there is enough energy to keep us going for an other minute then we dont need to know the exact value
	if (serverValueMegaWattHour > 99900)
		serverAvailableMegaWattHour = 99900;
	else
		serverAvailableMegaWattHour = serverValueMegaWattHour.ToInt();
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


		netMegaWatt += cicuit->GetmBatterySumPowerInput(); 
	}

	globalChargeRateMegaWattRunningTotal += netMegaWatt;

	UE::TScopeLock<FCriticalSection> lock(localStorageLock);
	localAvailableMegaJoule += deltaTime * netMegaWatt; // MW * timePassedInSeconds = MJ
}

void AApEnergyLinkSubsystem::EnergyLinkTick(float deltaTime) {
	double localAvailableMegaWattHour = ProcessLocalStorage();

	float individualStorage = serverAvailableMegaWattHour;
	if (individualStorage > 1000)
		individualStorage = 1000.0f;

	float maxStorage = 90.0f;
	if (serverAvailableMegaWattHour + 10.0f >  maxStorage)
		maxStorage = serverAvailableMegaWattHour + 10;

	for (TActorIterator<AFGBuildablePowerStorage> actorItterator(GetWorld()); actorItterator; ++actorItterator) {
		AFGBuildablePowerStorage* powerStorage = *actorItterator;
		if (!IsValid(powerStorage))
			continue;

		//current building stored power
		powerStorage->mPowerStore = serverAvailableMegaWattHour + localAvailableMegaWattHour;
		//current building capacity
		powerStorage->mPowerStoreCapacity = serverAvailableMegaWattHour + localAvailableMegaWattHour + 100.0f;
		//gird capacity
		powerStorage->mBatteryInfo->mPowerStoreCapacity = serverAvailableMegaWattHour + localAvailableMegaWattHour + 100.0f;
		//grid stored power
		powerStorage->mBatteryInfo->mPowerStore = serverAvailableMegaWattHour + localAvailableMegaWattHour;
	}

	//hack to not have to constantly send this to all clients
	MARK_PROPERTY_DIRTY_FROM_NAME(AApEnergyLinkSubsystem, replicatedGlobalChargeRateMegaWatt, this);
}

double AApEnergyLinkSubsystem::ProcessLocalStorage() {
	UE::TScopeLock<FCriticalSection> lock(localStorageLock);

	if (localAvailableMegaJoule <= 0 && serverAvailableMegaWattHour == 0) {
		localAvailableMegaJoule = 0.0f; //trip powah
	}
	else if (localAvailableMegaJoule != 0.0f) {
		float intergerPart;
		long remainder = modff(localAvailableMegaJoule / ENERGYLINK_COST_DENOMINATOR, &intergerPart); //splits int and decimal parts

		localAvailableMegaJoule = (remainder * ENERGYLINK_COST_DENOMINATOR);

		//we can only send full intergers
		SendEnergyToServer(intergerPart * ENERGYLINK_COST_NUMERATOR);
	}

	return localAvailableMegaJoule / 3600; // MJ > MWh
}

void AApEnergyLinkSubsystem::SendEnergyToServer(long amountMegaJoule) {
	if (energyLinkState != EApEnergyLinkState::Enabled)
		return;

	// correct conversion from MJ to energylink's J is * 1.000.000, 
	// however for balance reasons use a different conversion here (dont tell the players)
	ap->ModdifyEnergyLink(amountMegaJoule * ENERGYLINK_MULTIPLIER); 
}

TArray<FApGraphInfo> AApEnergyLinkSubsystem::GetEnergyLinkGraphs(UFGPowerCircuit* circuit) const {
	const FLinearColor circuitChargeColor(0.2f, 0.5f, 0.1f);
	const FLinearColor circuitDrainColor(0.7f, 0.5f, 0.1f);

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

int256 AApEnergyLinkSubsystem::Int256FromDecimal(FString decimal) {
	bool negative = false;
	if (decimal.StartsWith(TEXT("-"))) {
		negative = true;
		decimal = decimal.RightChop(1);
	}

	FString reverse = decimal.Reverse();

	int256 total = 0;
	int multiplier = 0;

	for (TCHAR c : reverse) {
		if (!FChar::IsDigit(c)) {
			UE_LOGFMT(LogApEnergyLink, Error, "AApEnergyLinkSubsystem::Int256FromDecimal() received non numeric input {0} in string {1}", c, decimal);
			total = 0;
		}

		int256 multiplierValue = int256::One;
		for (int i = 0; i < multiplier; i++) {
			multiplierValue *= 10;
		}
				
		total += (multiplierValue * FChar::ConvertCharDigitToInt(c));
		multiplier++;
	}

	if (negative)
		total *= -1;

	UE_LOGFMT(LogApEnergyLink, VeryVerbose, "AApEnergyLinkSubsystem::Int256FromDecimal() validation, input: {0} > hex: {1} > validation: {2}", decimal, total.ToString(), FParse::HexNumber(*total.ToString()));

	return total;
}
#undef LOCTEXT_NAMESPACE
