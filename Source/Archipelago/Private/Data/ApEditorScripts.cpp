

#include "Data/ApEditorScripts.h"

#if WITH_EDITOR 
#include "Kismet2/KismetEditorUtilities.h" 
#include "Kismet2/BlueprintEditorUtils.h" 
#endif

#include "FGSchematic.h"
#include "Unlocks/FGUnlockRecipe.h"


#include "ApUtils.h"

DEFINE_LOG_CATEGORY(LogApEditorScripts);

#if WITH_EDITOR 
void UApEditorScripts::GenerateApItemSchematicBlueprints() {
	UE_LOGFMT(LogApEditorScripts, Log, "UApEditorScripts::GenerateApItemSchematicBlueprints()");

	TMap<int64, TSharedRef<FApItemBase>> itemMap;
	AApMappingsSubsystem::LoadMappings(itemMap);

	for (TPair<int64, TSharedRef<FApItemBase>>& itemInfoMapping : itemMap) {
		switch (itemInfoMapping.Value->Type)
		{
			case EItemType::Item:
				break;

			case EItemType::Recipe:
			case EItemType::Building:
				CreateApItemSchematicBlueprintsForRecipe(itemInfoMapping.Key, StaticCastSharedRef<FApRecipeItem>(itemInfoMapping.Value));
				break;

			case EItemType::Schematic:
				break;

			case EItemType::Special:
				break;
		}
	}

	UE_LOGFMT(LogApEditorScripts, Log, "UApEditorScripts::GenerateApItemSchematicBlueprints() Done");
}

void UApEditorScripts::CreateApItemSchematicBlueprintsForRecipe(int64 itemId, TSharedRef<FApRecipeItem> recipeItem) {
	FName bpName(TEXT("AP_") + UApUtils::FStr(itemId));
	FString packagePath(TEXT("/Archipelago/Schematics/AP_ItemSchematics/") + bpName.ToString());

	UPackage* Package = CreatePackage(*packagePath);
	UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(UFGSchematic::StaticClass(), Package, bpName, BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());

	FString PathName = BP->GetPathName();

	UE_LOGFMT(LogApEditorScripts, Log, "Created new BP at path {0}", PathName);

	if (!PathName.EndsWith("_C")) {
		PathName.Append("_C");
	}

	TSubclassOf<UFGSchematic> InnerBPClass = LoadClass<UFGSchematic>(NULL, *PathName);
	UFGSchematic* schematic = Cast<UFGSchematic>(InnerBPClass->GetDefaultObject());

	UE_LOGFMT(LogTemp, Log, "Schematic", schematic->mDisplayName.ToString());

	FString typePrefix = recipeItem->Type == EItemType::Building ? "Building: " : "Recipe: ";
	FString recipeName = recipeItem->Recipes[0].Recipe->GetDisplayName().ToString();

	schematic->mDisplayName = FText::FromString(typePrefix + recipeName);
	schematic->mType = ESchematicType::EST_Custom;
	schematic->mMenuPriority = itemId;
	schematic->mTechTier = 0;
	schematic->mTimeToComplete = 0;

	UFGUnlockRecipe recipeUnlock;

	for (FApRecipeInfo& recipeInfo : recipeItem->Recipes) {
		recipeUnlock.AddRecipe(recipeInfo.Class);
	}

	schematic->mUnlocks.Add(&recipeUnlock);

	BP->MarkPackageDirty();
}
#endif