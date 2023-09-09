#pragma once

#include "CoreMinimal.h"

#include "ApMappings.generated.h"

/**
 * 
 */
UCLASS()
class ARCHIPELAGO_API UApMappings : public UObject
{
	GENERATED_BODY()

public:
	static const TMap<int64_t, FString> ItemIdToGameItemDescriptor;
	static const TMap<int64_t, FString> ItemIdToGameName2;
	static const TMap<int64_t, FString> ItemIdToGameRecipe;
	static const TMap<int64_t, FString> ItemIdToGameBuilding;
	static const TMap<int64_t, FName> ItemIdToTrap;
};
