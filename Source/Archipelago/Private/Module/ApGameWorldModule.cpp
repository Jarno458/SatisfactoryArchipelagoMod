#include "Module/ApGameWorldModule.h"

DEFINE_LOG_CATEGORY(LogApGameWorldModule);

UApGameWorldModule::UApGameWorldModule() {
	ModSubsystems.Add(AApSubsystem::StaticClass());
	ModSubsystems.Add(AApEnergyLinkSubsystem::StaticClass());
	ModSubsystems.Add(AApPortalSubsystem::StaticClass());
}

void UApGameWorldModule::DispatchLifecycleEvent(ELifecyclePhase phase) {
	Super::DispatchLifecycleEvent(phase);
	UE_LOG(LogApGameWorldModule, Display, TEXT("UApGameWorldModule::DispatchLifecycleEvent"));

	USubsystemActorManager* SubsystemActorManager = GetWorld()->GetSubsystem<USubsystemActorManager>();
	check(SubsystemActorManager);

	AApSubsystem* ap = SubsystemActorManager->GetSubsystemActor<AApSubsystem>();

	ap->DispatchLifecycleEvent(phase);
}
