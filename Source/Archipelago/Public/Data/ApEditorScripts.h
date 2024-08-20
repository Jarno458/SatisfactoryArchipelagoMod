#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/UserDefinedStruct.h"
#include "Logging/StructuredLog.h"

#include "ApMappings.h"
#include "Subsystem/ApMappingsSubsystem.h"

#include "ApEditorScripts.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApEditorScripts, Log, All);

UCLASS()
class ARCHIPELAGO_API UApEditorScripts : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

#if WITH_EDITOR 
public:
	UFUNCTION(BlueprintCallable)
	static void GenerateApItemSchematicBlueprints();

private:
	static void CreateApItemSchematicBlueprintsForRecipe(int64 itemId, TSharedRef<FApRecipeItem> recipeItem);
#endif
};
