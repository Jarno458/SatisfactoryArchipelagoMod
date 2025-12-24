#include "Subsystem/ApSlotDataSubsystem.h"
#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "PushModel.h"
#include "Logging/StructuredLog.h"
#include "Subsystem/SubsystemActorManager.h"

DEFINE_LOG_CATEGORY(LogApSlotDataSubsystem);

#define LOCTEXT_NAMESPACE "Archipelago"
#define EXPECTED_SLOTDATA_VERSION 1

AApSlotDataSubsystem::AApSlotDataSubsystem() {
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

AApSlotDataSubsystem* AApSlotDataSubsystem::Get(UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApSlotDataSubsystem* AApSlotDataSubsystem::Get(UWorld* world) {
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

bool AApSlotDataSubsystem::HasLoadedSlotData() {
	slotDataState = EApSlotDataState::NotLoaded;

	if (hasLoadedSlotData && hasLoadedExplorationData) {
		slotDataState = EApSlotDataState::Ready;
		return true;
	}

	AApConnectionInfoSubsystem* connectionInfoSubsystem = AApConnectionInfoSubsystem::Get(GetWorld());
	if (connectionInfoSubsystem == nullptr) {
		UE_LOG(LogApSlotDataSubsystem, Warning, TEXT("AApSlotDataSubsystem::HasLoadedSlotData() Could not get ApConnectionInfoSubsystem"));
		return false;
	}

	if (connectionInfoSubsystem->GetConnectionState() != EApConnectionState::Connected) {
		return false;
	}

	AApSubsystem* apSubsystem = AApSubsystem::Get(GetWorld());
	if (apSubsystem == nullptr) {
		UE_LOG(LogApSlotDataSubsystem, Warning, TEXT("AApSlotDataSubsystem::HasLoadedSlotData() Could not get ApSubsystem"));
		return false;
	}

	slotDataState = TryLoadSlotDataFromServer(apSubsystem->GetSlotDataJson());

	return slotDataState == EApSlotDataState::Ready;
}

EApSlotDataState AApSlotDataSubsystem::TryLoadSlotDataFromServer(FString slotDataJson) {
	hasLoadedSlotData = false;
	hasLoadedExplorationData = false;

	if (slotDataJson.IsEmpty()) {
		lastError = LOCTEXT("SlotDataFailedToParseEmptyJson", "Failed to load SlotData - json was empty.");
		return EApSlotDataState::Failed;
	}

	const TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*slotDataJson);

	TSharedPtr<FJsonObject> parsedJson;
	FJsonSerializer::Deserialize(reader, parsedJson);

	if (!parsedJson.IsValid()) {
		UE_LOGFMT(LogApSlotDataSubsystem, Error, "AApSlotDataSubsystem::SetSlotDataJson() failed to parse slotdata: {0}", slotDataJson);
		lastError = FText::Format(LOCTEXT("SlotDataFailedToParse", "Failed to parse SlotData json: {0}"), FText::FromString(slotDataJson));
		return EApSlotDataState::Failed;
	}

	const TSharedPtr<FJsonObject>* dataJsonPointer = nullptr;
	if (!parsedJson->TryGetObjectField("Data", dataJsonPointer)) {
		UE_LOGFMT(LogApSlotDataSubsystem, Error, "AApSlotDataSubsystem::SetSlotDataJson() failed to parse slotdata: {0}", slotDataJson);
		lastError = FText::Format(LOCTEXT("SlotDataFailedToParse", "Failed to parse SlotData json: {0}"), FText::FromString(slotDataJson));
		return EApSlotDataState::Failed;
	}

	const TSharedPtr<FJsonObject> dataJson = *dataJsonPointer;

	int slotDataVersion = -1;
	if (!dataJson->TryGetNumberField("SlotDataVersion", slotDataVersion) || slotDataVersion < EXPECTED_SLOTDATA_VERSION) {
		UE_LOGFMT(LogApSlotDataSubsystem, Error, "AApSlotDataSubsystem::SetSlotDataJson() failed to parse slotdata of version {0}", slotDataVersion);
		lastError = FText::Format(LOCTEXT("SlotDataFailedVersionMissMatch", "Failed to parse SlotData of version {0}, expected version {1}"), slotDataVersion, EXPECTED_SLOTDATA_VERSION);
		return EApSlotDataState::Failed;
	}

	TArray<FApReplicatedHubLayoutEntry> parsedHubCostEntries;
	int tierNumber = 0;
	for (TSharedPtr<FJsonValue> tier : dataJson->GetArrayField("HubLayout")) {
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
	for (TPair<FString, TSharedPtr<FJsonValue>> cost : dataJson->GetObjectField("ExplorationCosts")->Values) {
		int64 itemId = FCString::Atoi64(*cost.Key);
		uint32 amount;
		cost.Value->TryGetNumber(amount);

		parsedExplorationCosts.Add(FApReplicatedCostAmount(itemId, amount));
	}
	replicatedExplorationCost = parsedExplorationCosts;
	MARK_PROPERTY_DIRTY_FROM_NAME(AApSlotDataSubsystem, replicatedExplorationCost, this);
	ReconstructExplorationCost();

	TSharedPtr<FJsonObject> options = dataJson->GetObjectField("Options");

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
	if (!options->TryGetNumberField("FinalElevatorPhase", finalSpaceElevatorTier)) {
		options->TryGetNumberField("FinalElevatorTier", finalSpaceElevatorTier); //legacy support
	}
	uint64 finalResourceSinkPoints; 
	options->TryGetNumberField("FinalResourceSinkPointsTotal", finalResourceSinkPoints);
	uint64 finalResourceSinkPointsPerMinute;
	options->TryGetNumberField("FinalResourceSinkPointsPerMinute", finalResourceSinkPointsPerMinute);

	TArray<FString> goalSelection; 
	options->TryGetStringArrayField("GoalSelection", goalSelection);
	bool isSpaceElevatorGoalEnabled = goalSelection.Contains("Space Elevator Phase") ||
												 goalSelection.Contains("Space Elevator Tier"); //legacy support
	bool isResourceSinkGoalEnabled = goalSelection.Contains("AWESOME Sink Points (total)");
	bool isResourceSinkPerMinuteGoalEnabled = goalSelection.Contains("AWESOME Sink Points (per minute)");
	bool isExplorationGoalEnabled = goalSelection.Contains("Exploration Collectables");
	bool isFicsmasGoalEnabled = goalSelection.Contains("Erect a FICSMAS Tree");

	Goals = FApGoals(requiresAnyGoal, isResourceSinkGoalEnabled, isSpaceElevatorGoalEnabled, isResourceSinkPerMinuteGoalEnabled,
		isExplorationGoalEnabled, isFicsmasGoalEnabled,
		finalSpaceElevatorTier, finalResourceSinkPoints, finalResourceSinkPointsPerMinute);

	FreeSampleEquipment = options->GetIntegerField("FreeSampleEquipment");
	FreeSampleBuildings = options->GetIntegerField("FreeSampleBuildings");
	FreeSampleParts = options->GetIntegerField("FreeSampleParts");
	FreeSampleRadioactive = options->GetBoolField("FreeSampleRadioactive");

	if (!options->TryGetBoolField("EnergyLink", EnergyLink))
		EnergyLink = false;
	if (!parsedJson->TryGetBoolField("DeathLink", DeathLink))
		DeathLink = false;

	hasLoadedSlotData = true;
	hasLoadedExplorationData = true;

	return EApSlotDataState::Ready;
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
	ReconstructExplorationCost();

	//Temp fix for old saves that with missing goals
	if (!Goals.IsSpaceElevatorGoalEnabled()
		&& !Goals.IsResourceSinkGoalEnabled()
		&& !Goals.IsResourceSinkPerMinuteGoalEnabled()
		&& !Goals.IsExplorationGoalEnabled()
		&& !Goals.IsFicsmasGoalEnabled()) {

		UE_LOG(LogApSlotDataSubsystem, Display, TEXT("AApSlotDataSubsystem::PostLoadGame_Implementation(saveVersion: %i, gameVersion: %i) No goal selected, let slot_data be decided by server"), saveVersion, gameVersion);
		//something is wrong if we dont have a goal, force load slot_data from server
		hasLoadedSlotData = false;
	}
}

#undef LOCTEXT_NAMESPACE
