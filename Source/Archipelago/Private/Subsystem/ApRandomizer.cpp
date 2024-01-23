#include "Subsystem/ApRandomizerSubsystem.h"

DEFINE_LOG_CATEGORY(LogApRandomizerSubsystem);

//TODO REMOVE
#pragma optimize("", off)

#define LOCTEXT_NAMESPACE "Archipelago"

AApRandomizerSubsystem::AApRandomizerSubsystem() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.5f;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer; // TODO_MULTIPLAYER is this what we want long term?
}

AApRandomizerSubsystem* AApRandomizerSubsystem::Get(class UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApRandomizerSubsystem* AApRandomizerSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApRandomizerSubsystem>();
}

#pragma optimize("", on)

#undef LOCTEXT_NAMESPACE
