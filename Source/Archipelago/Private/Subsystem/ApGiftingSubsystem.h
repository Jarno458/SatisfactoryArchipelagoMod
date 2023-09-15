#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "JsonObjectConverter.h"

#include "Subsystem/ModSubsystem.h"
#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "FGResourceSinkSubsystem.h"

#include "Data/ApGiftJson.h"

#include "ApGiftingSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApGiftingSubsystem, Log, All);

/**
 * 
 */
UCLASS()
class AApGiftingSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApGiftingSubsystem();

	virtual void BeginPlay() override;

	virtual void Tick(float dt) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get Archipelago Gifting Subsystem"))
	static AApGiftingSubsystem* Get();
	static AApGiftingSubsystem* Get(class UWorld* world);

private:
	static const int pollInterfall = 10;
	std::string defaultGiftboxValue = "{}";

	bool apInitialized;

	TMap<TSubclassOf<UFGItemDescriptor>, int64_t> ItemToItemId;
	TMap<FString, float> TraitDefaults;
	TMap<int64_t, TMap<FString, float>> TraitsPerItem;

	TMap<int, TSharedPtr<TQueue<FInventoryStack, EQueueMode::Mpsc>>> InputQueue;

	TQueue<FApGiftJson> GiftsToRefund;

	AApSubsystem* ap;
	AApPortalSubsystem* portalSubSystem;
	AFGResourceSinkSubsystem* resourceSinkSubsystem;
	AApMappingsSubsystem* mappingSubsystem;

	FDateTime lastPoll = FDateTime::Now();

public:
	void EnqueueForSending(int targetSlot, FInventoryStack itemStack);

private:
	void LoadItemNameMapping();

	void OpenGiftbox();
	void OnGiftsUpdated(AP_SetReply setReply);

	void PullAllGiftsAsync();
	void ProcessInputQueue();
	void HandleGiftsToReject();
	FString BuildTraitsJson(TArray<FApGiftTraitJson> Traits);

	void Send(TMap<int, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend);

	TSubclassOf<UFGItemDescriptor> TryGetItemClassByTraits(TArray<FApGiftTraitJson> traits);
	FString GetTraitsJsonForItem(int64_t itemId, int itemValue);
};
