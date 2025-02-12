#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "FGOptionInterface.h"

#include "ApBPFL.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApBPFL, Log, All);

UCLASS()
class ARCHIPELAGO_API UApBPFL : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	//UFUNCTION(BlueprintCallable)
	//static void SetSessionSetting(const FString& cvar, TScriptInterface<class IFGOptionInterface> optionsInterface);
};
