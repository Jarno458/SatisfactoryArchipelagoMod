#pragma once

#include "CoreMinimal.h"

#include "ApGiftingMappings.generated.h"

UENUM(BlueprintType)
enum class EGiftTrait : uint8
{
	Electronics = 0,
	Iron,
	Steel,
	Silver,
	Copper,
	Gold ,
	Metal,
	Material,
	Speed,
	Buff,
	Grass,
	Bomb,
	Weapon,
	Stone,
	Ore,
	Damage,
	Wood,
	Vegetable,
	Food,
	Seed,
	Fruit,
	Consumable,
	Mineral,
	Trap,
	Resource,
	Tool,
	Radioactive,
	Crystal,
	Artifact,
	Armor,
	Slowness,
	Fiber,
	Coal,
	Drink,
	Heal,

	//1.0
	Berry,
	Oil,
	Fuel,
	Water,
	Container,
	Cloth,
	Plastic,
	Diamond,
	Chemicals,
	Platinum,
	DarkMatter,
	SpaceMineral,
	Statue
};

UCLASS()
class ARCHIPELAGO_API UApGiftingMappings : public UObject
{
	GENERATED_BODY()

public:
	static const TMap<FString, int64> HardcodedItemNameToIdMappings;
	static const TMap<EGiftTrait, EGiftTrait> TraitParents;
	static const TMap<EGiftTrait, int64> TraitDefaultItemIds;
	static const TMap<int64, TMap<EGiftTrait, float>> TraitsPerItemRatings;
	static const TMap<int64, int> HardcodedSinkValues;
};
