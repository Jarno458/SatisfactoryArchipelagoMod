#pragma once

#include "CoreMinimal.h"

#include "ApMappings.generated.h"

UENUM()
enum class EApMappingsSpecialItemType : uint8
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
	static const TMap<int64, const FString> ItemIdToGameItemDescriptor;
	static const TMap<int64, const EApMappingsSpecialItemType> ItemIdToSpecialItemType;
	static const TMap<int64, const TArray<FString>> ItemIdToGameRecipe;
	static const TMap<int64, const TArray<FString>> ItemIdToGameBuilding;
	static const TMap<int64, const FString> ItemIdToGameSchematic;
	static const TMap<int64, const FName> ItemIdToTrap;
};
