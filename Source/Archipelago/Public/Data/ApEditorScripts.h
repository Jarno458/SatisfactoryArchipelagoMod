#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#if WITH_EDITOR 
#include "Engine/UserDefinedStruct.h"
#include "Logging/StructuredLog.h"
#include "FGSchematic.h"
#include "FGRecipe.h"

#include "ApMappings.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "Module/ApGameWorldModule.h"
#endif

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
	UFUNCTION(BlueprintCallable)
	static void GenerateApHubSchematicBlueprints();

private:
	static TSubclassOf<UFGSchematic> CreateApItemSchematicBlueprintsForRecipe(int64 itemId, TSharedRef<FApRecipeItem> recipeItem);

	static UApGameWorldModule* GetWorldModule();

	static void RemoveEmptySchematics(UApGameWorldModule* worldModule);
	static void RemoveSchematicsContaining(UApGameWorldModule* worldModule, FString name);

	static FString GetRecipeName(UFGRecipe* recipe);
#endif
};
