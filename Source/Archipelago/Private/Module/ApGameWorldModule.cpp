#include "Module/ApGameWorldModule.h"

DEFINE_LOG_CATEGORY(LogApGameWorldModule);

UApGameWorldModule::UApGameWorldModule() {

}

void UApGameWorldModule::DispatchLifecycleEvent(ELifecyclePhase phase) {
	Super::DispatchLifecycleEvent(phase);

	UE_LOG(LogApGameWorldModule, Display, TEXT("UApGameWorldModule()::DispatchLifecycleEvent(%s)"), *UEnum::GetValueAsString(phase));

	UWorld* world = GetWorld();
	if (!IsValid(world))	{
		UE_LOG(LogApGameWorldModule, Error, TEXT("UApGameWorldModule()::DispatchLifecycleEvent() Failed to obtain world*"));
		return;
	}

	if (phase == ELifecyclePhase::CONSTRUCTION) {
		mAdditionalSchematics.Append(mSchematics);
	}

	AApMappingsSubsystem* mappingsSubsystem = AApMappingsSubsystem::Get(world);
	if (IsValid(mappingsSubsystem))
		mappingsSubsystem->DispatchLifecycleEvent(phase);

	AApSubsystem* apSubsystem = AApSubsystem::Get(world);
	if (IsValid(apSubsystem))
		apSubsystem->DispatchLifecycleEvent(phase);

	AApServerRandomizerSubsystem* apServerRandomizerSubsystem = AApServerRandomizerSubsystem::Get(world);
	if (IsValid(apServerRandomizerSubsystem))
		apServerRandomizerSubsystem->DispatchLifecycleEvent(phase, mAdditionalSchematics);
}
