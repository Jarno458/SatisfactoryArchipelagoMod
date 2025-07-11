#include "Subsystem/ApMamTreeSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "ApUtils.h"

DEFINE_LOG_CATEGORY(LogApMamTreeSubsystem);

AApMamTreeSubsystem::AApMamTreeSubsystem() : Super() {
	PrimaryActorTick.bCanEverTick = false;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

AApMamTreeSubsystem* AApMamTreeSubsystem::Get(UWorld* world) {
	return UApUtils::GetSubsystemActorIncludingParentClases<AApMamTreeSubsystem>(world);
}
