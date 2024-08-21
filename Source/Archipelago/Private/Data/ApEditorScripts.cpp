

#include "Data/ApEditorScripts.h"

#if WITH_EDITOR 
#include "Kismet2/KismetEditorUtilities.h" 
#include "Kismet2/BlueprintEditorUtils.h" 
#endif

#include "FGSchematic.h"
#include "Unlocks/FGUnlockRecipe.h"
#include "BPFContentLib.h"


#include "ApUtils.h"

DEFINE_LOG_CATEGORY(LogApEditorScripts);

#if WITH_EDITOR 
void UApEditorScripts::GenerateApItemSchematicBlueprints() {
	UE_LOGFMT(LogApEditorScripts, Log, "UApEditorScripts::GenerateApItemSchematicBlueprints()");

	TMap<int64, TSharedRef<FApItemBase>> itemMap;
	AApMappingsSubsystem::LoadRecipeMappings(itemMap);

	for (TPair<int64, TSharedRef<FApItemBase>>& itemInfoMapping : itemMap) {
		switch (itemInfoMapping.Value->Type)
		{
			case EItemType::Recipe:
			case EItemType::Building:
				CreateApItemSchematicBlueprintsForRecipe(itemInfoMapping.Key, StaticCastSharedRef<FApRecipeItem>(itemInfoMapping.Value));
				break;

			default:
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
	fgcheck(InnerBPClass != nullptr)
	UFGSchematic* schematic = Cast<UFGSchematic>(InnerBPClass->GetDefaultObject());

	FString typePrefix = recipeItem->Type == EItemType::Building ? "Building: " : "Recipe: ";

	UE_LOGFMT(LogApEditorScripts, Log, "build typePrefix: {0}", typePrefix);

	FString recipeName = recipeItem->Recipes[0].Recipe->GetDisplayName().ToString();

	FString recipeName2 = Cast<UFGRecipe>(recipeItem->Recipes[0].Class->GetDefaultObject())->GetDisplayName().ToString();

	UE_LOGFMT(LogApEditorScripts, Log, "build recipeName: {0}", recipeName);
	UE_LOGFMT(LogApEditorScripts, Log, "build recipeName2: {0}", recipeName);

	schematic->mDisplayName = FText::FromString(typePrefix + recipeName);
	schematic->mType = ESchematicType::EST_Custom;
	schematic->mMenuPriority = itemId;
	schematic->mTechTier = 0;
	schematic->mTimeToComplete = 0;

	for (FApRecipeInfo& recipeInfo : recipeItem->Recipes) {
		UBPFContentLib::AddRecipeToUnlock(InnerBPClass, nullptr, recipeInfo.Class);
	}

	BP->MarkPackageDirty();
}
#endif