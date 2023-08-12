#pragma once

#include "CoreMinimal.h"

#include "ApBlueprintDataBridge.h"
#include "Module/GameInstanceModule.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApGameInstanceModule, Log, All);

#include "ApGameInstanceModule.generated.h"

UCLASS(Abstract, Blueprintable)
class ARCHIPELAGO_API UApGameInstanceModule : public UGameInstanceModule
{
	GENERATED_BODY()

public:
	UApGameInstanceModule();

	// Data Asset populated in Blueprint that contains references to assets so Cpp can refer to them
	UPROPERTY(EditDefaultsOnly, Category = "Archipelago")
	UApBlueprintDataBridge* BlueprintData;
};
