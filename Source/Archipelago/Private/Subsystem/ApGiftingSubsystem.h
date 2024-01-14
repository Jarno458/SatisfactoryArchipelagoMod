#pragma once

#include "CoreMinimal.h"

#include "Subsystem/ModSubsystem.h"
#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "FGResourceSinkSubsystem.h"
#include "ApUtils.h"

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

	static AApGiftingSubsystem* Get(class UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get ApGiftingSubsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApGiftingSubsystem* Get(UObject* worldContext);

	TMap<FApPlayer, FApGiftBoxMetaData> AcceptedGiftTraitsPerPlayer;

private:
	static const int pollInterfall = 10;

	bool apInitialized;

	TMap<TSubclassOf<UFGItemDescriptor>, TMap<FString, float>> TraitsPerItem;
	TMap<uint32, TSubclassOf<UFGItemDescriptor>> ItemPerTraitsHashCache;

	static const TMap<FString, int64> HardcodedItemNameToIdMappings;
	static const TMap<FString, int64> TraitDefaultItemIds;
	static const TMap<int64, TMap<FString, float>> TraitsPerItemRatings;
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
	TArray<FApPlayer> GetAllApPlayers(); // forwarded from ApSubsystem

private:
	void LoadMappings();

	void UpdateAcceptedGifts();
	void PullAllGiftsAsync();
	void ProcessInputQueue();

	void Send(TMap<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend);

	TSubclassOf<UFGItemDescriptor> TryGetItemClassByTraits(TArray<FApGiftTrait> traits);

	TArray<FApGiftTrait> GetTraitsForItem(TSubclassOf<UFGItemDescriptor> itemClass, int itemValue);

	void UpdatedProcessedIds(TArray<FApReceiveGift> gifts);

	int GetResourceSinkPointsForItem(TSubclassOf<UFGItemDescriptor> itemClass, int64 itemId);

	bool HasTraitKnownToSatisfactory(TArray<FApGiftTrait> traits);

	uint32 GetTraitsHash(TArray<FApGiftTrait> traits);
	float GetTraitValue(int itemValue, float defaultValueForTrait, float itemTraitMultiplier);

	void PrintTraitValuesPerItem();
};
