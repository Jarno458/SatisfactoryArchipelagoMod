#include "Module/ApGameWorldModule.h"

DEFINE_LOG_CATEGORY(LogApGameWorldModule);

UApGameWorldModule::UApGameWorldModule() {
	ModSubsystems.Add(AApSubsystem::StaticClass());
	ModSubsystems.Add(AApEnergyLinkSubsystem::StaticClass());
	ModSubsystems.Add(AApPortalSubsystem::StaticClass());
	// Trap Subsystem is implemented in Blueprint so it is added to the subsystems array in the BP implementation of the module
}

void UApGameWorldModule::DispatchLifecycleEvent(ELifecyclePhase phase) {
	Super::DispatchLifecycleEvent(phase);
	UE_LOG(LogApGameWorldModule, Display, TEXT("UApGameWorldModule::DispatchLifecycleEvent"));

	AApSubsystem::Get(GetWorld())->DispatchLifecycleEvent(phase);
}
