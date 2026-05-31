#include "Subsystem/ApPortalSubsystem.h"

#include "ApVaultSubsystem.h"
#include "Containers/List.h"
#include "EngineUtils.h"
#include "Buildable/ApPortal.h"
#include "Subsystem/ApServerGiftingSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"

DEFINE_LOG_CATEGORY(LogApPortalSubsystem);

#pragma optimize("", off)

#define VAULT_SEND_INTERVAL 10.0f

AApPortalSubsystem* AApPortalSubsystem::Get(UWorld* world) {
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
	PrimaryActorTick.TickInterval = 0.048f; //TODO might want to change this is portals take care of thier own output

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

void AApPortalSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApPortalSubsystem, Display, TEXT("AApPortalSubsystem::BeginPlay()"));

	UWorld* world = GetWorld();
	giftingSubsystem = AApServerGiftingSubsystem::Get(world);
	mappings = AApMappingsSubsystem::Get(world);
	connectionInfoSubsystem = AApConnectionInfoSubsystem::Get(world);
	vaultSubsystem = AApVaultSubsystem::Get(world);

	lastAutoSendTime = FDateTime::Now();
}

void AApPortalSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!isInitialized) {
		RebuildQueueFromSave();

		isInitialized = true;
	}
	else {
		SendOutputQueueToPortals();
	}

	 if (lastAutoSendTime + FTimespan::FromSeconds(VAULT_SEND_INTERVAL) < FDateTime::Now()) {

		 ProcessPendingOutputQueue();

		 ProcessAutoVaultStoring();

		 lastAutoSendTime = FDateTime::Now();
	 }
}

void AApPortalSubsystem::ProcessPendingOutputQueue() {
	FInventoryItem pendingItem;

	while (PendingOutputQueue.Dequeue(pendingItem)) {
		AddToEndOfQueue(pendingItem);
	}

	//TODO only take what is requested by filters, likely the portals themzelf will take the items and handle filtering

	TArray<FItemAmount> personalVaultItems = vaultSubsystem->GetItems(true);
	if (!personalVaultItems.IsEmpty())
	{	 
		for (FItemAmount vaultItem : personalVaultItems)
		{
			int32 taken = vaultSubsystem->Take(vaultItem, true);

			for (int i = 0; i < taken; i++)
				AddToEndOfQueue(FInventoryItem(vaultItem.ItemClass));
		}
	}
	
	
	/*else { // from global should only be done if explicitly requested
		TArray<FItemAmount> globalVaultItems = vaultSubsystem->GetItems(false);

		for (FItemAmount vaultItem : globalVaultItems)
		{
			for (int i = 0; i < vaultItem.Amount; i++)
				AddToEndOfQueue(FInventoryItem(vaultItem.ItemClass));
		}
	}*/
}

void AApPortalSubsystem::SendOutputQueueToPortals() {
	for (TActorIterator<AApPortal> actorIterator(GetWorld()); actorIterator; ++actorIterator) {
		AApPortal* portal = *actorIterator;
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

void AApPortalSubsystem::SendBuffer(FApPlayer targetPlayer, TArray<FItemAmount> items)
{
	if (!targetPlayer.IsValid())
		return;

	if (vaultSubsystem->DoesPlayerAcceptVaultItems(targetPlayer)) {
		for (FItemAmount item : items) {
			vaultSubsystem->Store(item, targetPlayer);
		}
		return;
	}
		
	for (FItemAmount item : items) {
		static_cast<AApServerGiftingSubsystem*>(giftingSubsystem)->EnqueueForSending(targetPlayer, item);
	}
}

void AApPortalSubsystem::ReQueue(FInventoryItem nextItem) const
{
	if (nextItem.IsValid()) {
		vaultSubsystem->Store(FItemAmount(nextItem.GetItemClass(), 1), true);
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

void AApPortalSubsystem::ProcessAutoVaultStoring() const
{
	TMap<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, uint64>> itemsToStore;

	for (TActorIterator<AApPortal> actorIterator(GetWorld()); actorIterator; ++actorIterator) {
		AApPortal* portal = *actorIterator;
		if (!IsValid(portal) || !portal->targetPlayer.IsValid())
			continue;

		UFGInventoryComponent* inventory = portal->GetInventory();
		if (!IsValid(inventory))
			continue;

		TArray<FInventoryStack> contents;
		inventory->GetInventoryStacks(contents);
		inventory->Empty();

		TMap<TSubclassOf<UFGItemDescriptor>, uint64>& itemsForPlayer = itemsToStore.FindOrAdd(portal->targetPlayer);
		for (const FInventoryStack& stack : contents) {
			itemsForPlayer.FindOrAdd(stack.Item.GetItemClass()) += stack.NumItems;
		}
	}

	 for (const TPair<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, uint64>>& playerItems : itemsToStore) {
		 const FApPlayer& player = playerItems.Key;

		 for (const TPair<TSubclassOf<UFGItemDescriptor>, uint64>& item : playerItems.Value) {
			 const uint64& amount = item.Value;

		 	vaultSubsystem->Store(FItemAmount(item.Key, static_cast<int32>(FMath::Clamp<int64>(amount, 0, INT32_MAX))));
		 }
	 }
}	

#pragma optimize("", on)
