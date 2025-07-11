#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApServerGiftingSubsystem.h"

DEFINE_LOG_CATEGORY(LogApPortalSubsystem);

AApPortalSubsystem* AApPortalSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApPortalSubsystem>();
}

AApPortalSubsystem::AApPortalSubsystem() : Super() {
	UE_LOG(LogApPortalSubsystem, Display, TEXT("AApPortalSubsystem::AApPortalSubsystem()"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.048f; //needs to thick atleast 20 times per seconds to keep up with a 1200 belt, 0.048 is ~20.8 times per second

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
		if (IsValid(portal) && portal->TrySetOutput(nextItemToOutput))
			nextItemToOutput = FInventoryItem::NullInventoryItem;
	}
}

void AApPortalSubsystem::Enqueue(TSubclassOf<UFGItemDescriptor>& cls, int amount) {
	UE_LOG(LogApPortalSubsystem, Display, TEXT("AApPortalSubsystem::Enqueue(%s, %i)"), *UFGItemDescriptor::GetItemName(cls).ToString(), amount);

	for (int i = 0; i < amount; i++) {
		OutputQueue.Enqueue(FInventoryItem(cls));
	}
}

void AApPortalSubsystem::Send(FApPlayer targetPlayer, FInventoryStack itemStack) {
	((AApServerGiftingSubsystem*)giftingSubsystem)->EnqueueForSending(targetPlayer, itemStack);
}

void AApPortalSubsystem::RegisterPortal(AApPortal* portal) {
	if (portal == nullptr)
		return;

	bool alreadyExists;
	BuiltPortals.Add(portal, &alreadyExists);
}

void AApPortalSubsystem::UnRegisterPortal(AApPortal* portal, FInventoryItem nextItem) {
	if (portal == nullptr)
		return;

	BuiltPortals.Remove(portal);
	
	if (nextItem.IsValid()) {
		//TODO should be added to front of queue
		OutputQueue.Enqueue(nextItem);
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
