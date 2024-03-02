#pragma once

#include "CoreMinimal.h"

#include "Engine/UserDefinedStruct.h"
#include "JsonObjectConverter.h"

#include "ApTypes.generated.h"


USTRUCT(BlueprintType)
struct ARCHIPELAGO_API FApPlayer
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, SaveGame)
	int Team;
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
	FString Trait;
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
	int ItemValue;
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
struct ARCHIPELAGO_API FApGiftBoxMetaData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool AcceptAllTraits;
	UPROPERTY()
	TArray<FString> AcceptedTraits;

	//Override the comparison operator
	bool operator==(const FApGiftBoxMetaData& Other) const
	{
		if (AcceptAllTraits == Other.AcceptAllTraits	&& AcceptedTraits.Num() == Other.AcceptedTraits.Num()) {
			for (int i = 0; i < AcceptedTraits.Num(); i++) {
				if (!AcceptedTraits[i].Equals(Other.AcceptedTraits[i])) {
					return false;
				}
			}

			return true;
		}

		return false;
	}
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

USTRUCT()
struct ARCHIPELAGO_API FApReplicateableGiftBoxMetaData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FApPlayer Player;

	UPROPERTY()
	FApGiftBoxMetaData Box;
};