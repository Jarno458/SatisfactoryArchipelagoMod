#pragma once

#include "CoreMinimal.h"

#include "ApGiftingMappings.generated.h"

UCLASS()
class ARCHIPELAGO_API UApGiftingMappings : public UObject
{
	GENERATED_BODY()

public:
	static const TMap<FString, int64> HardcodedItemNameToIdMappings;
	static const TMap<FString, FString> TraitParents;
	static const TMap<FString, int64> TraitDefaultItemIds;
	static const TMap<int64, TMap<FString, float>> TraitsPerItemRatings;
	static const TMap<int64, int> HardcodedSinkValues;
};
