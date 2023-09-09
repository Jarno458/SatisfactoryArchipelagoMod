#include "Subsystem/ApPortalSubsystem.h"

DEFINE_LOG_CATEGORY(LogApPortalSubsystem);

//TODO REMOVE
#pragma optimize("", off)

AApPortalSubsystem* AApPortalSubsystem::Get() {
	return Get(GEngine->GameViewport->GetWorld());
}

AApPortalSubsystem* AApPortalSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	check(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApPortalSubsystem>();
}

AApPortalSubsystem::AApPortalSubsystem() : Super() {
	UE_LOG(LogApPortalSubsystem, Display, TEXT("AApPortalSubsystem::AApPortalSubsystem()"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0;
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

	if(OutputQueue.IsEmpty())
		return;

	for (const AApPortal* portal : BuiltPortals) {
		if (OutputQueue.IsEmpty())
			return;

		if (portal->CanReceiveOutput() && portal->outputQueue.IsEmpty()) {
			FInventoryItem item;
			OutputQueue.Dequeue(item);
			portal->outputQueue.Enqueue(item);
		}
	}
}

void AApPortalSubsystem::Enqueue(TSubclassOf<UFGItemDescriptor> cls) {
	int stackSize = UFGItemDescriptor::GetStackSize(cls);

	for (size_t i = 0; i < stackSize; i++) {
		OutputQueue.Enqueue(FInventoryItem(cls));
	}
}

void AApPortalSubsystem::RegisterPortal(const AApPortal* portal) {
	bool alreadyExists;
	BuiltPortals.Add(portal, &alreadyExists);
}

void AApPortalSubsystem::UnRegisterPortal(const AApPortal* portal) {
	BuiltPortals.Remove(portal);
	
	//TODO should be added to front of queue
	FInventoryItem item;
	while (portal->outputQueue.Dequeue(item))
		OutputQueue.Enqueue(item);
}

#pragma optimize("", on)
