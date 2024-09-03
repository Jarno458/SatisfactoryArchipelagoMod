#include "Subsystem/ApSlotDataSubsystem.h"

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

	DOREPLIFETIME(AApSlotDataSubsystem, hubCostEntries);
	DOREPLIFETIME(AApSlotDataSubsystem, FinalSpaceElevatorTier);
	DOREPLIFETIME(AApSlotDataSubsystem, FinalResourceSinkPoints);
}

void AApSlotDataSubsystem::BeginPlay() {
	Super::BeginPlay();

	if (!hasLoadedSlotData) {
		UE_LOG(LogApSlotDataSubsystem, Error, TEXT("AApSlotDataSubsystem::BeginPlay() SlotData was not sucsesfully loaded in time"));
	}
}

void AApSlotDataSubsystem::SetSlotDataJson(FString slotDataJson) {
	const TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*slotDataJson);

	FJsonSerializer serializer;
	TSharedPtr<FJsonObject> parsedJson;

	serializer.Deserialize(reader, parsedJson);
	if (!parsedJson.IsValid()) {
		hasLoadedSlotData = false;
		return;
	}

	TArray<FApReplicatedHubLayoutEntry> parsedHubCostEntries;
	int tierNumber = 0;
	for (TSharedPtr<FJsonValue> tier : parsedJson->GetArrayField("HubLayout")) {
		int milestoneNumber = 0;
		for (TSharedPtr<FJsonValue> milestone : tier->AsArray()) {
			for (TPair<FString, TSharedPtr<FJsonValue>> cost : milestone->AsObject()->Values) {
				int64 itemId = FCString::Atoi64(*cost.Key);

				FApReplicatedHubLayoutEntry hubCostEntry;
				hubCostEntry.Pack(tierNumber, milestoneNumber, itemId, cost.Value->AsNumber());

				parsedHubCostEntries.Add(hubCostEntry);
			}

			milestoneNumber++;
		}

		tierNumber++;
	}

	hubCostEntries = parsedHubCostEntries;
	ReconstructHubLayout();

	TSharedPtr<FJsonObject> options = parsedJson->GetObjectField("Options");

	NumberOfChecksPerMilestone = parsedJson->GetIntegerField("SlotsPerMilestone");
	FinalSpaceElevatorTier = options->GetIntegerField("FinalElevatorTier");
	FinalResourceSinkPoints = options->GetIntegerField("FinalResourceSinkPoints");
	FreeSampleEquipment = options->GetIntegerField("FreeSampleEquipment");
	FreeSampleBuildings = options->GetIntegerField("FreeSampleBuildings");
	FreeSampleParts = options->GetIntegerField("FreeSampleParts");
	FreeSampleRadioactive = options->GetBoolField("FreeSampleRadioactive");

	if (!options->TryGetBoolField("EnergyLink", EnergyLink))
		EnergyLink = false;

	if (!options->TryGetBoolField("EnableHardDriveGacha", EnableHardDriveGacha))
		EnableHardDriveGacha = false;

	hasLoadedSlotData = true;
}

void AApSlotDataSubsystem::ReconstructHubLayout() {
	//reconstruct hubLayout based on saved HubCostEntries
	if (!hubCostEntries.IsEmpty()) {
		for (const FApReplicatedHubLayoutEntry& hubCostEntry : hubCostEntries) {
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


const TMap<int64, int> AApSlotDataSubsystem::GetCostsForMilestone(int tier, int milestone) {
	int8 correctedTier = tier - 1;
	int8 correctedMilestone = milestone - 1;

	if (correctedMilestone < 0) {
		auto x = 10;
	}

	fgcheck(correctedTier >= 0);
	fgcheck(correctedMilestone >= 0);

	if ((correctedTier <= (hubLayout.Num() - 1)) && (correctedMilestone <= (hubLayout[correctedTier].Num() - 1))) {
		return hubLayout[correctedTier][correctedMilestone];
	}
	else {
		return TMap<int64, int>();
	}
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

/*
void AApSlotDataSubsystem::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) {
	UE_LOG(LogApSlotDataSubsystem, Display, TEXT("AApSlotDataSubsystem::PreSaveGame_Implementation(saveVersion: %i, gameVersion: %i)"), saveVersion, gameVersion);

	for (int tier = 0; tier < slotData.hubLayout.Num(); tier++) {
		for (int milestone = 0; milestone < slotData.hubLayout[tier].Num(); milestone++) {
			for (const TPair<int64, int>& milestoneCostEntry : slotData.hubLayout[tier][milestone]) {
				FApReplicatedHubLayoutEntry hubCostEntry;
				hubCostEntry.Pack(tier, milestone, milestoneCostEntry.Key, milestoneCostEntry.Value);

				saveSlotDataHubLayout.Add(hubCostEntry);
			}
		}
	}
}

void AApSlotDataSubsystem::PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) {
	UE_LOG(LogApSlotDataSubsystem, Display, TEXT("AApSlotDataSubsystem::PostSaveGame_Implementation(saveVersion: %i, gameVersion: %i)"), saveVersion, gameVersion);

	//saveSlotDataHubLayout.Empty();
}
*/

//TODO fix is now fired when loading a fresh save
void AApSlotDataSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) {
	UE_LOG(LogApSlotDataSubsystem, Display, TEXT("AApSlotDataSubsystem::PostLoadGame_Implementation(saveVersion: %i, gameVersion: %i)"), saveVersion, gameVersion);

	ReconstructHubLayout();
}

#pragma optimize("", on)