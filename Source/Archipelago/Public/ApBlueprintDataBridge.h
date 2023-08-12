#pragma once
#include "CoreMinimal.h"
#include "Resources/FGItemDescriptor.h"
#include "ApBlueprintDataBridge.generated.h"

UCLASS(NotBlueprintable, BlueprintType)
class UApBlueprintDataBridge : public UDataAsset {
	GENERATED_BODY()
public:

	// Item to use as the Archipelago Blocker
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<UFGItemDescriptor> BlockerItem;

	// Testing
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FText Testing123;
};
