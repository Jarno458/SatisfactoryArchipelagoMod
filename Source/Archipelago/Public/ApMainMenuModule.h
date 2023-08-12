#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApMainMenuModule, Log, All);

#include "ApMainMenuModule.generated.h"

UCLASS(Blueprintable)
class ARCHIPELAGO_API UApMainMenuModule : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UApMainMenuModule();
};
