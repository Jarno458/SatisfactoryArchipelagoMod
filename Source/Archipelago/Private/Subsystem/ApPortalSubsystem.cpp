#include "Subsystem/ApPortalSubsystem.h"

DEFINE_LOG_CATEGORY(LogApPortalSubsystem);

AApPortalSubsystem::AApPortalSubsystem() : Super() {
	UE_LOG(LogApPortalSubsystem, Display, TEXT("AApPortalSubsystem::AApPortalSubsystem()"));
}

void AApPortalSubsystem::BeginPlay() {
	Super::BeginPlay();
	UE_LOG(LogApPortalSubsystem, Display, TEXT("AApPortalSubsystem::BeginPlay()"));
}

void AApPortalSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!HasAuthority()) {
		return;
	}

	// TODO: building registration to BuiltPortals array
	// Do we want the list of BuiltPortals to be stored in saves, or be retrieved each time on save load?

	// Remove buildables from BuiltPortals that are no longer valid	

	// Do inventory operations
}
