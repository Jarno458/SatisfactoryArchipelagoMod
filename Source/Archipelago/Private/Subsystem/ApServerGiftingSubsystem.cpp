#include "ApServerGiftingSubsystem.h"
#include "Data/ApGiftingMappings.h"

DEFINE_LOG_CATEGORY(LogApServerGiftingSubsystem);

//TODO REMOVE
#pragma optimize("", off)

AApServerGiftingSubsystem* AApServerGiftingSubsystem::Get(UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApServerGiftingSubsystem* AApServerGiftingSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApServerGiftingSubsystem>();
}

AApServerGiftingSubsystem::AApServerGiftingSubsystem() : Super() {
	UE_LOG(LogApServerGiftingSubsystem, Display, TEXT("AApServerGiftingSubsystem::AApServerGiftingSubsystem()"));

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.1f;
}

void AApServerGiftingSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApServerGiftingSubsystem, Display, TEXT("AApServerGiftingSubsystem::BeginPlay()"));

	if (HasAuthority()) {
		UWorld* world = GetWorld();

		ap = AApSubsystem::Get(world);
		connectionInfoSubsystem = AApConnectionInfoSubsystem::Get(world);
		mappingSubsystem = AApMappingsSubsystem::Get(world);
		portalSubSystem = AApPortalSubsystem::Get(world);
		replicatedGiftingSubsystem = AApReplicatedGiftingSubsystem::Get(world);
	}
}

void AApServerGiftingSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!apInitialized) {
		if (connectionInfoSubsystem->GetConnectionState() == EApConnectionState::Connected
			&& replicatedGiftingSubsystem->HasLoadedItemTraits()
			&& mappingSubsystem->HasLoadedItemNameMappings()
			&& portalSubSystem->IsInitialized())
		{
			ap->SetGiftBoxState(true);

			apInitialized = true;

			SetActorTickInterval(pollInterfall);

			FString giftboxKey = FString::Format(TEXT("GiftBox;{0};{1}"), { connectionInfoSubsystem->GetCurrentPlayerTeam(), connectionInfoSubsystem->GetCurrentPlayerSlot()});
			ap->MonitorDataStoreValue(giftboxKey, [this]() { PullAllGiftsAsync(); });
		} else {
			return;
		}
	}

	ProcessInputQueue();
}

void AApServerGiftingSubsystem::PullAllGiftsAsync() {
	TArray<FApReceiveGift> gifts = ap->GetGifts();

	TSet<FString> giftsToAccept;
	TSet<FString> giftsToReject;

	UpdatedProcessedIds(gifts);

	for (FApReceiveGift& gift : gifts) {
		UE_LOG(LogApServerGiftingSubsystem, Display, TEXT("AApServerGiftingSubsystem::PullAllGiftsAsync() Received(%s)"), *gift.Id);

		if (ProcessedIds.Contains(gift.Id)) {
			UE_LOG(LogApServerGiftingSubsystem, Display, TEXT("AApServerGiftingSubsystem::PullAllGiftsAsync() Skipping gift(%s)"), *gift.Id);
			continue;
		}

		ProcessedIds.Add(gift.Id);

		TSubclassOf<UFGItemDescriptor> itemClass = nullptr;

		//try match on name
		if (mappingSubsystem->NameToItemId.Contains(gift.ItemName)) {
			UE_LOG(LogApServerGiftingSubsystem, Display, TEXT("AApServerGiftingSubsystem::PullAllGiftsAsync() passing gift(%s) to portalSubSystem using itemName %s"), *gift.Id, *gift.ItemName);
			itemClass = StaticCastSharedRef<FApItem>(mappingSubsystem->ApItems[mappingSubsystem->NameToItemId[gift.ItemName]])->Class;
		} else if (UApGiftingMappings::HardcodedItemNameToIdMappings.Contains(gift.ItemName)) {
			UE_LOG(LogApServerGiftingSubsystem, Display, TEXT("AApServerGiftingSubsystem::PullAllGiftsAsync() passing gift(%s) to portalSubSystem using hardcoded mapping for %s"), *gift.Id, *gift.ItemName);
			int64 itemId = UApGiftingMappings::HardcodedItemNameToIdMappings[gift.ItemName];
			itemClass = StaticCastSharedRef<FApItem>(mappingSubsystem->ApItems[itemId])->Class;
		} else {
			UE_LOG(LogApServerGiftingSubsystem, Display, TEXT("AApServerGiftingSubsystem::PullAllGiftsAsync() passing gift(%s) to portalSubSystem by traits as item %s"), *gift.Id, *UFGItemDescriptor::GetItemName(itemClass).ToString());
			itemClass = TryGetItemClassByTraits(gift.Traits);
		}

		if (itemClass != nullptr) {
			portalSubSystem->Enqueue(itemClass, gift.Amount);
		} else {
			UE_LOG(LogApServerGiftingSubsystem, Display, TEXT("AApServerGiftingSubsystem::PullAllGiftsAsync() rejecting gift(%s)"), *gift.Id);
			giftsToReject.Add(gift.Id);
			continue;
		}

		UE_LOG(LogApServerGiftingSubsystem, Display, TEXT("AApServerGiftingSubsystem::PullAllGiftsAsync() accepting gift(%s)"), *gift.Id);
		giftsToAccept.Add(gift.Id);
	}

	ap->AcceptGift(giftsToAccept);
	ap->RejectGift(giftsToReject);
}

void AApServerGiftingSubsystem::UpdatedProcessedIds(TArray<FApReceiveGift>& gifts) {
	TSet<FString> currentGiftIds;
	for (FApReceiveGift& gift : gifts) {
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

void AApServerGiftingSubsystem::EnqueueForSending(FApPlayer targetPlayer, FInventoryStack itemStack) {
	if (!InputQueue.Contains(targetPlayer)) {
		TSharedPtr<TQueue<FInventoryStack, EQueueMode::Mpsc>> queue = MakeShareable(new TQueue<FInventoryStack, EQueueMode::Mpsc>());
		InputQueue.Add(targetPlayer, queue);
	}

	InputQueue[targetPlayer]->Enqueue(itemStack);
}

void AApServerGiftingSubsystem::ProcessInputQueue() {
	TMap<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend;

	for (TPair<FApPlayer, TSharedPtr<TQueue<FInventoryStack, EQueueMode::Mpsc>>>& stacksPerPlayer : InputQueue) {
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

void AApServerGiftingSubsystem::Send(TMap<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>>& itemsToSend) {
	for (TPair<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>>& itemsToSendPerPlayer : itemsToSend) {
		if (itemsToSendPerPlayer.Value.Num() <= 0)
			continue;

		for (TPair<TSubclassOf<UFGItemDescriptor>, int>& stack : itemsToSendPerPlayer.Value) {
			FApSendGift gift;

			FString itemName = mappingSubsystem->ItemClassToItemId.Contains(stack.Key)
				? mappingSubsystem->ItemIdToName[mappingSubsystem->ItemClassToItemId[stack.Key]]
				: UFGItemDescriptor::GetItemName(stack.Key).ToString();

			gift.ItemName = itemName;
			gift.Amount = stack.Value;
			gift.ItemValue = 0;
			gift.Traits = replicatedGiftingSubsystem->GetTraitsForItem(stack.Key);
			gift.Receiver = itemsToSendPerPlayer.Key;

			ap->SendGift(gift);
		}
	}
}

bool AApServerGiftingSubsystem::HasTraitKnownToSatisfactory(TArray<FApGiftTrait>& traits) {
	static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

	for (const FApGiftTrait& trait : traits) {
		if (giftTraitEnum->IsValidEnumValue((uint8)trait.Trait))
			return true;
	}

	return false;
}

TSubclassOf<UFGItemDescriptor> AApServerGiftingSubsystem::TryGetItemClassByTraits(TArray<FApGiftTrait>& traits) {
	uint32 hash = GetTraitsHash(traits);
	if (ItemPerTraitsHashCache.Contains(hash))
		return ItemPerTraitsHashCache[hash];

	TMap<TSubclassOf<UFGItemDescriptor>, TPair<int, float>> numberOfMatchesAndTotalDiviationPerItemClass;
	int mostMatches = 0;

	if (!HasTraitKnownToSatisfactory(traits))
		return nullptr;

	for (const TPair<TSubclassOf<UFGItemDescriptor>, FApTraitValues>& traitsPerItem : replicatedGiftingSubsystem->TraitsPerItem) {
		int matches = 0;
		float totalDifference = 0.0f;

		for (FApGiftTrait& trait : traits) {
			if (traitsPerItem.Value.AcceptsTrait(trait.Trait)){
				totalDifference += FGenericPlatformMath::Abs(traitsPerItem.Value.TraitsValues[trait.Trait] - trait.Quality);
				matches++;
			}
		}

		if (matches >= mostMatches) {
			mostMatches = matches;
			numberOfMatchesAndTotalDiviationPerItemClass.Add(traitsPerItem.Key, TPair<int, float>(matches, totalDifference));
		}
	}

	float lowestDifference = 0.0f;
	TMap<TSubclassOf<UFGItemDescriptor>, float> accurencyPerItem;
	TSubclassOf<UFGItemDescriptor> itemClassWithLowestDifference = nullptr;

	for (const TPair<TSubclassOf<UFGItemDescriptor>, TPair<int, float>>& numberOfMatchesAndTotalDiviation : numberOfMatchesAndTotalDiviationPerItemClass) {
		if (numberOfMatchesAndTotalDiviation.Value.Key != mostMatches)
			continue;

		if (itemClassWithLowestDifference == nullptr || numberOfMatchesAndTotalDiviation.Value.Value < lowestDifference) {
			lowestDifference = numberOfMatchesAndTotalDiviation.Value.Value;
			itemClassWithLowestDifference = numberOfMatchesAndTotalDiviation.Key;
		}
	}

	ItemPerTraitsHashCache.Add(hash, itemClassWithLowestDifference);

	return itemClassWithLowestDifference;
}

uint32 AApServerGiftingSubsystem::GetTraitsHash(TArray<FApGiftTrait>& traits) {
	TSortedMap<EGiftTrait, uint32> hashesPerTrait;

	for (FApGiftTrait& trait : traits)
		hashesPerTrait.Add(trait.Trait, HashCombine(GetTypeHash(trait.Trait), GetTypeHash(trait.Quality)));

	uint32 totalHash = 0;

	TArray<uint32> hashes;
	hashesPerTrait.GenerateValueArray(hashes);

	for (uint32 hash : hashes)
		totalHash = HashCombine(totalHash, hash);

	return totalHash;
}

#pragma optimize("", on)