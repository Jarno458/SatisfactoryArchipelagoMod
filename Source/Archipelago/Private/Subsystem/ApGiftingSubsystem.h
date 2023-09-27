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

	TMap<FApPlayer, FApGiftBoxMetaData> AcceptedGiftTraitsPerPlayer;

private:
	static const int pollInterfall = 10;

	bool apInitialized;

	TMap<TSubclassOf<UFGItemDescriptor>, int64> ItemToItemId;
	TMap<FString, int> TraitAvarageValue;
	TArray<FString> AllTraits;

	static const TMap<FString, int64> TraitDefaultItemIds;
	static const TMap<int64, TMap<FString, float>> TraitsPerItem;
	static const TMap<FString, FString> TraitParents;
	static const TMap<int64, int> HardcodedSinkValues;

	TMap<FApPlayer, TSharedPtr<TQueue<FInventoryStack, EQueueMode::Mpsc>>> InputQueue;

	TSet<FString> ProcessedIds;

	AApSubsystem* ap;
	AApPortalSubsystem* portalSubSystem;
	AFGResourceSinkSubsystem* resourceSinkSubsystem;
	AApMappingsSubsystem* mappingSubsystem;

	FDateTime lastPoll = FDateTime::Now();

public:
	void EnqueueForSending(FApPlayer targetPlayer, FInventoryStack itemStack);
	bool CanSend(FApPlayer targetPlayer, FInventoryItem item);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FApPlayer> GetPlayersAcceptingGifts();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FString> GetAcceptedTraitsPerPlayer(FApPlayer player);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FString> GetTraitPerItem(TSubclassOf<UFGItemDescriptor> itemClass);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool DoesPlayerAcceptGiftTrait(FApPlayer player, FString giftTrait);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FApPlayer> GetAllPlayers(); // forwarded form ApSubsystem

private:
	void LoadItemNameMapping();

	void UpdateAcceptedGifts();
	void PullAllGiftsAsync();
	void ProcessInputQueue();

	void Send(TMap<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend);

	TSubclassOf<UFGItemDescriptor> TryGetItemClassByTraits(TArray<FApGiftTrait> traits);

	TArray<FApGiftTrait> GetTraitsForItem(int64 itemId, int itemValue);

	void UpdatedProcessedIds(TArray<FApReceiveGift> gifts);

	int GetResourceSinkPointsForItem(TSubclassOf<UFGItemDescriptor> itemClass, int64 itemId);
};
