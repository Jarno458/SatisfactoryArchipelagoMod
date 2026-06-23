#include "ApServerGiftingSubsystem.h"

#include "Logging/StructuredLog.h"
#include "Data/ApGiftingMappings.h"
#include "Subsystem/ApVaultSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"

DEFINE_LOG_CATEGORY(LogApServerGiftingSubsystem);

AApServerGiftingSubsystem* AApServerGiftingSubsystem::Get(UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApServerGiftingSubsystem* AApServerGiftingSubsystem::Get(UWorld* world) {
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
		giftTraitsSubsystem = AApGiftTraitsSubsystem::Get(world);
		vaultSubsystem = AApVaultSubsystem::Get(world);
		playerInfoSubsystem = AApPlayerInfoSubsystem::Get(world);
	}
}

void AApServerGiftingSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!apInitialized) {
		if (connectionInfoSubsystem->GetConnectionState() == EApConnectionState::Connected
			&& giftTraitsSubsystem->HasLoadedItemTraits()
			&& mappingSubsystem->HasLoadedItemNameMappings()
			&& playerInfoSubsystem->IsInitialized())
		{
			static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

			TSet<FString> allTraits;
			for (int32 i = 0; i < (giftTraitEnum->NumEnums() - 1); i++)
				allTraits.Add(giftTraitEnum->GetNameStringByIndex(i));

			ap->SetGiftBoxState(true, allTraits);

			apInitialized = true;

			SetActorTickInterval(pollInterfall);

			FString giftboxKey = FString::Format(TEXT("GiftBox;{0};{1}"), { connectionInfoSubsystem->GetCurrentPlayerTeam(), connectionInfoSubsystem->GetCurrentPlayerSlot() });
			ap->MonitorDataStoreJsonObjectValue(giftboxKey, [this](const FString& key, const TSharedRef<FJsonValue>& oldValueJson, const TSharedRef<FJsonValue>& newValueJson, int slot) {
				PullAllGiftsAsync(key, oldValueJson, newValueJson, slot);
			});
		}
		else {
			return;
		}
	}

	ProcessInputQueue();

	if (pullMore)
	{
		FString giftboxKey = FString::Format(TEXT("GiftBox;{0};{1}"), { connectionInfoSubsystem->GetCurrentPlayerTeam(), connectionInfoSubsystem->GetCurrentPlayerSlot() });
		ap->TickleDataStorageKey(giftboxKey);
		pullMore = false;
	}
}

void AApServerGiftingSubsystem::PullAllGiftsAsync(const FString& key, const TSharedRef<FJsonValue>& oldValueJson, const TSharedRef<FJsonValue>& newValueJson, int slot) {
	static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

	TSharedPtr<FJsonObject>* newValueObject;
	if (!newValueJson->TryGetObject(newValueObject))
		return;

	int count = 0;
	TArray<FApGift> gifts;
	for (const TPair<FString, TSharedPtr<FJsonValue>>& giftJson : (*newValueObject)->Values) {
		UE_LOGFMT(LogApServerGiftingSubsystem, Display, "AApServerGiftingSubsystem::PullAllGiftsAsync() received gift {0}", *giftJson.Key);

		if (++count > 50) {
			UE_LOG(LogApServerGiftingSubsystem, Warning, TEXT("AApServerGiftingSubsystem::PullAllGiftsAsync() More than 50 gifts in the gift box, skipping the rest"));
			break;
		}

		TSharedPtr<FJsonObject>* giftObjectPtr;
		if (!giftJson.Value->TryGetObject(giftObjectPtr))
		{
			UE_LOGFMT(LogApServerGiftingSubsystem, Error, "AApServerGiftingSubsystem::PullAllGiftsAsync() Failed to parse gift object for {0}", *giftJson.Key);
			continue;
		}

		FApGift gift;
		gift.Id = *giftJson.Key;

		if (!(*giftObjectPtr)->TryGetStringField(TEXT("item_name"), gift.ItemName))
		{
			UE_LOGFMT(LogApServerGiftingSubsystem, Error, "AApServerGiftingSubsystem::PullAllGiftsAsync() Failed to parse item_name for gift {0}", *giftJson.Key);
			continue;
		}
		if (!(*giftObjectPtr)->TryGetNumberField(TEXT("amount"), gift.Amount))
		{
			UE_LOGFMT(LogApServerGiftingSubsystem, Error, "AApServerGiftingSubsystem::PullAllGiftsAsync() Failed to parse amount for gift {0}", *giftJson.Key);
			continue;
		}
		if (!(*giftObjectPtr)->TryGetNumberField(TEXT("item_value"), gift.ItemValue))
		{
			gift.ItemValue = 0;
		}

		FApPlayer sender;
		if (!(*giftObjectPtr)->TryGetNumberField(TEXT("sender_team"), sender.Team))
		{
			UE_LOGFMT(LogApServerGiftingSubsystem, Error, "AApServerGiftingSubsystem::PullAllGiftsAsync() Failed to parse sender_team for gift {0}", *giftJson.Key);
			continue;
		}
		if (!(*giftObjectPtr)->TryGetNumberField(TEXT("sender_slot"), sender.Slot))
		{
			UE_LOGFMT(LogApServerGiftingSubsystem, Error, "AApServerGiftingSubsystem::PullAllGiftsAsync() Failed to parse sender_slot for gift {0}", *giftJson.Key);
			continue;
		}

		FApPlayer receiver;
		if (!(*giftObjectPtr)->TryGetNumberField(TEXT("receiver_team"), receiver.Team))
		{
			UE_LOGFMT(LogApServerGiftingSubsystem, Error, "AApServerGiftingSubsystem::PullAllGiftsAsync() Failed to parse receiver_team for gift {0}", *giftJson.Key);
			continue;
		}
		if (!(*giftObjectPtr)->TryGetNumberField(TEXT("receiver_slot"), receiver.Slot))
		{
			UE_LOGFMT(LogApServerGiftingSubsystem, Error, "AApServerGiftingSubsystem::PullAllGiftsAsync() Failed to parse receiver_slot for gift {0}", *giftJson.Key);
			continue;
		}

		gift.Sender = sender;
		gift.Receiver = receiver;
		gift.Traits.Empty();

		const TArray<TSharedPtr<FJsonValue>>* traitsJson;
		if ((*giftObjectPtr)->TryGetArrayField(TEXT("traits"), traitsJson)) //optional value
		{
			for (const TSharedPtr<FJsonValue>& traitJson : *traitsJson) {
				TSharedPtr<FJsonObject>* traitJsonObjectPtr;
				if (!traitJson->TryGetObject(traitJsonObjectPtr))
				{
					UE_LOGFMT(LogApServerGiftingSubsystem, Error, "AApServerGiftingSubsystem::PullAllGiftsAsync() Failed to parse trait for gift {0}", *giftJson.Key);
					continue;
				}

				FString traitName;
				if (!(*traitJsonObjectPtr)->TryGetStringField(TEXT("trait"), traitName))
				{
					UE_LOGFMT(LogApServerGiftingSubsystem, Error, "AApServerGiftingSubsystem::PullAllGiftsAsync() Failed to parse trait name for gift {0}", *giftJson.Key);
					continue;
				}

				const int64 enumValue = giftTraitEnum->GetValueByNameString(traitName);
				if (enumValue == INDEX_NONE)
					continue;

				FApGiftTrait trait;
				trait.Trait = static_cast<EGiftTrait>(enumValue);

				if (!(*giftObjectPtr)->TryGetNumberField(TEXT("duration"), trait.Duration))
				{
					trait.Duration = 1;
				}

				if (!(*giftObjectPtr)->TryGetNumberField(TEXT("quality"), trait.Quality))
				{
					trait.Quality = 1;
				}

				gift.Traits.Add(trait);
			}
		}

		gifts.Add(gift);
	}

	TSet<FString> giftsToAccept;
	TArray<FApGift> giftsToReject;

	TMap<TSubclassOf<UFGItemDescriptor>, int> itemsToAccepted;

	UpdatedProcessedIds(gifts);

	for (FApGift& gift : gifts) {
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
		}
		else if (UApGiftingMappings::HardcodedItemNameToIdMappings.Contains(gift.ItemName)) {
			UE_LOG(LogApServerGiftingSubsystem, Display, TEXT("AApServerGiftingSubsystem::PullAllGiftsAsync() passing gift(%s) to portalSubSystem using hardcoded mapping for %s"), *gift.Id, *gift.ItemName);
			int64 itemId = UApGiftingMappings::HardcodedItemNameToIdMappings[gift.ItemName];
			itemClass = StaticCastSharedRef<FApItem>(mappingSubsystem->ApItems[itemId])->Class;
		}
		else {
			itemClass = giftTraitsSubsystem->TryGetItemClassByTraits(gift.Traits);
			UE_LOG(LogApServerGiftingSubsystem, Display, TEXT("AApServerGiftingSubsystem::PullAllGiftsAsync() passing gift(%s) to portalSubSystem by traits as item %s"), *gift.Id, *UFGItemDescriptor::GetItemName(itemClass).ToString());
		}

		if (itemClass != nullptr) {
			int& amount = itemsToAccepted.FindOrAdd(itemClass);
			amount += gift.Amount;

			constexpr FLinearColor giftMessageColor(0.82f, 0.71f, 0.041f);
			FString sender = playerInfoSubsystem->GetPlayerName(gift.Sender);
			FString itemName = mappingSubsystem->ItemClassToItemId.Contains(itemClass)
				? mappingSubsystem->ItemIdToName[mappingSubsystem->ItemClassToItemId[itemClass]]
				: UFGItemDescriptor::GetItemName(itemClass).ToString();
			FString message = FString::Printf(TEXT("Accepted gift from %s of %i %s that turned into %i %s"), *sender, gift.Amount, *gift.ItemName, amount, *itemName);
			ap->SendChatMessage(EApMessageType::GiftsReceived, message, giftMessageColor);
		}
		else {
			UE_LOG(LogApServerGiftingSubsystem, Display, TEXT("AApServerGiftingSubsystem::PullAllGiftsAsync() rejecting gift(%s)"), *gift.Id);
			giftsToReject.Add(gift);
			continue;
		}

		UE_LOG(LogApServerGiftingSubsystem, Display, TEXT("AApServerGiftingSubsystem::PullAllGiftsAsync() accepting gift(%s)"), *gift.Id);
		giftsToAccept.Add(gift.Id);
	}

	if (!itemsToAccepted.IsEmpty()) {
		for (const TPair<TSubclassOf<UFGItemDescriptor>, int>& ItemsToAccepted : itemsToAccepted)
		{
			FItemAmount ItemAmount(ItemsToAccepted.Key, ItemsToAccepted.Value);

			vaultSubsystem->Store(ItemAmount, true);
		}
	}

	ap->ProcessGifts(giftsToAccept, giftsToReject);

	if (count >= 50)
		pullMore = true;
}

void AApServerGiftingSubsystem::UpdatedProcessedIds(TArray<FApGift>& gifts) {
	TSet<FString> currentGiftIds;
	for (FApGift& gift : gifts) {
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

void AApServerGiftingSubsystem::EnqueueForSending(FApPlayer targetPlayer, FItemAmount itemStack) {
	if (!InputQueue.Contains(targetPlayer)) {
		TSharedPtr<TQueue<FItemAmount, EQueueMode::Mpsc>> queue = MakeShareable(new TQueue<FItemAmount, EQueueMode::Mpsc>());
		InputQueue.Add(targetPlayer, queue);
	}

	InputQueue[targetPlayer]->Enqueue(itemStack);
}

void AApServerGiftingSubsystem::ProcessInputQueue() {
	TMap<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend;

	for (TPair<FApPlayer, TSharedPtr<TQueue<FItemAmount, EQueueMode::Mpsc>>>& stacksPerPlayer : InputQueue) {
		if (!itemsToSend.Contains(stacksPerPlayer.Key))
			itemsToSend.Add(stacksPerPlayer.Key, TMap<TSubclassOf<UFGItemDescriptor>, int>());

		FItemAmount stack;
		while (stacksPerPlayer.Value->Dequeue(stack))
		{
			TSubclassOf<UFGItemDescriptor> cls = stack.ItemClass;

			if (!itemsToSend[stacksPerPlayer.Key].Contains(cls))
				itemsToSend[stacksPerPlayer.Key].Add(cls, stack.Amount);
			else
				itemsToSend[stacksPerPlayer.Key][cls] += stack.Amount;
		}
	}

	if (!itemsToSend.IsEmpty())
		Send(itemsToSend);
}

void AApServerGiftingSubsystem::Send(TMap<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>>& itemsToSend) {
	for (TPair<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>>& itemsToSendPerPlayer : itemsToSend) {
		for (TPair<TSubclassOf<UFGItemDescriptor>, int>& stack : itemsToSendPerPlayer.Value) {
			FApGift gift;

			FString itemName = mappingSubsystem->ItemClassToItemId.Contains(stack.Key)
				? mappingSubsystem->ItemIdToName[mappingSubsystem->ItemClassToItemId[stack.Key]]
				: UFGItemDescriptor::GetItemName(stack.Key).ToString();

			gift.Id = FGuid::NewGuid().ToString();
			gift.ItemName = itemName;
			gift.Amount = stack.Value;
			gift.ItemValue = 0;
			gift.Traits = giftTraitsSubsystem->GetFullTraitsForItem(stack.Key);
			gift.Sender = connectionInfoSubsystem->GetCurrentPlayer();
			gift.Receiver = itemsToSendPerPlayer.Key;

			//TODO group all items for the same player together
			ap->SendGift(gift);
		}
	}
}
