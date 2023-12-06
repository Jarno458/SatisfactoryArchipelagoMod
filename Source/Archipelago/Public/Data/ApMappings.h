#pragma once

#include "CoreMinimal.h"

#include "ApMappings.generated.h"

#define MISSING_ITEMID_DEVELOPER_BACKUP 1338151

/**
 * 
 */
UCLASS()
class ARCHIPELAGO_API UApMappings : public UObject
{
	GENERATED_BODY()

public:
	static const TMap<int64, FString> ItemIdToGameItemDescriptor;
	static const TMap<int64, FString> ItemIdToGameName2;
	static const TMap<int64, FString> ItemIdToGameRecipe;
	static const TMap<int64, FString> ItemIdToGameBuilding;
	static const TMap<int64, FName> ItemIdToTrap;
};
