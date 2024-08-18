

#include "Data/ApEditorScripts.h"

#if WITH_EDITOR 
#include "Kismet2/KismetEditorUtilities.h" 
#include "Kismet2/BlueprintEditorUtils.h" 
#endif

#include "FGSchematic.h"
#include "Unlocks/FGUnlockRecipe.h"
#include "Subsystem/ApMappingsSubsystem.h"

#include "ApUtils.h"

DEFINE_LOG_CATEGORY(LogApEditorScripts);

#if WITH_EDITOR 
void UApEditorScripts::GenerateApItemSchematicBlueprints() {
	UE_LOGFMT(LogApEditorScripts, Log, "UApEditorScripts::GenerateApItemSchematicBlueprints()");

	//AApMappingsSubsystem* mappingSubsystem = AApMappingsSubsystem::Get();
	//mappingSubsystem->DispatchLifecycleEvent();

	for (TPair<int64, TArray<FString>> recipeMapping : UApMappings::ItemIdToGameRecipe) {
		TArray<FApRecipeInfo> recipes;

		/*for (FString recipeName : recipeMapping.Value) {
			UFGRecipe* recipe = GetRecipeByName(recipeAssets, recipeName);
			TSubclassOf<UFGRecipe> recipeClass = recipe->GetClass();

			FApRecipeInfo recipeInfo;
			recipeInfo.Recipe = recipe;
			recipeInfo.Class = recipeClass;
			recipes.Add(recipeInfo);
		}

		CreateApItemSchematicBlueprints();*/
	}

	UE_LOGFMT(LogApEditorScripts, Log, "UApEditorScripts::GenerateApItemSchematicBlueprints() Done");
}

void UApEditorScripts::CreateApItemSchematicBlueprints(int64 itemId, FString recipeName) {
	FString itemIdString = UApUtils::FStr(itemId);

	UPackage* Package = CreatePackage(nullptr, *(TEXT("/Archipelago/Schematics/AP_ItemSchematics/") + itemIdString));
	FName bpName(TEXT("AP_") + itemIdString);

	UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(UFGSchematic::StaticClass(), Package, bpName, BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());

	FString PathName = BP->GetPathName();

	UE_LOGFMT(LogApEditorScripts, Log, "Created new BP at path {0}", PathName);

	if (!PathName.EndsWith("_C")) {
		PathName.Append("_C");
	}

	UClass* InnerBPClass = LoadObject<UClass>(NULL, *PathName);

	UObject* bpObject = InnerBPClass->GetDefaultObject();
	UFGSchematic* schematic = Cast<UFGSchematic>(bpObject);

	UE_LOGFMT(LogTemp, Log, "Schematic", schematic->mDisplayName.ToString());

	schematic->mType = ESchematicType::EST_Custom;
	schematic->mMenuPriority = itemId;
	schematic->mTechTier = 0;
	schematic->mTimeToComplete = 0;

	//TODO add recipe unlock
	//UFGUnlockRecipe recipeUnlock();
	//recipeUnlock->AddRecipe();

	//schematic->mUnlocks.Add(recipeUnlock);

	BP->MarkPackageDirty();
}
#endif