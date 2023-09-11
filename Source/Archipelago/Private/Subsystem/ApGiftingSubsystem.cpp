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

			slotData = ap->GetSlotData();

			OpenGiftbox();

			FString giftboxKey = FString::Printf(TEXT("GiftBox;%i;%i"), slotData.currentPlayerTeam, slotData.currentPlayerSlot);
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
	FString motherBoxKey = FString::Printf(TEXT("GiftBoxes;%i"), slotData.currentPlayerTeam);
			
	AP_SetServerDataRequest motherBoxOpenGiftBox;
	motherBoxOpenGiftBox.key = TCHAR_TO_UTF8(*motherBoxKey);

	FString json = FString::Printf(TEXT(R"({
		"%i": {
			"IsOpen": true,
			"AcceptsAnyGift": true,
			"DesiredTraits": []
		}
	})"), slotData.currentPlayerSlot);
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
	TArray<TSharedPtr<FJsonObject>> giftsToReject;

	for (TPair<FString, TSharedPtr<FJsonValue>> pair : parsedJson->Values) {
		TSharedPtr<FJsonObject> gift = pair.Value->AsObject();
		TSharedPtr<FJsonObject> giftItem = gift->GetObjectField("Item");
		FString itemName = giftItem->GetStringField("Name");
		int amount = giftItem->GetIntegerField("Amount");

		if (NameToItemMapping.Contains(itemName)) {
			portalSubSystem->Enqueue(NameToItemMapping[itemName], amount);
		} else {
			TArray<TSharedPtr<FJsonValue>> giftTraits = gift->GetArrayField("Traits");
			TSubclassOf<UFGItemDescriptor> itemClass = TryGetItemClassByTraits(giftTraits);

			if (itemClass != nullptr) {
				portalSubSystem->Enqueue(itemClass, amount);
			} else {
				giftsToReject.Add(gift);
			}
		}

		FString id = gift->GetStringField("Id");
		procesedGifts.Add(id);
	}

	HandleProcessedGifts(procesedGifts, giftsToReject);
}

void AApGiftingSubsystem::HandleProcessedGifts(TArray<FString> procesedGifts, TArray<TSharedPtr<FJsonObject>> giftsToReject) {
	for (TSharedPtr<FJsonObject> rejectedGift : giftsToReject) {
		//TODO swap sender/receiver & send to other giftbox
	};

	FString giftboxKey = FString::Printf(TEXT("GiftBox;%i;%i"), slotData.currentPlayerTeam, slotData.currentPlayerSlot);

	AP_SetServerDataRequest removeProcesedGifts;
	removeProcesedGifts.key = TCHAR_TO_UTF8(*giftboxKey);

	std::vector<AP_DataStorageOperation> operations;

	for (FString id : procesedGifts) {
		std::string guid = TCHAR_TO_UTF8(*id);

		AP_DataStorageOperation update;
		update.operation = "pop";
		update.value = &guid;

		operations.push_back(update);
	}

	std::string defaultGiftboxValue = "{}";

	removeProcesedGifts.operations = operations;
	removeProcesedGifts.default_value = &defaultGiftboxValue;
	removeProcesedGifts.type = AP_DataType::Raw;
	removeProcesedGifts.want_reply = false;

	ap->SetServerData(&removeProcesedGifts);
}



TSubclassOf<UFGItemDescriptor> AApGiftingSubsystem::TryGetItemClassByTraits(TArray<TSharedPtr<FJsonValue>> traits) {
	//TODO process item traits and quality, like "Metal" with quality 2 might be Encased Steam Beam
	 





	return nullptr;
}

void AApGiftingSubsystem::Send(TMap<int, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend) {
	for (TPair<int, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSendPerPlayer : itemsToSend) {

		FString giftboxKey = FString::Printf(TEXT("GiftBox;%i;%i"), slotData.currentPlayerTeam, itemsToSendPerPlayer.Key);

		AP_SetServerDataRequest addToGiftBox;
		addToGiftBox.key = TCHAR_TO_UTF8(*giftboxKey);

		std::vector<AP_DataStorageOperation> operations;

		FString sender = ap->GetPlayerName(slotData.currentPlayerSlot);
		FString receiver = ap->GetPlayerName(itemsToSendPerPlayer.Key);

		for (TPair<TSubclassOf<UFGItemDescriptor>, int> stack : itemsToSendPerPlayer.Value) {
			FString guid = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
			FString itemName = ItemToNameMapping[stack.Key];
			int64_t itemValue = 0;
			FString traits = TEXT("");
			FString json = FString::Printf(TEXT(R"({
				"%s": {
					"ID": "%s",
					"Item": {
						"Name": "%s",
						"Amount": %i,
						"Value": %i
					},
					"Traits": [ %s ],
					"Sender": "%s",
					"Receiver": "%s",
					"SenderTeam": %i,
					"ReceiverTeam": %i,
					"IsRefund": false
				}
			})"), *guid, *guid, *itemName, stack.Value, itemValue, *traits, *sender, *receiver, slotData.currentPlayerTeam, slotData.currentPlayerTeam);
			std::string stdJson = TCHAR_TO_UTF8(*json);

			AP_DataStorageOperation update;
			update.operation = "update";
			update.value = &stdJson;

			operations.push_back(update);
		}

		std::string defaultGiftboxValue = "{}";

		addToGiftBox.operations = operations;
		addToGiftBox.default_value = &defaultGiftboxValue;
		addToGiftBox.type = AP_DataType::Raw;
		addToGiftBox.want_reply = false;

		ap->SetServerData(&addToGiftBox);
	}
}

#pragma optimize("", on)