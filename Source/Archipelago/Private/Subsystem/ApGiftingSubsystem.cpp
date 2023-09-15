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

			OpenGiftbox();

			FString giftboxKey = FString::Printf(TEXT("GiftBox;%i;%i"), ap->currentPlayerTeam, ap->currentPlayerSlot);
			ap->MonitorDataStoreValue(TCHAR_TO_UTF8(*giftboxKey), AP_DataType::Raw, "{}", [&](AP_SetReply setReply) {
				OnGiftsUpdated(setReply);
			});

			apInitialized = true;
		}
	} else {
		FDateTime currentTime = FDateTime::Now();

		if ((currentTime - lastPoll).GetSeconds() >= pollInterfall)
		{
			lastPoll = currentTime;

			PullAllGiftsAsync();
			ProcessInputQueue();
			HandleGiftsToReject();
		}
	}
}

void AApGiftingSubsystem::LoadItemNameMapping() {
	for (TPair<int64_t, FApItemInfo> itemInfoMapping : mappingSubsystem->ItemInfo) {
		ItemToItemId.Add(itemInfoMapping.Value.Class, itemInfoMapping.Key);
	}
}

void AApGiftingSubsystem::OpenGiftbox() {

	FString json = FString::Printf(TEXT(R"({
		"%i": {
			"IsOpen": true,
			"AcceptsAnyGift": true,
			"DesiredTraits": [],
			"MinimumGiftDataVersion": 2,
			"MaximumGiftDataVersion": 2
		}
	})"), ap->currentPlayerSlot);

	std::string stdJson = TCHAR_TO_UTF8(*json);

	AP_DataStorageOperation update;
	update.operation = "update";
	update.value = &stdJson;

	std::vector<AP_DataStorageOperation> operations;
	operations.push_back(update);

	FString motherBoxKey = FString::Printf(TEXT("GiftBoxes;%i"), ap->currentPlayerTeam);

	AP_SetServerDataRequest motherBoxOpenGiftBox;
	motherBoxOpenGiftBox.key = TCHAR_TO_UTF8(*motherBoxKey);
	motherBoxOpenGiftBox.operations = operations;
	motherBoxOpenGiftBox.default_value = &defaultGiftboxValue;
	motherBoxOpenGiftBox.type = AP_DataType::Raw;
	motherBoxOpenGiftBox.want_reply = false;

	ap->SetServerData(&motherBoxOpenGiftBox);
}

void AApGiftingSubsystem::OnGiftsUpdated(AP_SetReply setReply) {
	FString originalValue(((std::string*)setReply.original_value)->c_str());
	FString value(((std::string*)setReply.value)->c_str());

	if (!value.StartsWith(TEXT("{}")) || originalValue.StartsWith(TEXT("{}")))
		return;

	const TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*originalValue);

	FJsonSerializer serializer;
	TSharedPtr<FJsonObject> parsedJson;

	serializer.Deserialize(reader, parsedJson);
	if (!parsedJson.IsValid())
		return;
	
	TArray<FApGiftJson> giftsToReject;

	for (TPair<FString, TSharedPtr<FJsonValue>> pair : parsedJson->Values) {
		const TSharedPtr<FJsonObject>* giftJsonObject;
		FApGiftJson gift;

		if (!pair.Value->TryGetObject(giftJsonObject)
			|| !FJsonObjectConverter::JsonObjectToUStruct<FApGiftJson>((*giftJsonObject).ToSharedRef(), &gift)) {
			continue;
		}

		//try match on name
		if (mappingSubsystem->NameToItemId.Contains(gift.ItemName)) {
			portalSubSystem->Enqueue(mappingSubsystem->ItemInfo[mappingSubsystem->NameToItemId[gift.ItemName]].Class, gift.Amount);
		} else {
			//if name cant be matched, try match on traits
			TSubclassOf<UFGItemDescriptor> itemClass = TryGetItemClassByTraits(gift.Traits);

			if (itemClass != nullptr) {
				portalSubSystem->Enqueue(itemClass, gift.Amount);
			} else {
				GiftsToRefund.Enqueue(gift);
			}
		}
	}
}

void AApGiftingSubsystem::PullAllGiftsAsync() {
	FString json = TEXT("{}");
	std::string stdJson = TCHAR_TO_UTF8(*json);
	AP_DataStorageOperation update;
	update.operation = "replace";
	update.value = &stdJson;

	std::vector<AP_DataStorageOperation> operations;
	operations.push_back(update);

	FString giftboxKey = FString::Printf(TEXT("GiftBox;%i;%i"), ap->currentPlayerTeam, ap->currentPlayerSlot);

	std::string defaultValue = "{}";

	AP_SetServerDataRequest nukeGiftBox;
	nukeGiftBox.key = TCHAR_TO_UTF8(*giftboxKey);
	nukeGiftBox.operations = operations;
	nukeGiftBox.default_value = &defaultValue;
	nukeGiftBox.type = AP_DataType::Raw;
	nukeGiftBox.want_reply = false;

	ap->SetServerData(&nukeGiftBox);
}

void AApGiftingSubsystem::HandleGiftsToReject() {
	FApGiftJson giftToReject;
	if (!GiftsToRefund.Dequeue(giftToReject))
		return;

	FString json = FString::Printf(TEXT(R"({
		"%s": {
			"ID": "%s",
			"ItemName": "%s",
			"Amount": %i,
			"ItemValue": %i,
			"Traits": [ %s ],
			"Sender": %i,
			"Receiver": %i,
			"SenderTeam": %i,
			"ReceiverTeam": %i,
			"IsRefund": true
		}
	})"), *giftToReject.ID, *giftToReject.ID, *giftToReject.ItemName, giftToReject.Amount, giftToReject.ItemValue, 
	*BuildTraitsJson(giftToReject.Traits), giftToReject.SenderSlot, giftToReject.ReceiverSlot, giftToReject.SenderTeam, giftToReject.ReceiverTeam);

	std::string stdJson = TCHAR_TO_UTF8(*json);
	AP_DataStorageOperation update;
	update.operation = "update";
	update.value = &stdJson;

	std::vector<AP_DataStorageOperation> operations;
	operations.push_back(update);

	FString giftboxKey = FString::Printf(TEXT("GiftBox;%i;%i"), giftToReject.SenderTeam, giftToReject.SenderSlot);

	AP_SetServerDataRequest removeProcesedGifts;
	removeProcesedGifts.key = TCHAR_TO_UTF8(*giftboxKey);
	removeProcesedGifts.operations = operations;
	removeProcesedGifts.default_value = &defaultGiftboxValue;
	removeProcesedGifts.type = AP_DataType::Raw;
	removeProcesedGifts.want_reply = false;

	ap->SetServerData(&removeProcesedGifts);
}

FString AApGiftingSubsystem::BuildTraitsJson(TArray<FApGiftTraitJson> Traits) {
	FString json = TEXT("");

	for (FApGiftTraitJson trait : Traits) {
		json += FString::Printf(TEXT(R"({
			"Trait": "%s",
			"Quality": %f,
			"Duration": %f
		},)"), *trait.Trait, trait.Quality, trait.Duration);
	}

	json.RemoveFromEnd(TEXT(","));

	return json;
}

void AApGiftingSubsystem::EnqueueForSending(int targetSlot, FInventoryStack itemStack) {
	if (!InputQueue.Contains(targetSlot)) {
		TSharedPtr<TQueue<FInventoryStack, EQueueMode::Mpsc>> queue = MakeShareable(new TQueue<FInventoryStack, EQueueMode::Mpsc>());
		InputQueue.Add(targetSlot, queue);
	}

	InputQueue[targetSlot]->Enqueue(itemStack);
}

void AApGiftingSubsystem::ProcessInputQueue() {
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
		Send(itemsToSend);
}

void AApGiftingSubsystem::Send(TMap<int, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend) {
	for (TPair<int, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSendPerPlayer : itemsToSend) {
		if (itemsToSendPerPlayer.Value.Num() <= 0)
			continue;

		FString json = TEXT("");

		for (TPair<TSubclassOf<UFGItemDescriptor>, int> stack : itemsToSendPerPlayer.Value) {
			int64_t itemId = ItemToItemId[stack.Key];
			FString guid = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
			FString itemName = mappingSubsystem->ItemInfo[itemId].Name;
			int itemValue = resourceSinkSubsystem->GetResourceSinkPointsForItem(stack.Key);
			FString traits = GetTraitsJsonForItem(itemId, itemValue);
			json += FString::Printf(TEXT(R"("%s": {
				"ID": "%s",
				"ItemName": "%s",
				"Amount": %i,
				"ItemValue": %i,
				"Traits": [ %s ],
				"Sender": %i,
				"Receiver": %i,
				"SenderTeam": %i,
				"ReceiverTeam": %i,
				"IsRefund": false
			},)"), *guid, *guid, *itemName, stack.Value, itemValue, *traits, ap->currentPlayerSlot, itemsToSendPerPlayer.Key, ap->currentPlayerTeam, ap->currentPlayerTeam);
		}

		json.RemoveFromEnd(TEXT(","));
		json = TEXT("{") + json + TEXT("}");
		std::string stdJson = TCHAR_TO_UTF8(*json);

		AP_DataStorageOperation update;
		update.operation = "update";
		update.value = &stdJson;

		std::vector<AP_DataStorageOperation> operations;
		operations.push_back(update);

		FString giftboxKey = FString::Printf(TEXT("GiftBox;%i;%i"), ap->currentPlayerTeam, itemsToSendPerPlayer.Key);
		AP_SetServerDataRequest addToGiftBox;

		addToGiftBox.key = TCHAR_TO_UTF8(*giftboxKey);
		addToGiftBox.operations = operations;
		addToGiftBox.default_value = &defaultGiftboxValue;
		addToGiftBox.type = AP_DataType::Raw;
		addToGiftBox.want_reply = false;

		ap->SetServerData(&addToGiftBox);
	}
}

TSubclassOf<UFGItemDescriptor> AApGiftingSubsystem::TryGetItemClassByTraits(TArray<FApGiftTraitJson> traits) {
	//TODO process item traits and quality, like "Metal" with quality 2 might be Encased Steam Beam
	return nullptr;
}

FString AApGiftingSubsystem::GetTraitsJsonForItem(int64_t itemId, int itemValue) {
	//TODO map item to trails

	if (!TraitsPerItem.Contains(itemId))
		return TEXT("");

	TArray<FApGiftTraitJson> Traits;

	for (TPair<FString, float> trait : TraitsPerItem[itemId]) {
		float traitValue = (itemValue / TraitDefaults[trait.Key] / 10) * trait.Value;

		FApGiftTraitJson traitSpecification;
		traitSpecification.Trait = trait.Key;
		traitSpecification.Quality = traitValue;
		traitSpecification.Duration = 1.0f;

		Traits.Add(traitSpecification);
	}

	return BuildTraitsJson(Traits);
}

#pragma optimize("", on)