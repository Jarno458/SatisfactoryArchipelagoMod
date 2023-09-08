#include "Module/ApGameWorldModule.h"

DEFINE_LOG_CATEGORY(LogApGameWorldModule);

UApGameWorldModule::UApGameWorldModule() {
}

void UApGameWorldModule::DispatchLifecycleEvent(ELifecyclePhase phase) {
	Super::DispatchLifecycleEvent(phase);
	UE_LOG(LogApGameWorldModule, Display, TEXT("UApGameWorldModule::DispatchLifecycleEvent"));

	AApSubsystem::Get(GetWorld())->DispatchLifecycleEvent(phase);
}
