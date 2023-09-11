#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApGiftingSubsystem.h"

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

	lastInventoryDump = FDateTime::Now();
}

void AApPortalSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApPortalSubsystem, Display, TEXT("AApPortalSubsystem::BeginPlay()"));

	giftingSubsystem = AApGiftingSubsystem::Get(GetWorld());
}

void AApPortalSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!HasAuthority()) {
		return;
	}

	ProcessInputQueue();
	ProcessOutputQueue();
}

void AApPortalSubsystem::ProcessInputQueue() {
	FDateTime currentTime = FDateTime::Now();

	if ((currentTime - lastInventoryDump).GetSeconds() < 30)
		return;

	lastInventoryDump = currentTime;

	TMap<int, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend;

	for (TPair<int, TSharedPtr<TQueue<FInventoryStack, EQueueMode::Mpsc>>> stacksPerPlayer : InputQueue) {
		if (!itemsToSend.Contains(stacksPerPlayer.Key))
			itemsToSend.Add(stacksPerPlayer.Key, TMap<TSubclassOf<UFGItemDescriptor>, int>());

		FInventoryStack stack;
		while (stacksPerPlayer.Value->Dequeue(stack))
		{
			TSubclassOf<UFGItemDescriptor> cls = stack.Item.GetItemClass();

			if (!itemsToSend[stacksPerPlayer.Key].Contains(cls))
				itemsToSend[stacksPerPlayer.Key].Add(cls, stack.NumItems);
			else
				itemsToSend[stacksPerPlayer.Key][cls] += stack.NumItems;
		}
	}

	if (!itemsToSend.IsEmpty())
		((AApGiftingSubsystem*)giftingSubsystem)->Send(itemsToSend);
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

void AApPortalSubsystem::Send(int targetSlot, FInventoryStack itemStack) {
	if (!InputQueue.Contains(targetSlot)) {
		TSharedPtr<TQueue<FInventoryStack, EQueueMode::Mpsc>> queue = MakeShareable(new TQueue<FInventoryStack, EQueueMode::Mpsc>());
		InputQueue.Add(targetSlot, queue);
	}

	InputQueue[targetSlot]->Enqueue(itemStack);
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
