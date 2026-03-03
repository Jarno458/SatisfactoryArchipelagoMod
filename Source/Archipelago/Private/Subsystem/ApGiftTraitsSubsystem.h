#pragma once

#include "CoreMinimal.h"
#include "ApMappingsSubsystem.h"
#include "ApTypes.h"
#include "FGResourceSinkSubsystem.h"

#include "Subsystem/ModSubsystem.h"
#include "Resources/FGItemDescriptor.h"

#include "ApGiftTraitsSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApGiftTraitsSubsystem, Log, All);

UCLASS()
class AApGiftTraitsSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApGiftTraitsSubsystem();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Gift Traits Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApGiftTraitsSubsystem* Get(UObject* worldContext);
	static AApGiftTraitsSubsystem* Get(UWorld* world);

private:
	AApMappingsSubsystem* mappingSubsystem;

	TSet<EGiftTrait> AllTraits;
	TArray<TSubclassOf<UFGItemDescriptor>> AllItems;
	TMap<TSubclassOf<UFGItemDescriptor>, FApTraitValues> TraitsPerItem;

	bool hasLoadedTraits = false;

	TMap<uint32, TSubclassOf<UFGItemDescriptor>> ItemPerTraitsHashCache;

public:
	FORCEINLINE bool HasLoadedItemTraits() const { return hasLoadedTraits; };

	UFUNCTION(BlueprintPure)
	TSet<EGiftTrait> GetAllTraits() const { return AllTraits; }

	UFUNCTION(BlueprintPure)
	TSet<EGiftTrait> GetTraitEnumsPerItem(TSubclassOf<UFGItemDescriptor> itemClass) const;

	UFUNCTION(BlueprintPure)
	TArray<FApGiftTrait> GetFullTraitsForItem(TSubclassOf<UFGItemDescriptor> itemClass) const;

	FApTraitValues GetTraitsForItem(TSubclassOf<UFGItemDescriptor> itemClass) const;

	TArray<TSubclassOf<UFGItemDescriptor>> GetItemsWithOverlappingTraits(FApTraitBits traits) const;

	TArray<TSubclassOf<UFGItemDescriptor>> GetAllItems() const;

	TSubclassOf<UFGItemDescriptor> TryGetItemClassByTraits(TArray<FApGiftTrait>& traits);

	static bool HasTraitKnownToSatisfactory(TArray<FApGiftTrait>& traits);

	static uint32 GetTraitsHash(TArray<FApGiftTrait>& traits);

private:
	void LoadTraitMappings();
	static int GetResourceSinkPointsForItem(AFGResourceSinkSubsystem* resourceSinkSubsystem, TSubclassOf<UFGItemDescriptor> itemClass, int64 itemId);
	static float GetTraitValue(int itemValue, float avarageItemValueForTrait, float itemSpecificTraitMultiplier);
	void PrintTraitValuesPerItem();

	UFUNCTION() //required for event hookup
	void OnClientSubsystemsValid();

	friend class AApVaultSubsystem;
};
