#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApGiftingSubsystem.h"

DEFINE_LOG_CATEGORY(LogApPortalSubsystem);

//TODO REMOVE
#pragma optimize("", off)

AApPortalSubsystem* AApPortalSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApPortalSubsystem>();
}

AApPortalSubsystem::AApPortalSubsystem() : Super() {
	UE_LOG(LogApPortalSubsystem, Display, TEXT("AApPortalSubsystem::AApPortalSubsystem()"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.05; //needs to thick atleast 13 times per seconds to keep up with a 780 belt
}

void AApPortalSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApPortalSubsystem, Display, TEXT("AApPortalSubsystem::BeginPlay()"));

	UWorld* world = GetWorld();
	giftingSubsystem = AApGiftingSubsystem::Get(world);
	mappings = AApMappingsSubsystem::Get(world);
}

void AApPortalSubsystem::Tick(float dt) {
	Super::Tick(dt);

	//if (!HasAuthority()) {
	//	return;
	//}

	if (!isInitialized) {
		RebuildQueueFromSave();

		isInitialized = true;
	} else {
		ProcessOutputQueue();
	}
}

void AApPortalSubsystem::ProcessOutputQueue() {
	if (OutputQueue.IsEmpty())
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


void AApPortalSubsystem::Enqueue(TSubclassOf<UFGItemDescriptor> cls, int amount) {
	for (size_t i = 0; i < amount; i++) {
		OutputQueue.Enqueue(FInventoryItem(cls));
	}
}

void AApPortalSubsystem::Send(FApPlayer targetPlayer, FInventoryStack itemStack) {
	((AApGiftingSubsystem*)giftingSubsystem)->EnqueueForSending(targetPlayer, itemStack);
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


void AApPortalSubsystem::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) {
	StoreQueueForSave();
}

void AApPortalSubsystem::PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) {
	RebuildQueueFromSave();
}

void AApPortalSubsystem::StoreQueueForSave() {
	FInventoryItem item;

	for (const AApPortal* portal : BuiltPortals) {
		while (portal->outputQueue.Dequeue(item)) {
			OutputQueueSave.Add(mappings->ItemClassToItemId[item.GetItemClass()]);
		}
	}
	while (OutputQueue.Dequeue(item)) {
		OutputQueueSave.Add(mappings->ItemClassToItemId[item.GetItemClass()]);
	}
}

void AApPortalSubsystem::RebuildQueueFromSave() {
	OutputQueue.Empty();

	for (int64 itemId : OutputQueueSave) {
		TSubclassOf<UFGItemDescriptor> cls = StaticCastSharedRef<FApItem>(mappings->ApItems[itemId])->Class;
		OutputQueue.Enqueue(FInventoryItem(cls));
	}

	OutputQueueSave.Empty();
}

#pragma optimize("", on)
