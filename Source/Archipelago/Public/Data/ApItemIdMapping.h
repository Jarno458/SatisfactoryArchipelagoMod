

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ApItemIdMapping.generated.h"

/**
 * Holds manually populated data in static fields. Archipelago Item ID#s to game-usable names.
 */
UCLASS()
class ARCHIPELAGO_API UApItemIdMapping : public UObject
{
	GENERATED_BODY()

public:
	static TMap<int64_t, FString> ItemIdToGameItemDescriptor;
	static TMap<int64_t, FString> ItemIdToGameName2;
	static TMap<int64_t, FString> ItemIdToGameRecipe;
	static TMap<int64_t, FString> ItemIdToGameBuilding;
	static TMap<int64_t, FName>	  ItemIdToTrap;
};
