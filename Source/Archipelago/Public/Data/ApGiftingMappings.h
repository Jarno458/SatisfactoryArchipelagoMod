#pragma once

#include "CoreMinimal.h"

#include "ApGiftingMappings.generated.h"
// EGiftTrait is an interger value between 0 and 64
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
	Berry, //1.0
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
	Statue,
	Copy, //new traits
	Teleport,
	Angular,
	Liquid,
	RangedWeapon,
	MeleeWeapon,
	Neutralizing, //new new traits
	Explosive,
	Pipe,
	Insect,
	Energy
	//59-63 unused
};
ENUM_RANGE_BY_FIRST_AND_LAST(EGiftTrait, EGiftTrait::Electronics, EGiftTrait::Statue)

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
