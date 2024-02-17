#include "Module/ApGameWorldModule.h"

DEFINE_LOG_CATEGORY(LogApGameWorldModule);

UApGameWorldModule::UApGameWorldModule() {

}

void UApGameWorldModule::DispatchLifecycleEvent(ELifecyclePhase phase) {
	Super::DispatchLifecycleEvent(phase);

	UE_LOG(LogApGameWorldModule, Display, TEXT("UApGameWorldModule::DispatchLifecycleEvent"));

	UWorld* world = GetWorld();

	AApMappingsSubsystem* mappingsSubsystem = AApMappingsSubsystem::Get(world);
	if (mappingsSubsystem != nullptr)
		mappingsSubsystem->DispatchLifecycleEvent(phase);

	AApSubsystem* apSubsystem = AApSubsystem::Get(world);
	if (apSubsystem != nullptr)
			apSubsystem->DispatchLifecycleEvent(phase, mSchematics);
}
