#include "Subsystem/ApSlotDataSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "PushModel.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(LogApSlotDataSubsystem);

//TODO REMOVE
#pragma optimize("", off)

AApSlotDataSubsystem::AApSlotDataSubsystem() {
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

AApSlotDataSubsystem* AApSlotDataSubsystem::Get(class UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApSlotDataSubsystem* AApSlotDataSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApSlotDataSubsystem>();
}

void AApSlotDataSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams replicationParams;
	replicationParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AApSlotDataSubsystem, hubCostEntriesReplicated, replicationParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AApSlotDataSubsystem, replicatedExplorationCost, replicationParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AApSlotDataSubsystem, starterRecipeIds, replicationParams);
	DOREPLIFETIME(AApSlotDataSubsystem, Goals);
}

void AApSlotDataSubsystem::BeginPlay() {
	Super::BeginPlay();

	if (!hasLoadedSlotData || !hasLoadedExplorationData) {
		UE_LOGFMT(LogApSlotDataSubsystem, Error, "AApSlotDataSubsystem::BeginPlay() SlotData was not sucsesfully loaded in time");
	}
}

void AApSlotDataSubsystem::SetSlotDataJson(FString slotDataJson) {
	if (slotDataJson.IsEmpty())
		return;

	hasLoadedSlotData = false;
	hasLoadedExplorationData = false;

	const TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*slotDataJson);

	FJsonSerializer serializer;
	TSharedPtr<FJsonObject> parsedJson;

	serializer.Deserialize(reader, parsedJson);
	if (!parsedJson.IsValid()) {
		UE_LOGFMT(LogApSlotDataSubsystem, Error, "AApSlotDataSubsystem::SetSlotDataJson() failed to parse slotdata: {0}", slotDataJson);
		return;
	}

	int slotDataVersion = -1;
	if (!parsedJson->TryGetNumberField("SlotDataVersion", slotDataVersion) || slotDataVersion < 1) {
		UE_LOGFMT(LogApSlotDataSubsystem, Error, "AApSlotDataSubsystem::SetSlotDataJson() failed to parse slotdata of version {0}", slotDataVersion);
		return;
	}

	TArray<FApReplicatedHubLayoutEntry> parsedHubCostEntries;
	int tierNumber = 0;
	for (TSharedPtr<FJsonValue> tier : parsedJson->GetArrayField("HubLayout")) {
		int milestoneNumber = 0;
		for (TSharedPtr<FJsonValue> milestone : tier->AsArray()) {
			for (TPair<FString, TSharedPtr<FJsonValue>> cost : milestone->AsObject()->Values) {
				int64 itemId = FCString::Atoi64(*cost.Key);
				uint32 amount;
				cost.Value->TryGetNumber(amount);

				parsedHubCostEntries.Add(FApReplicatedHubLayoutEntry(tierNumber, milestoneNumber, itemId, amount));
			}

			milestoneNumber++;
		}

		tierNumber++;
	}
	hubCostEntriesReplicated = parsedHubCostEntries;
	MARK_PROPERTY_DIRTY_FROM_NAME(AApSlotDataSubsystem, hubCostEntriesReplicated, this);
	ReconstructHubLayout();

	TArray<FApReplicatedCostAmount> parsedExplorationCosts;
	for (TPair<FString, TSharedPtr<FJsonValue>> cost : parsedJson->GetObjectField("ExplorationCosts")->Values) {
		int64 itemId = FCString::Atoi64(*cost.Key);
		uint32 amount;
		cost.Value->TryGetNumber(amount);

		parsedExplorationCosts.Add(FApReplicatedCostAmount(itemId, amount));
	}
	replicatedExplorationCost = parsedExplorationCosts;
	MARK_PROPERTY_DIRTY_FROM_NAME(AApSlotDataSubsystem, replicatedExplorationCost, this);
	ReconstructExplorationCost();

	TSharedPtr<FJsonObject> options = parsedJson->GetObjectField("Options");

	TArray<TSharedPtr<FJsonValue>> starting_items_array = options->GetArrayField("StartingRecipies");
	starterRecipeIds.SetNum(starting_items_array.Num());
	for (size_t i = 0; i < starting_items_array.Num(); i++)
	{
		int64 itemId;
		starting_items_array[i]->TryGetNumber(itemId);
		starterRecipeIds[i] = itemId;
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(AApSlotDataSubsystem, starterRecipeIds, this);

	//using Try methods as it allow for unsinged ints
	int goalRequirement = options->GetIntegerField("GoalRequirement");
	bool requiresAnyGoal = goalRequirement == 0;

	uint8 finalSpaceElevatorTier;
	options->TryGetNumberField("FinalElevatorTier", finalSpaceElevatorTier);
	uint64 finalResourceSinkPoints; 
	options->TryGetNumberField("FinalResourceSinkPointsTotal", finalResourceSinkPoints);
	uint64 finalResourceSinkPointsPerMinute;
	options->TryGetNumberField("FinalResourceSinkPointsPerMinute", finalResourceSinkPointsPerMinute);

	TArray<FString> goalSelection; 
	options->TryGetStringArrayField("GoalSelection", goalSelection);
	bool isSpaceElevatorGoalEnabled = goalSelection.Contains("Space Elevator Tier");
	bool isResourceSinkGoalEnabled = goalSelection.Contains("AWESOME Sink Points (total)");
	bool isResourceSinkPerMinuteGoalEnabled = goalSelection.Contains("AWESOME Sink Points (per minute)");
	bool isExplorationGoalEnabled = goalSelection.Contains("Exploration Collectables");
	bool isFicsmasGoalEnabled = goalSelection.Contains("Erect a FICSMAS Tree");

	Goals = FApGoals(requiresAnyGoal, isResourceSinkGoalEnabled, isSpaceElevatorGoalEnabled, isResourceSinkPerMinuteGoalEnabled,
		isExplorationGoalEnabled, isFicsmasGoalEnabled,
		finalSpaceElevatorTier, finalResourceSinkPoints, finalResourceSinkPointsPerMinute);

	NumberOfChecksPerMilestone = parsedJson->GetIntegerField("SlotsPerMilestone");
	FreeSampleEquipment = options->GetIntegerField("FreeSampleEquipment");
	FreeSampleBuildings = options->GetIntegerField("FreeSampleBuildings");
	FreeSampleParts = options->GetIntegerField("FreeSampleParts");
	FreeSampleRadioactive = options->GetBoolField("FreeSampleRadioactive");

	if (!options->TryGetBoolField("EnergyLink", EnergyLink))
		EnergyLink = false;

	hasLoadedSlotData = true;
	hasLoadedExplorationData = true;
}

void AApSlotDataSubsystem::ReconstructHubLayout() {
	//reconstruct hubLayout based on saved HubCostEntries
	if (!hubCostEntriesReplicated.IsEmpty()) {
		for (const FApReplicatedHubLayoutEntry& hubCostEntry : hubCostEntriesReplicated) {
			int tier = hubCostEntry.GetTier();
			int milestone = hubCostEntry.GetMilestone();

			if ((hubLayout.Num() - 1) < tier)
				hubLayout.Add(TArray<TMap<int64, int>>());

			if ((hubLayout[tier].Num() - 1) < milestone)
				hubLayout[tier].Add(TMap<int64, int>());

			hubLayout[tier][milestone].Add(hubCostEntry.GetItemId(), hubCostEntry.GetAmount());
		}

		hasLoadedSlotData = true;
	}
}

void AApSlotDataSubsystem::ReconstructExplorationCost() {
	if (!replicatedExplorationCost.IsEmpty()) {
		for (const FApReplicatedCostAmount& costEntry : replicatedExplorationCost) {
			explorationCosts.Add(costEntry.GetItemId(), costEntry.GetAmount());
		}

		hasLoadedExplorationData = true;
	}
}

const TMap<int64, int> AApSlotDataSubsystem::GetCostsForMilestone(int tier, int milestone) {
	int8 correctedTier = tier - 1;
	int8 correctedMilestone = milestone - 1;

	fgcheck(correctedTier >= 0);
	fgcheck(correctedMilestone >= 0);

	if ((correctedTier <= (hubLayout.Num() - 1)) && (correctedMilestone <= (hubLayout[correctedTier].Num() - 1))) {
		return hubLayout[correctedTier][correctedMilestone];
	}
	else {
		return TMap<int64, int>();
	}
}

const TMap<int64, int> AApSlotDataSubsystem::GetExplorationGoalCosts() {
	return explorationCosts;
}

int AApSlotDataSubsystem::GetNumberOfHubTiers() {
	return hubLayout.Num();
}

int AApSlotDataSubsystem::GetNumberOfMilestonesForTier(int tier) {
	int8 correctedTier = tier - 1;

	if (correctedTier <= (hubLayout.Num() - 1)) {
		return hubLayout[correctedTier].Num();
	}

	return 0;
}

TArray<int64> AApSlotDataSubsystem::GetStarterRecipeIds() {
	return starterRecipeIds;
}

// is also fired when loading a fresh save
void AApSlotDataSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) {
	UE_LOG(LogApSlotDataSubsystem, Display, TEXT("AApSlotDataSubsystem::PostLoadGame_Implementation(saveVersion: %i, gameVersion: %i)"), saveVersion, gameVersion);

	ReconstructHubLayout();
}

#pragma optimize("", on)