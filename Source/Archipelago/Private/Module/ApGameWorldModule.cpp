#include "Module/ApGameWorldModule.h"

DEFINE_LOG_CATEGORY(LogApGameWorldModule);

UApGameWorldModule::UApGameWorldModule() {

}

void UApGameWorldModule::DispatchLifecycleEvent(ELifecyclePhase phase) {
	Super::DispatchLifecycleEvent(phase);

	UE_LOG(LogApGameWorldModule, Display, TEXT("UApGameWorldModule::DispatchLifecycleEvent"));

	UWorld* world = GetWorld();

	AApMappingsSubsystem* mappingsSubsystem = AApMappingsSubsystem::Get(world);
	if (IsValid(mappingsSubsystem))
		mappingsSubsystem->DispatchLifecycleEvent(phase);

	AApSubsystem* apSubsystem = AApSubsystem::Get(world);
	if (IsValid(apSubsystem))
			apSubsystem->DispatchLifecycleEvent(phase, mSchematics);
}
