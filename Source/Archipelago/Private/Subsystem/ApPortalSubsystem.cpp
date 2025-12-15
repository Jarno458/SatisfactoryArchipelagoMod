#include "Subsystem/ApPortalSubsystem.h"
#include "Containers/List.h"
#include "EngineUtils.h"
#include "Buildable/ApPortal.h"
#include "Subsystem/ApServerGiftingSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"

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

	//needs to thick atleast 1200 times per minute to keep up with a 1200 belt, 1200 / 60sec = 20x per second
	//0.048 is ~20.8 times per second
	PrimaryActorTick.TickInterval = 0.048f; 

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

void AApPortalSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApPortalSubsystem, Display, TEXT("AApPortalSubsystem::BeginPlay()"));

	UWorld* world = GetWorld();
	giftingSubsystem = AApServerGiftingSubsystem::Get(world);
	mappings = AApMappingsSubsystem::Get(world);
	connectionInfoSubsystem = AApConnectionInfoSubsystem::Get(world);

	//connectionInfoSubsystem->GetCurrentPlayerTeam(), connectionInfoSubsystem->GetCurrentPlayerSlot()
}

void AApPortalSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!isInitialized) {
		//if (connectionInfoSubsystem->GetConnectionState() == EApConnectionState::Connected) {

		//}

		RebuildQueueFromSave();

		isInitialized = true;
	} else {
		ProcessPendingOutputQueue();
		SendOutputQueueToPortals();
	}
}

void AApPortalSubsystem::ProcessPendingOutputQueue() {
	FInventoryItem pendingItem;

	while (PriorityPendingOutputQueue.Dequeue(pendingItem)) {
		AddToStartOfQueue(pendingItem);
	}

	while (PendingOutputQueue.Dequeue(pendingItem)) {
		AddToEndOfQueue(pendingItem);
	}
}

void AApPortalSubsystem::SendOutputQueueToPortals() {
	for (TActorIterator<AApPortal> actorItterator(GetWorld()); actorItterator; ++actorItterator) {
		AApPortal* portal = *actorItterator;
		if (!IsValid(portal))
			continue;

		if (!nextItemToOutput.IsValid())
			TryPopFromQueue(nextItemToOutput);
		if (!nextItemToOutput.IsValid())
			return;

		if (portal->TrySetOutput(nextItemToOutput))
			nextItemToOutput = FInventoryItem::NullInventoryItem;
	}
}

void AApPortalSubsystem::Enqueue(TSubclassOf<UFGItemDescriptor>& cls, int amount) {
	UE_LOG(LogApPortalSubsystem, Display, TEXT("AApPortalSubsystem::Enqueue(%s, %i)"), *UFGItemDescriptor::GetItemName(cls).ToString(), amount);

	const FInventoryItem item(cls);

	for (int i = 0; i < amount; i++) {
		PendingOutputQueue.Enqueue(item);
	}
}

void AApPortalSubsystem::Send(FApPlayer targetPlayer, FInventoryStack itemStack) {
	//TODO send items for self directly to output queue instead of gifting subsystem

	if (!targetPlayer.IsValid())
		return;

	if (false) { //TODO check if target player is self
		TSubclassOf<UFGItemDescriptor> cls = itemStack.Item.GetItemClass();
		Enqueue(cls, itemStack.NumItems);
	} else {
		((AApServerGiftingSubsystem*)giftingSubsystem)->EnqueueForSending(targetPlayer, itemStack);
	}
}

void AApPortalSubsystem::ReQueue(FInventoryItem nextItem) {
	if (nextItem.IsValid()) {
		PriorityPendingOutputQueue.Enqueue(nextItem);
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
	while (TryPopFromQueue(item)) {
		OutputQueueSave.Add(mappings->ItemClassToItemId[item.GetItemClass()]);
	}
}

void AApPortalSubsystem::RebuildQueueFromSave() {
	ClearQueue();

	for (int64 itemId : OutputQueueSave) {
		TSubclassOf<UFGItemDescriptor> cls = StaticCastSharedRef<FApItem>(mappings->ApItems[itemId])->Class;
		AddToEndOfQueue(FInventoryItem(cls));
	}

	OutputQueueSave.Empty();
}

void AApPortalSubsystem::AddToEndOfQueue(FInventoryItem item) {
	OutputQueue.AddHead(item);
}

void AApPortalSubsystem::AddToStartOfQueue(FInventoryItem item) {
	OutputQueue.AddTail(item);
}

bool AApPortalSubsystem::TryPopFromQueue(FInventoryItem& outItem) {
	if (!OutputQueue.IsEmpty()) {
		TDoubleLinkedList<FInventoryItem>::TDoubleLinkedListNode* head = OutputQueue.GetHead();
		outItem = head->GetValue();
		OutputQueue.RemoveNode(head);
		return true;
	}
	
	return false;
}

void AApPortalSubsystem::ClearQueue() {
	OutputQueue.Empty();
}