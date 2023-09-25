#pragma once

#include "CoreMinimal.h"

#include "Subsystem/ModSubsystem.h"
#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "FGResourceSinkSubsystem.h"

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

	bool apInitialized;

	TMap<TSubclassOf<UFGItemDescriptor>, int64_t> ItemToItemId;
	TMap<FString, int> TraitAvarageValue;

	static const TMap<FString, int64_t> TraitDefaultItemIds;
	static const TMap<int64_t, TMap<FString, float>> TraitsPerItem;
	static const TMap<FString, FString> TraitParents;
	static const TMap<int64_t, int> HardcodedSinkValues;

	TMap<FApPlayer, TSharedPtr<TQueue<FInventoryStack, EQueueMode::Mpsc>>> InputQueue;

	TSet<FString> ProcessedIds;

	AApSubsystem* ap;
	AApPortalSubsystem* portalSubSystem;
	AFGResourceSinkSubsystem* resourceSinkSubsystem;
	AApMappingsSubsystem* mappingSubsystem;

	FDateTime lastPoll = FDateTime::Now();

public:
	void EnqueueForSending(FApPlayer targetPlayer, FInventoryStack itemStack);

private:
	void LoadItemNameMapping();

	void PullAllGiftsAsync();
	void ProcessInputQueue();

	void Send(TMap<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend);

	TSubclassOf<UFGItemDescriptor> TryGetItemClassByTraits(TArray<FApGiftTrait> traits);

	TArray<FApGiftTrait> GetTraitsForItem(int64_t itemId, int itemValue);

	void UpdatedProcessedIds(TArray<FApReceiveGift> gifts);

	int GetResourceSinkPointsForItem(TSubclassOf<UFGItemDescriptor> itemClass, int64_t itemId);
};
