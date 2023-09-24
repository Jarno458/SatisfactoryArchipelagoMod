#include "ApGiftingSubsystem.h"
#include "Data/ApMappings.h"

DEFINE_LOG_CATEGORY(LogApGiftingSubsystem);

//TODO REMOVE
#pragma optimize("", off)

AApGiftingSubsystem* AApGiftingSubsystem::Get() {
	return Get(GEngine->GameViewport->GetWorld());
}

AApGiftingSubsystem* AApGiftingSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	check(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApGiftingSubsystem>();
}

AApGiftingSubsystem::AApGiftingSubsystem() : Super() {
	UE_LOG(LogApGiftingSubsystem, Display, TEXT("AApGiftingSubsystem::AApGiftingSubsystem()"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.1f;
}

void AApGiftingSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApGiftingSubsystem, Display, TEXT("AApGiftingSubsystem::BeginPlay()"));

	UWorld* world = GetWorld();
	ap = AApSubsystem::Get(world);
	portalSubSystem = AApPortalSubsystem::Get(world);
	resourceSinkSubsystem = AFGResourceSinkSubsystem::Get(world);
	mappingSubsystem = AApMappingsSubsystem::Get(world);
}

void AApGiftingSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!apInitialized) {
		if (ap->ConnectionState == EApConnectionState::Connected && mappingSubsystem->IsInitialized()) {
			LoadItemNameMapping();

			ap->SetGiftBoxState(true);

			apInitialized = true;
		}
	} else {
		FDateTime currentTime = FDateTime::Now();

		if ((currentTime - lastPoll).GetSeconds() >= pollInterfall)
		{
			lastPoll = currentTime;

			ProcessInputQueue();
			PullAllGiftsAsync();
		}
	}
}

void AApGiftingSubsystem::LoadItemNameMapping() {
	for (TPair<int64_t, FApItemInfo> itemInfoMapping : mappingSubsystem->ItemInfo) {
		ItemToItemId.Add(itemInfoMapping.Value.Class, itemInfoMapping.Key);
	}
}

void AApGiftingSubsystem::PullAllGiftsAsync() {
	TArray<FApReceiveGift> gifts = ap->GetGifts();

	UpdatedProcessedIds(gifts);

	for (FApReceiveGift gift : gifts) {
		if (ProcessedIds.Contains(gift.Id))
			continue;

		ProcessedIds.Add(gift.Id);

		//try match on name
		if (mappingSubsystem->NameToItemId.Contains(gift.ItemName)) {
			portalSubSystem->Enqueue(mappingSubsystem->ItemInfo[mappingSubsystem->NameToItemId[gift.ItemName]].Class, gift.Amount);
		}	else {
			//if name cant be matched, try match on traits
			TSubclassOf<UFGItemDescriptor> itemClass = TryGetItemClassByTraits(gift.Traits);

			if (itemClass != nullptr) {
				portalSubSystem->Enqueue(itemClass, gift.Amount);
			} else {
				ap->RejectGift(gift.Id);
			}
		}

		ap->AcceptGift(gift.Id);
	}
}

void AApGiftingSubsystem::UpdatedProcessedIds(TArray<FApReceiveGift> gifts) {
	TSet<FString> currentGiftIds;
	for (FApReceiveGift gift : gifts) {
		currentGiftIds.Add(gift.Id);
	}

	TArray<FString> giftIdsToForget;
	for (FString id : ProcessedIds) {
		if (!currentGiftIds.Contains(id))
			giftIdsToForget.Add(id);
	}

	for (FString id : giftIdsToForget) {
		ProcessedIds.Remove(id);
	}
}

void AApGiftingSubsystem::EnqueueForSending(FApPlayer targetPlayer, FInventoryStack itemStack) {
	if (!InputQueue.Contains(targetPlayer)) {
		TSharedPtr<TQueue<FInventoryStack, EQueueMode::Mpsc>> queue = MakeShareable(new TQueue<FInventoryStack, EQueueMode::Mpsc>());
		InputQueue.Add(targetPlayer, queue);
	}

	InputQueue[targetPlayer]->Enqueue(itemStack);
}

void AApGiftingSubsystem::ProcessInputQueue() {
	TMap<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend;

	for (TPair<FApPlayer, TSharedPtr<TQueue<FInventoryStack, EQueueMode::Mpsc>>> stacksPerPlayer : InputQueue) {
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
		Send(itemsToSend);
}

void AApGiftingSubsystem::Send(TMap<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend) {
	for (TPair<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSendPerPlayer : itemsToSend) {
		if (itemsToSendPerPlayer.Value.Num() <= 0)
			continue;

		for (TPair<TSubclassOf<UFGItemDescriptor>, int> stack : itemsToSendPerPlayer.Value) {
			int64_t itemId = ItemToItemId[stack.Key];
			FString itemName = mappingSubsystem->ItemInfo[itemId].Name;
			int itemValue = resourceSinkSubsystem->GetResourceSinkPointsForItem(stack.Key);

			FApSendGift gift;
			gift.ItemName = mappingSubsystem->ItemInfo[itemId].Name;
			gift.Amount = stack.Value;
			gift.ItemValue = resourceSinkSubsystem->GetResourceSinkPointsForItem(stack.Key);
			gift.Traits = GetTraitsForItem(itemId, itemValue);
			gift.Receiver = itemsToSendPerPlayer.Key;

			ap->SendGift(gift);
		}
	}
}

TSubclassOf<UFGItemDescriptor> AApGiftingSubsystem::TryGetItemClassByTraits(TArray<FApGiftTrait> traits) {
	//TODO process item traits and quality, like "Metal" with quality 2 might be Encased Steam Beam
	return nullptr;
}

TArray<FApGiftTrait> AApGiftingSubsystem::GetTraitsForItem(int64_t itemId, int itemValue) {
	//TODO map item to trails

	if (!TraitsPerItem.Contains(itemId))
		return TArray<FApGiftTrait>();

	TArray<FApGiftTrait> Traits;

	for (TPair<FString, float> trait : TraitsPerItem[itemId]) {
		float traitValue = (itemValue / TraitDefaults[trait.Key] / 10) * trait.Value;

		FApGiftTrait traitSpecification;
		traitSpecification.Trait = trait.Key;
		traitSpecification.Quality = traitValue;
		traitSpecification.Duration = 1.0f;

		Traits.Add(traitSpecification);
	}

	return Traits;
}

#pragma optimize("", on)