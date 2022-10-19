#include "ApGameWorldModule.h"

DEFINE_LOG_CATEGORY(ApGameWorldModule);

UApGameWorldModule::UApGameWorldModule() {
	bRootModule = true;

	ModSubsystems.Add(AApSubsystem::StaticClass());
	ModSubsystems.Add(AEnergyLinkSubsystem::StaticClass());
}

void UApGameWorldModule::DispatchLifecycleEvent(ELifecyclePhase phase) {
	Super::DispatchLifecycleEvent(phase);

	USubsystemActorManager* SubsystemActorManager = GetWorld()->GetSubsystem<USubsystemActorManager>();
	check(SubsystemActorManager);

	AApSubsystem* ap = SubsystemActorManager->GetSubsystemActor<AApSubsystem>();

	ap->DispatchLifecycleEvent(phase);
}
