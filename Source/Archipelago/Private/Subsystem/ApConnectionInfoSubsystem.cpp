#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogApConnectionInfoSubsystem);

#define LOCTEXT_NAMESPACE "Archipelago"

AApConnectionInfoSubsystem::AApConnectionInfoSubsystem() {
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;

	ConnectionState = EApConnectionState::NotYetAttempted;
	ConnectionStateDescription = LOCTEXT("NotYetAttempted", "A connection has not yet been attempted. Load a save file to attempt to connect.");
}

AApConnectionInfoSubsystem* AApConnectionInfoSubsystem::Get(class UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApConnectionInfoSubsystem* AApConnectionInfoSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApConnectionInfoSubsystem>();
}

void AApConnectionInfoSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AApConnectionInfoSubsystem, ConnectionState);
	DOREPLIFETIME(AApConnectionInfoSubsystem, ConnectionStateDescription);
	DOREPLIFETIME(AApConnectionInfoSubsystem, currentPlayerTeam);
	DOREPLIFETIME(AApConnectionInfoSubsystem, currentPlayerSlot);
}

#undef LOCTEXT_NAMESPACE
