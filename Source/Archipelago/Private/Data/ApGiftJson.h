#pragma once

#include "CoreMinimal.h"

#include "Engine/UserDefinedStruct.h"

#include "ApGiftJson.generated.h"

//Based on Gifting API v2
USTRUCT()
struct ARCHIPELAGO_API FApGiftTraitJson
{
   GENERATED_BODY()

   UPROPERTY()
   FString Trait;

   UPROPERTY()
   float Quality;

   UPROPERTY()
   float Duration;
};

USTRUCT()
struct ARCHIPELAGO_API FApGiftJson
{
	GENERATED_BODY()

   UPROPERTY()
   FString ID;

   UPROPERTY()
   FString ItemName;

   UPROPERTY()
   int Amount;

   UPROPERTY()
   int ItemValue;

   UPROPERTY()
   TArray<FApGiftTraitJson> Traits;

   UPROPERTY()
   int SenderSlot;

   UPROPERTY()
   int SenderTeam;

   UPROPERTY()
   int ReceiverSlot;

   UPROPERTY()
   int ReceiverTeam;

   UPROPERTY()
   bool IsRefund;
};

USTRUCT()
struct ARCHIPELAGO_API FApGiftMotherBoxJson
{
   GENERATED_BODY()

   UPROPERTY()
   bool IsOpen;

   UPROPERTY()
   bool AcceptsAnyGift;

   UPROPERTY()
   TArray<FString> DesiredTraits;

   UPROPERTY()
   int MinimumGiftDataVersion = 2;

   UPROPERTY()
   int MaximumGiftDataVersion = 2;
};