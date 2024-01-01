#include "Module/ApGameWorldModule.h"

DEFINE_LOG_CATEGORY(LogApGameWorldModule);

UApGameWorldModule::UApGameWorldModule() {

}

void UApGameWorldModule::DispatchLifecycleEvent(ELifecyclePhase phase) {
	Super::DispatchLifecycleEvent(phase);

	UE_LOG(LogApGameWorldModule, Display, TEXT("UApGameWorldModule::DispatchLifecycleEvent"));

	UWorld* world = GetWorld();
	AApMappingsSubsystem::Get(world)->DispatchLifecycleEvent(phase);
	AApSubsystem::Get(world)->DispatchLifecycleEvent(phase, mSchematics);
}
