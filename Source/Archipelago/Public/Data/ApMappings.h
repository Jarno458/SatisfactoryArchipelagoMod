#pragma once

#include "CoreMinimal.h"

#include "ApMappings.generated.h"

UENUM()
enum class EApMappingsSpecailItemType : uint8
{
	Inventory3,
	Inventory6,
	Toolbelt1,
};

UCLASS()
class ARCHIPELAGO_API UApMappings : public UObject
{
	GENERATED_BODY()

public:
	static const TMap<int64, FString> ItemIdToGameItemDescriptor;
	static const TMap<int64, EApMappingsSpecailItemType> ItemIdToSpecailItemType;
	static const TMap<int64, FString> ItemIdToGameRecipe;
	static const TMap<int64, TArray<FString>> ItemIdToGameBuilding;
	static const TMap<int64, FString> ItemIdToGameSchematic;
	static const TMap<int64, FName> ItemIdToTrap;
};
