#pragma once

#include "CoreMinimal.h"

#include "Module/MenuWorldModule.h"


DECLARE_LOG_CATEGORY_EXTERN(LogApMainMenuModule, Log, All);

#include "ApMainMenuModule.generated.h"

/**
 * Blueprint implemented Mod Module
*/
UCLASS(Abstract, Blueprintable)
class ARCHIPELAGO_API UApMainMenuModule : public UMenuWorldModule
{
	GENERATED_BODY()

public:
	UApMainMenuModule();
};
