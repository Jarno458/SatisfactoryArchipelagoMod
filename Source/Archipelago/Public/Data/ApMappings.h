#pragma once

#include "CoreMinimal.h"

#include "ApMappings.generated.h"

UENUM()
enum class EApMappingsSpecialItemType : uint8
{
	Inventory3,
	Inventory6,
	Toolbelt1,
	//InventoryUpload //removed as doesnt persist through death
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
	static const TMap<int64, const int> ItemIdToSpecailStackSize;
	static const TMap<int64, const FName> ItemIdToTrap;

#if WITH_EDITOR 
	static const TMap<int64, int> ItemIdToCouponCost;
#endif
};
