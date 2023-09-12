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
	PrimaryActorTick.TickInterval = 0;
}

void AApGiftingSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApGiftingSubsystem, Display, TEXT("AApGiftingSubsystem::BeginPlay()"));

	auto world = GetWorld();
	ap = AApSubsystem::Get(world);
	portalSubSystem = AApPortalSubsystem::Get(world);
}

void AApGiftingSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!apInitialized) {
		if (ap->ConnectionState == EApConnectionState::Connected) {
			LoadItemNameMapping();

			OpenGiftbox();

			FString giftboxKey = FString::Printf(TEXT("GiftBox;%i;%i"), ap->currentPlayerTeam, ap->currentPlayerSlot);
			ap->MonitorDataStoreValue(TCHAR_TO_UTF8(*giftboxKey), AP_DataType::Raw, "{}", [&](AP_SetReply setReply) {
				OnGiftsUpdated(setReply);
			});

			apInitialized = true;
		}
	}
}

void AApGiftingSubsystem::LoadItemNameMapping() {
	TMap<FName, FAssetData> itemDescriptorAssets = UApUtils::GetItemDescriptorAssets();
	for (TPair<int64_t, FString> itemMapping : UApMappings::ItemIdToGameItemDescriptor) {
		UFGItemDescriptor* itemDescriptor = UApUtils::GetItemDescriptorByName(itemDescriptorAssets, itemMapping.Value);
		TSubclassOf<UFGItemDescriptor> itemClass = itemDescriptor->GetClass();
		FString itemName = ap->GetItemName(itemMapping.Key);

		NameToItemMapping.Add(itemName, itemClass);
		ItemToNameMapping.Add(itemClass, itemName);
	}
}

void AApGiftingSubsystem::OpenGiftbox() {
	FString motherBoxKey = FString::Printf(TEXT("GiftBoxes;%i"), ap->currentPlayerTeam);
			
	AP_SetServerDataRequest motherBoxOpenGiftBox;
	motherBoxOpenGiftBox.key = TCHAR_TO_UTF8(*motherBoxKey);

	FApGiftMotherBoxJson motherBox;
	motherBox.IsOpen = true;
	motherBox.AcceptsAnyGift = true;
	motherBox.DesiredTraits = TArray<FString>();
	motherBox.GiftDataVersion = 2;

	FString motherBoxJson;
	FJsonObjectConverter::UStructToJsonObjectString<FApGiftMotherBoxJson>(motherBox, motherBoxJson);

	FString json = FString::Printf(TEXT(R"({ "%i": %s })"), ap->currentPlayerSlot, *motherBoxJson);
	std::string stdJson = TCHAR_TO_UTF8(*json);

	AP_DataStorageOperation update;
	update.operation = "update";
	update.value = &stdJson;

	std::vector<AP_DataStorageOperation> operations;
	operations.push_back(update);

	std::string defaultGiftboxValue = "{}";

	motherBoxOpenGiftBox.operations = operations;
	motherBoxOpenGiftBox.default_value = &defaultGiftboxValue;
	motherBoxOpenGiftBox.type = AP_DataType::Raw;
	motherBoxOpenGiftBox.want_reply = false;

	ap->SetServerData(&motherBoxOpenGiftBox);
}

void AApGiftingSubsystem::OnGiftsUpdated(AP_SetReply setReply) {
	FString jsonString(((std::string*)setReply.value)->c_str());

	const TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*jsonString);

	FJsonSerializer serializer;
	TSharedPtr<FJsonObject> parsedJson;

	serializer.Deserialize(reader, parsedJson);
	if (!parsedJson.IsValid())
		return;
	
	TArray<FString> procesedGifts;
	TArray<FApGiftJson> giftsToReject;

	for (TPair<FString, TSharedPtr<FJsonValue>> pair : parsedJson->Values) {
		const TSharedPtr<FJsonObject>* giftJsonObject;
		FApGiftJson gift;

		if (!pair.Value->TryGetObject(giftJsonObject)
			|| !FJsonObjectConverter::JsonObjectToUStruct<FApGiftJson>((*giftJsonObject).ToSharedRef(), &gift)) {
			procesedGifts.Add(pair.Key); 
			continue;
		}

		//try match on name
		if (NameToItemMapping.Contains(gift.ItemName)) {
			portalSubSystem->Enqueue(NameToItemMapping[gift.ItemName], gift.Amount);
		} else {
			//if name cant be matched, try match on traits
			TSubclassOf<UFGItemDescriptor> itemClass = TryGetItemClassByTraits(gift.Traits);

			if (itemClass != nullptr) {
				portalSubSystem->Enqueue(itemClass, gift.Amount);
			} else {
				//if gift cannot be matched to an item, reject it
				giftsToReject.Add(gift);
			}
		}

		procesedGifts.Add(pair.Key);
	}

	HandleProcessedGifts(procesedGifts, giftsToReject);
}

void AApGiftingSubsystem::HandleProcessedGifts(TArray<FString> procesedGifts, TArray<FApGiftJson> giftsToReject) {
	for (FApGiftJson rejectedGift : giftsToReject) {
		//TODO swap sender/receiver & send to other giftbox
	};

	FString giftboxKey = FString::Printf(TEXT("GiftBox;%i;%i"), ap->currentPlayerTeam, ap->currentPlayerSlot);

	std::vector<AP_DataStorageOperation> operations;

	for (FString id : procesedGifts) {
		std::string guid = TCHAR_TO_UTF8(*id);

		AP_DataStorageOperation update;
		update.operation = "pop";
		update.value = &guid;

		operations.push_back(update);
	}

	std::string defaultGiftboxValue = "{}";

	AP_SetServerDataRequest removeProcesedGifts;
	removeProcesedGifts.key = TCHAR_TO_UTF8(*giftboxKey);
	removeProcesedGifts.operations = operations;
	removeProcesedGifts.default_value = &defaultGiftboxValue;
	removeProcesedGifts.type = AP_DataType::Raw;
	removeProcesedGifts.want_reply = false;

	ap->SetServerData(&removeProcesedGifts);
}



TSubclassOf<UFGItemDescriptor> AApGiftingSubsystem::TryGetItemClassByTraits(TArray<FApGiftTraitJson> traits) {
	//TODO process item traits and quality, like "Metal" with quality 2 might be Encased Steam Beam
	 





	return nullptr;
}

void AApGiftingSubsystem::Send(TMap<int, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend) {
	for (TPair<int, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSendPerPlayer : itemsToSend) {
		TArray<FString> jsons;
		for (TPair<TSubclassOf<UFGItemDescriptor>, int> stack : itemsToSendPerPlayer.Value) {
			FString guid = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);

			FApGiftJson gift;
			gift.ID = guid;
			gift.ItemName = ItemToNameMapping[stack.Key];
			gift.Amount = stack.Value;
			gift.ItemValue = 0;
			gift.Traits = GetTraitsForItem(0);
			gift.SenderSlot = ap->currentPlayerSlot;
			gift.SenderTeam = ap->currentPlayerTeam;
			gift.ReceiverSlot = itemsToSendPerPlayer.Key;
			gift.ReceiverTeam = ap->currentPlayerTeam;
			gift.IsRefund = false;

			FString giftJson;
			if (FJsonObjectConverter::UStructToJsonObjectString<FApGiftJson>(gift, giftJson)) {
				FString json = FString::Printf(TEXT(R"({ "%s": %s })"), *guid, *giftJson);
				jsons.Add(json);
			}
		}

		FString json = FString::Join(jsons, TEXT(", "));
		std::string stdJson = TCHAR_TO_UTF8(*json);

		AP_DataStorageOperation update;
		update.operation = "update";
		update.value = &stdJson;

		std::vector<AP_DataStorageOperation> operations;
		operations.push_back(update);

		FString giftboxKey = FString::Printf(TEXT("GiftBox;%i;%i"), ap->currentPlayerTeam, itemsToSendPerPlayer.Key);
		std::string defaultGiftboxValue = "{}";

		AP_SetServerDataRequest addToGiftBox;
		addToGiftBox.key = TCHAR_TO_UTF8(*giftboxKey);
		addToGiftBox.operations = operations;
		addToGiftBox.default_value = &defaultGiftboxValue;
		addToGiftBox.type = AP_DataType::Raw;
		addToGiftBox.want_reply = false;

		ap->SetServerData(&addToGiftBox);
	}
}

TArray<FApGiftTraitJson> AApGiftingSubsystem::GetTraitsForItem(int64_t itemId) {
	return TArray<FApGiftTraitJson>();
}

#pragma optimize("", on)