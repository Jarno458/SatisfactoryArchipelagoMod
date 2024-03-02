#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApServerGiftingSubsystem.h"

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
	PrimaryActorTick.TickInterval = 0.06; //needs to thick atleast 13 times per seconds to keep up with a 780 belt

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

void AApPortalSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApPortalSubsystem, Display, TEXT("AApPortalSubsystem::BeginPlay()"));

	UWorld* world = GetWorld();
	giftingSubsystem = AApServerGiftingSubsystem::Get(world);
	mappings = AApMappingsSubsystem::Get(world);
}

void AApPortalSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!isInitialized) {
		RebuildQueueFromSave();

		isInitialized = true;
	} else {
		ProcessOutputQueue();
	}
}

void AApPortalSubsystem::ProcessOutputQueue() {
	for (AApPortal* portal : BuiltPortals) {
		if (!nextItemToOutput.IsValid())
			OutputQueue.Dequeue(nextItemToOutput);
		if (!nextItemToOutput.IsValid())
			return;
		if (portal != nullptr && portal->TrySetOutput(nextItemToOutput))
			nextItemToOutput = FInventoryItem::NullInventoryItem;
	}
}

void AApPortalSubsystem::Enqueue(TSubclassOf<UFGItemDescriptor> cls, int amount) {
	for (size_t i = 0; i < amount; i++) {
		OutputQueue.Enqueue(FInventoryItem(cls));
	}
}

void AApPortalSubsystem::Send(FApPlayer targetPlayer, FInventoryStack itemStack) {
	((AApServerGiftingSubsystem*)giftingSubsystem)->EnqueueForSending(targetPlayer, itemStack);
}

void AApPortalSubsystem::RegisterPortal(AApPortal* portal) {
	bool alreadyExists;
	BuiltPortals.Add(portal, &alreadyExists);
}

void AApPortalSubsystem::UnRegisterPortal(AApPortal* portal) {
	BuiltPortals.Remove(portal);
	
	FInventoryItem item = portal->TryStealOutput();

	if (item.IsValid()) {
		//TODO should be added to front of queue
		OutputQueue.Enqueue(item);
	}
}

void AApPortalSubsystem::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) {
	SetActorTickEnabled(false);

	StoreQueueForSave();
}

void AApPortalSubsystem::PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) {
	RebuildQueueFromSave();

	SetActorTickEnabled(true);
}

void AApPortalSubsystem::StoreQueueForSave() {
	for (AApPortal* portal : BuiltPortals) {
		if (portal == nullptr)
			continue;

		FInventoryItem item = portal->TryStealOutput();

		if (item.IsValid())
			OutputQueueSave.Add(mappings->ItemClassToItemId[item.GetItemClass()]);
	}

	FInventoryItem item;
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
