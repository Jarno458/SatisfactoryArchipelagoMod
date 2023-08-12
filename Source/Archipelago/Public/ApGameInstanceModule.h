#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApGameInstanceModule, Log, All);

#include "ApGameInstanceModule.generated.h"

UCLASS(Blueprintable)
class ARCHIPELAGO_API UApGameInstanceModule : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UApGameInstanceModule();
};
