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

void AApSlotDataSubsystem::SetSlotDataJson(FString slotDataJson) {
	FApSlotData::ParseSlotData(slotDataJson, &slotData);
}

void AApSlotDataSubsystem::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) {
	UE_LOG(LogApSlotDataSubsystem, Display, TEXT("AApSlotDataSubsystem::PreSaveGame_Implementation(saveVersion: %i, gameVersion: %i)"), saveVersion, gameVersion);

	for (int tier = 0; tier < slotData.hubLayout.Num(); tier++) {
		for (int milestone = 0; milestone < slotData.hubLayout[tier].Num(); milestone++) {
			FApSaveableHubLayout hubLayout;
			hubLayout.tier = tier;
			hubLayout.milestone = milestone;
			hubLayout.costs = slotData.hubLayout[tier][milestone];

			saveSlotDataHubLayout.Add(hubLayout);
		}
	}
}

void AApSlotDataSubsystem::PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) {
	UE_LOG(LogApSlotDataSubsystem, Display, TEXT("AApSlotDataSubsystem::PostSaveGame_Implementation(saveVersion: %i, gameVersion: %i)"), saveVersion, gameVersion);

	saveSlotDataHubLayout.Empty();
}

//TODO fix is now fired when loading a fresh save
void AApSlotDataSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) {
	UE_LOG(LogApSlotDataSubsystem, Display, TEXT("AApSlotDataSubsystem::PostLoadGame_Implementation(saveVersion: %i, gameVersion: %i)"), saveVersion, gameVersion);

	if (!saveSlotDataHubLayout.IsEmpty() && slotData.numberOfChecksPerMilestone > 0) {
		for (FApSaveableHubLayout hubLayout : saveSlotDataHubLayout) {
			if ((slotData.hubLayout.Num() - 1) < hubLayout.tier)
				slotData.hubLayout.Add(TArray<TMap<FString, int>>());

			if ((slotData.hubLayout[hubLayout.tier].Num() - 1) < hubLayout.milestone)
				slotData.hubLayout[hubLayout.tier].Add(hubLayout.costs);
		}

		slotData.hasLoadedSlotData = true;
	}
}

#pragma optimize("", on)