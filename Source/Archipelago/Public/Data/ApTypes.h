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
	UPROPERTY(BlueprintReadWrite)
	int Team;
	UPROPERTY(BlueprintReadWrite)
	FString Name;

	//Override the comparison operator
	bool operator==(const FApPlayer& Other) const
	{
		return Team == Other.Team && Name.Equals(Other.Name);
	}

	// Value constructor
	FApPlayer(int team, FString name) : Name(name), Team(team)
	{}

	// Default constructor
	FApPlayer() : FApPlayer(-1, "Invalid")
	{}

	// Copy constructor
	FApPlayer(const FApPlayer& Other) : Name(Other.Name), Team(Other.Team)
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
	UPROPERTY()
	FString Trait;
	UPROPERTY()
	float Quality;
	UPROPERTY()
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
};