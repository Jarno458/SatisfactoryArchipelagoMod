#include "Subsystem/ApPortalSubsystem.h"

AApPortalSubsystem::AApPortalSubsystem() : Super() {
	
}

void AApPortalSubsystem::BeginPlay() {
	Super::BeginPlay();
}

void AApPortalSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!HasAuthority()) {
		return;
	}

	// TODO: building registration in BuiltPortals
	// Do we want the list of BuiltPortals to be stored in saves, or be retrieved each time on save load?

	// Remove buildables from BuiltPortals that are no longer valid	

	// Do inventory operations
}
