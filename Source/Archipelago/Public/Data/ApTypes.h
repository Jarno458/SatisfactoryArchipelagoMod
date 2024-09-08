#pragma once

#include "CoreMinimal.h"

#include "Engine/UserDefinedStruct.h"
#include "JsonObjectConverter.h"
#include "Data/ApGiftingMappings.h"

#include "ApTypes.generated.h"


USTRUCT(BlueprintType)
struct ARCHIPELAGO_API FApPlayer
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, SaveGame)
	uint8 Team;
	UPROPERTY(BlueprintReadWrite, SaveGame)
	FString Name;

	//Override the comparison operator
	bool operator==(const FApPlayer& Other) const
	{
		return Team == Other.Team && Name.Equals(Other.Name);
	}

	// Value constructor
	FApPlayer(int team, FString name) : Team(team), Name(name)
	{}
	// Default constructor
	FApPlayer() : FApPlayer(-1, "Invalid")
	{}
	// Copy constructor
	FApPlayer(const FApPlayer& Other) : Team(Other.Team), Name(Other.Name)
	{}

	FORCEINLINE bool IsValid() {
		return this->Team >= 0;
	}

	FORCEINLINE FString toString(const FApPlayer& This) {
		FString out;
		FJsonObjectConverter::UStructToJsonObjectString(This, out);
		return out;
	}
};

FORCEINLINE uint32 GetTypeHash(const FApPlayer& This)
{
	return HashCombine(GetTypeHash(This.Team), GetTypeHash(This.Name));
};


USTRUCT(BlueprintType)
struct ARCHIPELAGO_API FApGiftTrait
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	EGiftTrait Trait;
	UPROPERTY(BlueprintReadOnly)
	float Quality;
	UPROPERTY(BlueprintReadOnly)
	float Duration;
};

USTRUCT()
struct ARCHIPELAGO_API FApGift
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString ItemName;
	UPROPERTY()
	int Amount;
	UPROPERTY()
	int64 ItemValue;
	UPROPERTY()
	TArray<FApGiftTrait> Traits;
};

USTRUCT()
struct ARCHIPELAGO_API FApSendGift : public FApGift
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FApPlayer Receiver;
};

USTRUCT()
struct ARCHIPELAGO_API FApReceiveGift : public FApGift
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString Id;
	UPROPERTY()
	int SenderSlot;
	UPROPERTY()
	int SenderTeam;
};

USTRUCT()
struct ARCHIPELAGO_API FApNetworkItem
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	int64 item;

	UPROPERTY(SaveGame)
	int64 location;

	UPROPERTY(SaveGame)
	int player;

	UPROPERTY(SaveGame)
	int flags;

	UPROPERTY(SaveGame)
	FString itemName;

	UPROPERTY(SaveGame)
	FString locationName;

	UPROPERTY(SaveGame)
	FString playerName;
};

#define ACCEPT_ALL_TRAITS_BIT 0x8000000000000000

USTRUCT()
struct ARCHIPELAGO_API FApTraitBits
{
	GENERATED_BODY()

public:
	// Value constructor
	FApTraitBits(TMap<EGiftTrait, float> traitValues)
	{
		data = 0;
		for (const TPair<EGiftTrait, float>& traitValue : traitValues) {
			data |= ((uint64)1 << (uint8)traitValue.Key);
		}
	}
	FApTraitBits(bool acceptAllTraits, TSet<EGiftTrait> acceptedTraits)
	{
		data = acceptAllTraits ? ACCEPT_ALL_TRAITS_BIT : 0;
		for (EGiftTrait trait : acceptedTraits) {
			data |= ((uint64)1 << (uint8)trait);
		}
	}
	// Copy constructor
	FApTraitBits(const FApTraitBits& Other) : data(Other.data)
	{}
	// Default constructor
	FApTraitBits() : data(0)
	{}

private:
	UPROPERTY()
	uint64 data;

public:
	FORCEINLINE bool AcceptsAllTraits() const { return (data & ACCEPT_ALL_TRAITS_BIT) > 0; }

	FORCEINLINE bool AcceptsTrait(EGiftTrait trait) const { return (data & ((uint64)1 << (uint8)trait)) > 0; }

	FORCEINLINE bool HasOverlap(FApTraitBits traitBits) const { return (data & (traitBits.data | ACCEPT_ALL_TRAITS_BIT)) > 0; }

	const TSet<EGiftTrait> GetTraits() const {
		static const uint8 numberOfEnumValues = StaticEnum<EGiftTrait>()->NumEnums();
		TSet<EGiftTrait> traits;
		for (uint8 i = 0; i < numberOfEnumValues; i++) {
			if ((data & ((uint64)1 << i)) > 0) {
				traits.Add((EGiftTrait)i);
			}
		}
		return traits;
	}

	//Override the comparison operator
	bool operator==(const FApTraitBits& Other) const
	{
		return data == Other.data;
	}
};

USTRUCT()
struct ARCHIPELAGO_API FApTraitValues : public FApTraitBits
{
	GENERATED_BODY()

public:
	// Value constructor
	FApTraitValues(TMap<EGiftTrait, float> traitValues) : FApTraitBits(traitValues), TraitsValues(traitValues)
	{}
	// Copy constructor
	FApTraitValues(const FApTraitValues& Other) : FApTraitBits(Other), TraitsValues(Other.TraitsValues)
	{}
	// Default constructor
	FApTraitValues() : FApTraitBits(TMap<EGiftTrait, float>()), TraitsValues(TMap<EGiftTrait, float>())
	{}

public:
	UPROPERTY()
	TMap<EGiftTrait, float> TraitsValues;
};

USTRUCT()
struct ARCHIPELAGO_API FApTraitByPlayer : public FApTraitBits
{
	GENERATED_BODY()

public:
	// Value constructor
	FApTraitByPlayer(FApPlayer player, bool acceptAllTraits, TSet<EGiftTrait> acceptedTraits)	: FApTraitBits(acceptAllTraits, acceptedTraits), Player(player)
	{}
	FApTraitByPlayer(FApPlayer player, FApTraitBits acceptedTraits) : FApTraitBits(acceptedTraits), Player(player)
	{}
	// Copy constructor
	FApTraitByPlayer(const FApTraitByPlayer& Other)	: FApTraitBits(Other), Player(Other.Player)
	{}
	// Default constructor
	FApTraitByPlayer() : FApTraitBits(TMap<EGiftTrait, float>()), Player()
	{}

public:
	UPROPERTY()
	FApPlayer Player;
};