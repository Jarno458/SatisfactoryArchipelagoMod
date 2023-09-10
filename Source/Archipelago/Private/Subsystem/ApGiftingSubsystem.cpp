#include "ApGiftingSubsystem.h"

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

			const FApSlotData slotData = ap->GetSlotData();
			OpenGiftbox(slotData);

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

		ItemNameMapping.Add(itemName, itemClass);
	}
}

void AApGiftingSubsystem::OpenGiftbox(const FApSlotData slotData) {
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
	
	for (TPair<FString, TSharedPtr<FJsonValue>> pair : parsedJson->Values)
	{
		TSharedPtr<FJsonObject> gift = pair.Value->AsObject();
		TSharedPtr<FJsonObject> giftItem = gift->GetObjectField("Item");
		FString itemName = giftItem->GetStringField("Name");
		int amount = giftItem->GetIntegerField("Amount");

		if (ItemNameMapping.Contains(itemName))
			portalSubSystem->Enqueue(ItemNameMapping[itemName], amount);
		else {
			TArray<TSharedPtr<FJsonValue>> giftTraits = gift->GetArrayField("Traits");
			TSubclassOf<UFGItemDescriptor> itemClass = TryGetItemClassByTraits(giftTraits);

			if (itemClass != nullptr) {
				portalSubSystem->Enqueue(itemClass, amount);
			}
			else {
				//TODO refund we dont want it
			}
		}
	}
}

TSubclassOf<UFGItemDescriptor> AApGiftingSubsystem::TryGetItemClassByTraits(TArray<TSharedPtr<FJsonValue>> traits) {
	//TODO process item traits and quality, like "Metal" with quality 2 might be Encased Steam Beam
	
	return nullptr;
}

#pragma optimize("", on)