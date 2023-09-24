#pragma once

#include "CoreMinimal.h"

#include "Engine/UserDefinedStruct.h"

#include "ApTypes.generated.h"

USTRUCT(BlueprintType)
struct ARCHIPELAGO_API FApPlayer
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int Team;
	UPROPERTY()
	FString Name;

	//Override the comparison operator
	bool operator==(const FApPlayer& Other) const
	{
		return Team == Other.Team && Name.Equals(Other.Name);
	}
};

FORCEINLINE uint32 GetTypeHash(const FApPlayer& This)
{
	return HashCombine(GetTypeHash(This.Team), GetTypeHash(This.Name));
};

USTRUCT()
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
