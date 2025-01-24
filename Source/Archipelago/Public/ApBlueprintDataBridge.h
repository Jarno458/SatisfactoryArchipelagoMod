#pragma once
#include "CoreMinimal.h"
#include "Resources/FGItemDescriptor.h"
#include "Buildables/FGBuildablePowerStorage.h"
#include "ApBlueprintDataBridge.generated.h"

/**
 * Data asset definition to allow passing assets from blueprint to the C++ side without having to LoadObject hard code them
 */
UCLASS(NotBlueprintable, BlueprintType)
class UApBlueprintDataBridge : public UDataAsset {
	GENERATED_BODY()
public:

	// Item to use as the Archipelago Blocker
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<UFGItemDescriptor> BlockerItem;

	// Item to use as the replacement for Somersloops
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<UFGItemDescriptor> ExplorationPickupItem_Somersloop;

	// Item to use as the replacement for Mercer Spheres
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<UFGItemDescriptor> ExplorationPickupItem_Mercer;
};
