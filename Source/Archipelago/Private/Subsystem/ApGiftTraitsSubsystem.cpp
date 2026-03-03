#include "ApGiftTraitsSubsystem.h"

#include "ApMappingsSubsystem.h"
#include "FGGameState.h"
#include "Logging/StructuredLog.h"
#include "ApUtils.h"
#include "FGResourceSinkSubsystem.h"

DEFINE_LOG_CATEGORY(LogApGiftTraitsSubsystem);

AApGiftTraitsSubsystem* AApGiftTraitsSubsystem::Get(UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApGiftTraitsSubsystem>();
}

AApGiftTraitsSubsystem* AApGiftTraitsSubsystem::Get(UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApGiftTraitsSubsystem::AApGiftTraitsSubsystem() : Super() {
	UE_LOGFMT(LogApGiftTraitsSubsystem, Display, "AApGiftTraitsSubsystem::AApGiftTraitsSubsystem()");
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnLocal;

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AApGiftTraitsSubsystem::BeginPlay() {
	UE_LOGFMT(LogApGiftTraitsSubsystem, Display, "AApGiftTraitsSubsystem::BeginPlay()");

	AllTraits.Empty();
	for (EGiftTrait trait : TEnumRange<EGiftTrait>()) {
		AllTraits.Add(trait);
	}

	UWorld* world = GetWorld();
	fgcheck(world != nullptr);
	AGameStateBase* gameState = world->GetGameState();

	mappingSubsystem = AApMappingsSubsystem::Get(world);

	if (gameState->HasAuthority()) {
		LoadTraitMappings();
	}
	else {
		if (AFGGameState* factoryGameState = Cast<AFGGameState>(gameState)) {
			factoryGameState->mOnClientSubsystemsValid.AddDynamic(this, &AApGiftTraitsSubsystem::OnClientSubsystemsValid);

			if (factoryGameState->AreClientSubsystemsValid()) {
				LoadTraitMappings();
			}
		}
	}
}

void AApGiftTraitsSubsystem::OnClientSubsystemsValid() {
	LoadTraitMappings();
}

TSet<EGiftTrait> AApGiftTraitsSubsystem::GetTraitEnumsPerItem(TSubclassOf<UFGItemDescriptor> itemClass) const {
	FApTraitValues traits = GetTraitsForItem(itemClass);

	if (traits.AcceptsAllTraits())
		return AllTraits;

	return traits.GetTraits();
}

FApTraitValues AApGiftTraitsSubsystem::GetTraitsForItem(TSubclassOf<UFGItemDescriptor> itemClass) const {
	if (!hasLoadedTraits || !TraitsPerItem.Contains(itemClass))
		return FApTraitValues();

	return TraitsPerItem[itemClass];
}

TArray<FApGiftTrait> AApGiftTraitsSubsystem::GetFullTraitsForItem(TSubclassOf<UFGItemDescriptor> itemClass) const {
	if (!hasLoadedTraits || !TraitsPerItem.Contains(itemClass))
		return TArray<FApGiftTrait>();

	TArray<FApGiftTrait> traits;

	for (const TPair<EGiftTrait, float>& trait : TraitsPerItem[itemClass].TraitsValues) {
		FApGiftTrait traitSpecification;
		traitSpecification.Trait = trait.Key;
		traitSpecification.Quality = trait.Value;
		traitSpecification.Duration = 1.0f;

		traits.Add(traitSpecification);
	}

	return traits;
}

TArray<TSubclassOf<UFGItemDescriptor>> AApGiftTraitsSubsystem::GetItemsWithOverlappingTraits(FApTraitBits traits) const {
	if (traits.AcceptsAllTraits())
		return AllItems;

	TArray<TSubclassOf<UFGItemDescriptor>> itemClasses;

	for (const TPair<TSubclassOf<UFGItemDescriptor>, FApTraitValues>& traitsPerItem : TraitsPerItem) {
		if (traitsPerItem.Value.HasOverlap(traits))
			itemClasses.Add(traitsPerItem.Key);
	}

	return itemClasses;
}

TArray<TSubclassOf<UFGItemDescriptor>> AApGiftTraitsSubsystem::GetAllItems() const
{
	return AllItems;
}

void AApGiftTraitsSubsystem::LoadTraitMappings() {
	AFGResourceSinkSubsystem* resourceSinkSubsystem = AFGResourceSinkSubsystem::Get(GetWorld());
	fgcheck(resourceSinkSubsystem)

		TMap<EGiftTrait, float> defaultSinkPointsPerTrait;

	for (const TPair<EGiftTrait, int64>& traitDefault : UApGiftingMappings::TraitDefaultItemIds) {
		fgcheck(mappingSubsystem->ApItems.Contains(traitDefault.Value) && mappingSubsystem->ApItems[traitDefault.Value]->Type == EItemType::Item);

		TSharedRef<FApItem> itemInfo = StaticCastSharedRef<FApItem>(mappingSubsystem->ApItems[traitDefault.Value]);

		int defaultItemSinkPoints = GetResourceSinkPointsForItem(resourceSinkSubsystem, itemInfo->Class, traitDefault.Value);

		defaultSinkPointsPerTrait.Add(traitDefault.Key, defaultItemSinkPoints);
	}

	for (TPair<int64, TSharedRef<FApItemBase>>& itemInfoMapping : mappingSubsystem->ApItems) {
		if (!UApGiftingMappings::TraitsPerItemRatings.Contains(itemInfoMapping.Key))
			continue;

		fgcheck(itemInfoMapping.Value->Type == EItemType::Item)

			TSharedRef<FApItem> itemInfo = StaticCastSharedRef<FApItem>(itemInfoMapping.Value);
		TSubclassOf<UFGItemDescriptor> itemClass = itemInfo->Class;
		int64 itemId = itemInfoMapping.Key;

		int itemValue = GetResourceSinkPointsForItem(resourceSinkSubsystem, itemClass, itemId);

		for (const TPair<EGiftTrait, float>& traitRelativeRating : UApGiftingMappings::TraitsPerItemRatings[itemInfoMapping.Key]) {
			EGiftTrait traitName = traitRelativeRating.Key;

			fgcheck(defaultSinkPointsPerTrait.Contains(traitName));
			float traitValue = GetTraitValue(itemValue, defaultSinkPointsPerTrait[traitName], traitRelativeRating.Value);

			TMap<EGiftTrait, float> traitValueForItem;
			traitValueForItem.Add(traitName, traitValue);

			while (UApGiftingMappings::TraitParents.Contains(traitName)) {
				traitName = UApGiftingMappings::TraitParents[traitName];

				if (!traitValueForItem.Contains(traitName)) {
					fgcheck(defaultSinkPointsPerTrait.Contains(traitName));
					traitValue = GetTraitValue(itemValue, defaultSinkPointsPerTrait[traitName], traitRelativeRating.Value);
					traitValueForItem.Add(traitName, traitValue);
				}
			}

			if (!TraitsPerItem.Contains(itemInfo->Class)) {
				TraitsPerItem.Add(itemInfo->Class, FApTraitValues(traitValueForItem));
			}
			else {
				TraitsPerItem[itemInfo->Class].TraitsValues.Append(traitValueForItem);
				TraitsPerItem[itemInfo->Class] = FApTraitValues(TraitsPerItem[itemInfo->Class].TraitsValues);
			}
		}
	}

	TraitsPerItem.GenerateKeyArray(AllItems);

	//PrintTraitValuesPerItem();

	hasLoadedTraits = true;
}

int AApGiftTraitsSubsystem::GetResourceSinkPointsForItem(AFGResourceSinkSubsystem* resourceSinkSubsystem, TSubclassOf<UFGItemDescriptor> itemClass, int64 itemId) {
	if (UApGiftingMappings::HardcodedSinkValues.Contains(itemId))
		return UApGiftingMappings::HardcodedSinkValues[itemId];

	int value = resourceSinkSubsystem->GetResourceSinkPointsForItem(itemClass);

	if (value == 0) {
		FString itemName = UFGItemDescriptor::GetItemName(itemClass).ToString();
		UE_LOGFMT(LogApGiftTraitsSubsystem, Error, "AApMappingsSubsystem::GetResourceSinkPointsForItem(\"{0}\", {1}) No sink value for item", itemName, itemId);
		value = 1;
	}

	return value;
}

float AApGiftTraitsSubsystem::GetTraitValue(int itemValue, float avarageItemValueForTrait, float itemSpecificTraitMultiplier) {
	return (FPlatformMath::LogX(10, (double)itemValue + 0.1) / FPlatformMath::LogX(10, (double)avarageItemValueForTrait + 0.1)) * itemSpecificTraitMultiplier;
}

void AApGiftTraitsSubsystem::PrintTraitValuesPerItem() {
	TMap<EGiftTrait, TSortedMap<float, FString>> valuesPerItem;

	for (const TPair<TSubclassOf<UFGItemDescriptor>, FApTraitValues>& traitsPerItem : TraitsPerItem) {
		for (TPair<EGiftTrait, float> trait : traitsPerItem.Value.TraitsValues) {
			if (!valuesPerItem.Contains(trait.Key))
				valuesPerItem.Add(trait.Key, TSortedMap<float, FString>());

			FString itemName = mappingSubsystem->ItemIdToName[mappingSubsystem->ItemClassToItemId[traitsPerItem.Key]];

			valuesPerItem[trait.Key].Add(trait.Value, itemName);
		}
	}

	TArray<FString> lines;
	for (const TPair<EGiftTrait, TSortedMap<float, FString>>& traitsPerItem : valuesPerItem) {
		static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();
		FName traitName = giftTraitEnum->GetNameByValue((int64)traitsPerItem.Key);

		lines.Add(FString::Printf(TEXT("Trait: \"%s\":"), *traitName.ToString()));

		for (TPair<float, FString> valuePerItem : traitsPerItem.Value)
			lines.Add(FString::Printf(TEXT("  - Item: \"%s\": %.2f"), *valuePerItem.Value, valuePerItem.Key));
	}

	FString fileText = FString::Join(lines, TEXT("\n"));

	UApUtils::WriteStringToFile(fileText, TEXT("T:\\ItemTraits.txt"), false);
}

bool AApGiftTraitsSubsystem::HasTraitKnownToSatisfactory(TArray<FApGiftTrait>& traits) {
	static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

	for (const FApGiftTrait& trait : traits) {
		if (giftTraitEnum->IsValidEnumValue((uint8)trait.Trait))
			return true;
	}

	return false;
}

TSubclassOf<UFGItemDescriptor> AApGiftTraitsSubsystem::TryGetItemClassByTraits(TArray<FApGiftTrait>& traits) {
	if (!HasTraitKnownToSatisfactory(traits))
		return nullptr;

	uint32 hash = GetTraitsHash(traits);
	if (ItemPerTraitsHashCache.Contains(hash))
	{
		UE_LOGFMT(LogApGiftTraitsSubsystem, Display, "AApGiftTraitsSubsystem::TryGetItemClassByTraits() matched on hash");
		return ItemPerTraitsHashCache[hash];
	}

	TMap<TSubclassOf<UFGItemDescriptor>, TPair<int, float>> numberOfMatchesAndTotalDiviationPerItemClass;
	int mostMatches = 0;

	for (const TPair<TSubclassOf<UFGItemDescriptor>, FApTraitValues>& traitsPerItem : TraitsPerItem) {
		int matches = 0;
		float totalDifference = 0.0f;

		for (FApGiftTrait& trait : traits) {
			if (traitsPerItem.Value.AcceptsTrait(trait.Trait)) {
				totalDifference += FGenericPlatformMath::Abs(traitsPerItem.Value.TraitsValues[trait.Trait] - trait.Quality);
				matches++;
			}
		}

		if (matches >= mostMatches) {
			mostMatches = matches;
			numberOfMatchesAndTotalDiviationPerItemClass.Add(traitsPerItem.Key, TPair<int, float>(matches, totalDifference));
		}
	}

	float lowestDifference = 0.0f;
	TMap<TSubclassOf<UFGItemDescriptor>, float> accurencyPerItem;
	TSubclassOf<UFGItemDescriptor> itemClassWithLowestDifference = nullptr;

	for (const TPair<TSubclassOf<UFGItemDescriptor>, TPair<int, float>>& numberOfMatchesAndTotalDiviation : numberOfMatchesAndTotalDiviationPerItemClass) {
		if (numberOfMatchesAndTotalDiviation.Value.Key != mostMatches)
			continue;

		if (itemClassWithLowestDifference == nullptr || numberOfMatchesAndTotalDiviation.Value.Value < lowestDifference) {
			lowestDifference = numberOfMatchesAndTotalDiviation.Value.Value;
			itemClassWithLowestDifference = numberOfMatchesAndTotalDiviation.Key;
		}
	}

	ItemPerTraitsHashCache.Add(hash, itemClassWithLowestDifference);

	return itemClassWithLowestDifference;
}

uint32 AApGiftTraitsSubsystem::GetTraitsHash(TArray<FApGiftTrait>& traits) {
	TSortedMap<EGiftTrait, uint32> hashesPerTrait;

	for (FApGiftTrait& trait : traits)
		hashesPerTrait.Add(trait.Trait, HashCombine(GetTypeHash(trait.Trait), GetTypeHash(trait.Quality)));

	uint32 totalHash = 0;

	TArray<uint32> hashes;
	hashesPerTrait.GenerateValueArray(hashes);

	for (uint32 hash : hashes)
		totalHash = HashCombine(totalHash, hash);

	return totalHash;
}

