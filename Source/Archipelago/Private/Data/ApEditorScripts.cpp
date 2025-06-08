#include "Data/ApEditorScripts.h"

#if WITH_EDITOR 
#include "Kismet2/KismetEditorUtilities.h" 
#include "Kismet2/BlueprintEditorUtils.h" 

#include "Unlocks/FGUnlockRecipe.h"
#include "BPFContentLib.h"
#include "Resources/FGBuildingDescriptor.h"
#include "Resources/FGVehicleDescriptor.h"
#include "FGVehicle.h"
#include "AvailabilityDependencies/FGSchematicPurchasedDependency.h"
#include "Unlocks/FGUnlockGiveItem.h"
#include "FGSchematicCategory.h"

#include "ApUtils.h"
#endif

DEFINE_LOG_CATEGORY(LogApEditorScripts);

#if WITH_EDITOR 
void UApEditorScripts::GenerateApItemSchematicBlueprints() {
	UE_LOGFMT(LogApEditorScripts, Log, "UApEditorScripts::GenerateApItemSchematicBlueprints()");

	UApGameWorldModule* worldModule = GetWorldModule();
	RemoveEmptySchematics(worldModule);
	RemoveSchematicsContaining(worldModule, "AP_ItemSchematics");

	TMap<int64, TSharedRef<FApItemBase>> recipeMap;
	AApMappingsSubsystem::LoadRecipeMappings(recipeMap);

	TMap<TSubclassOf<UFGItemDescriptor>, TSet<TSubclassOf<UFGSchematic>>> schematicsPerParts;

	TSubclassOf<UFGSchematic> schematic;
	for (const TPair<int64, TSharedRef<FApItemBase>>& itemInfoMapping : recipeMap) {
		if (itemInfoMapping.Value->Type == EItemType::Recipe || itemInfoMapping.Value->Type == EItemType::Building)
		{
			const TSharedRef<FApRecipeItem> recipeItem = StaticCastSharedRef<FApRecipeItem>(itemInfoMapping.Value);

			schematic = CreateApItemSchematicBlueprintsForRecipe(itemInfoMapping.Key, recipeItem);
			worldModule->mSchematics.Add(schematic);

			if (itemInfoMapping.Value->Type == EItemType::Recipe){
				for (const FApRecipeInfo& recipeInfo : recipeItem->Recipes) {
					for (const FItemAmount& productAmount : UFGRecipe::GetProducts(recipeInfo.Class)) {
						if (!schematicsPerParts.Contains(productAmount.ItemClass))
							schematicsPerParts.Add(productAmount.ItemClass, TSet<TSubclassOf<UFGSchematic>> { schematic });
						else
							schematicsPerParts[productAmount.ItemClass].Add(schematic);
					}
				}
			}
		}
	}

	TMap<int64, TSharedRef<FApItemBase>> itemMap;
	AApMappingsSubsystem::LoadItemMappings(itemMap);

	const TSharedRef<FApItem> couponItem = StaticCastSharedRef<FApItem>(itemMap[1338040]);
	TSubclassOf<UFGItemDescriptor> couponClass = couponItem->Class;

	for (const TPair<TSubclassOf<UFGItemDescriptor>, TSet<TSubclassOf<UFGSchematic>>>& schematicsPerPart : schematicsPerParts) {
		TSubclassOf<UFGItemDescriptor> part = schematicsPerPart.Key;

		UE_LOGFMT(LogApEditorScripts, Log, "UApEditorScripts::GenerateApItemSchematicBlueprints() Handleing shop part {0}", UFGItemDescriptor::GetItemName(part).ToString());

		if (UFGItemDescriptor::GetForm(part) != EResourceForm::RF_SOLID)
			continue;

		if (!AApMappingsSubsystem::ItemClassToItemId.Contains(part)) {
			UE_LOGFMT(LogApEditorScripts, Log, "UApEditorScripts::GenerateApItemSchematicBlueprints() part {0} isnt mapped to itemid", UFGItemDescriptor::GetItemName(part).ToString());
		}
		int64 itemId = AApMappingsSubsystem::ItemClassToItemId[part];

		UE_LOGFMT(LogApEditorScripts, Log, "UApEditorScripts::GenerateApItemSchematicBlueprints() itemid {0}", itemId);

		TSharedRef<FApItem> partItem = StaticCastSharedRef<FApItem>(itemMap[itemId]);

		if (partItem->couponCost < 0)
			continue;

		schematic = CreateApShopSchematicForPart(couponClass, partItem, schematicsPerPart.Value);
		worldModule->mSchematics.Add(schematic);
	}

	worldModule->MarkPackageDirty();

	UE_LOGFMT(LogApEditorScripts, Log, "UApEditorScripts::GenerateApItemSchematicBlueprints() Done");
}

void UApEditorScripts::GenerateApHubSchematicBlueprints() {
	UE_LOGFMT(LogApEditorScripts, Log, "UApEditorScripts::GenerateApHubchematicBlueprints() Done");

	UApGameWorldModule* worldModule = GetWorldModule();
	RemoveEmptySchematics(worldModule);
	RemoveSchematicsContaining(worldModule, "AP_HubSchematics");

	for (int tier = 1; tier <= 9; tier++) {
		for (int milestone = 1; milestone <= 5; milestone++) {
			FName bpName(TEXT("AP_HUB_") + FString::FromInt(tier) + TEXT("_") + FString::FromInt(milestone));
			FString packagePath(TEXT("/Archipelago/Schematics/AP_HubSchematics/") + bpName.ToString());

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
			fgcheck(schematic != nullptr)
			UTexture2D* apIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Archipelago/Assets/SourceArt/ArchipelagoAssetPack/ArchipelagoIcon128.ArchipelagoIcon128.ArchipelagoIcon128.ArchipelagoIcon128"));
			fgcheck(apIcon != nullptr)

			FString displayName(TEXT("Hub ") + FString::FromInt(tier) + TEXT("-") + FString::FromInt(milestone));

			schematic->mDisplayName = FText::FromString(displayName);
			schematic->mType = ESchematicType::EST_Milestone;
			schematic->mTimeToComplete = 200;
			schematic->mMenuPriority = milestone;
			schematic->mTechTier = tier;
			schematic->mSchematicIcon.SetResourceObject(apIcon);
			schematic->mSmallSchematicIcon = apIcon;

			worldModule->mSchematics.Add(InnerBPClass);

			BP->MarkPackageDirty();
		}
	}

	worldModule->MarkPackageDirty();

	UE_LOGFMT(LogApEditorScripts, Log, "UApEditorScripts::GenerateApHubchematicBlueprints() Done");
}

TSubclassOf<UFGSchematic> UApEditorScripts::CreateApItemSchematicBlueprintsForRecipe(int64 itemId, TSharedRef<FApRecipeItem> recipeItem) {
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
	fgcheck(schematic != nullptr)

	FString typePrefix = recipeItem->Type == EItemType::Building ? "Building: " : "Recipe: ";
	FString recipeName = GetRecipeName(recipeItem->Recipes[0].Recipe);

	UE_LOGFMT(LogApEditorScripts, Log, "build recipeName: {0}{1}", typePrefix, recipeName);

	schematic->mDisplayName = FText::FromString(typePrefix + recipeName);
	schematic->mType = ESchematicType::EST_Custom;
	schematic->mMenuPriority = itemId; //used by randomizer to map to AP id
	schematic->mTechTier = -1; //used by randomizer to seperate AP Item schematics from AP Locations schematics
	schematic->mTimeToComplete = 0;

	for (FApRecipeInfo& recipeInfo : recipeItem->Recipes) {
		UBPFContentLib::AddRecipeToUnlock(InnerBPClass, nullptr, recipeInfo.Class);
	}

	BP->MarkPackageDirty();

	return InnerBPClass;
}

TSubclassOf<UFGSchematic> UApEditorScripts::CreateApShopSchematicForPart(TSubclassOf<UFGItemDescriptor> couponClass, TSharedRef<FApItem> part, TSet<TSubclassOf<UFGSchematic>> referencedReschematics) {
	FText partName = UFGItemDescriptor::GetItemName(part->Class);

	FName bpName(TEXT("AP_SHOP_") + partName.ToString().Replace(TEXT(" "), TEXT("_"), ESearchCase::CaseSensitive));
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
	fgcheck(schematic != nullptr)

	//UBPFContentLib::AddGiveItemsToUnlock() cant use contentlib here due to its requirement of its subsystem
	UClass* Class = FindObject<UClass>(FindPackage(nullptr, TEXT("/Game/")), TEXT("BP_UnlockGiveItem_C"), false);
	if (!Class) {
		Class = LoadObject<UClass>(nullptr, TEXT("/Game/FactoryGame/Unlocks/BP_UnlockGiveItem.BP_UnlockGiveItem_C"));
		if (!Class) {
			UE_LOGFMT(LogApEditorScripts, Fatal, "CL: Couldn't find BP_UnlockGiveItem wanting to Add to {0}", schematic->GetName());
		}
	}
	UFGUnlockGiveItem* unlockRewardItem = NewObject<UFGUnlockGiveItem>(schematic, Class);

	TArray<FItemAmount> itemsToGive{ FItemAmount(part->Class, part->stackSize) };
	unlockRewardItem->SetmItemsToGive(itemsToGive);

	TSubclassOf<UFGSchematicCategory> category;
	TSubclassOf<UFGSchematicCategory> subCategory;

	if (part->Id >= 1338150) {
		category = LoadObject<UClass>(nullptr, TEXT("/Game/FactoryGame/Schematics/ResourceSinkShopCategories/SC_RSS_Equipment.SC_RSS_Equipment_C"));
		subCategory = LoadObject<UClass>(nullptr, TEXT("/Game/FactoryGame/Schematics/ResourceSinkShopCategories/SC_RSS_Equipment2.SC_RSS_Equipment2_C"));
	} else {
		category = LoadObject<UClass>(nullptr, TEXT("/Game/FactoryGame/Schematics/ResourceSinkShopCategories/SC_RSS_Parts.SC_RSS_Parts_C"));
		subCategory = LoadObject<UClass>(nullptr, TEXT("/Game/FactoryGame/Schematics/ResourceSinkShopCategories/SC_RSS_StandardParts.SC_RSS_StandardParts_C"));
	}

	UE_LOGFMT(LogApEditorScripts, Log, "building shop item: {0}", partName.ToString());

	schematic->mDisplayName = partName;
	schematic->mType = ESchematicType::EST_ResourceSink;
	schematic->mSchematicCategory = category;
	schematic->mSubCategories = TArray<TSubclassOf<UFGSchematicCategory>>{ subCategory };
	schematic->mMenuPriority = 0; //this should probably be set in some logical fashion
	schematic->mTechTier = 1;
	schematic->mTimeToComplete = 0;
	schematic->mHiddenUntilDependenciesMet = true;
	schematic->mSchematicIcon.SetResourceObject(UFGItemDescriptor::GetBigIcon(part->Class));
	schematic->mSmallSchematicIcon = UFGItemDescriptor::GetSmallIcon(part->Class);

	TArray<FItemAmount> ticketCost { FItemAmount(couponClass, part->couponCost) };
	schematic->mCost = ticketCost;

	schematic->mUnlocks.Add(unlockRewardItem);

	for (TSubclassOf<UFGSchematic> dependency : referencedReschematics) {
		UBPFContentLib::AddSchematicToPurchaseDep(InnerBPClass, nullptr, dependency);
	}

	UFGSchematicPurchasedDependency* purchasedDepObject = Cast<UFGSchematicPurchasedDependency>(schematic->mSchematicDependencies[0]);
	purchasedDepObject->SetmRequireAllSchematicsToBePurchased(false);

	BP->MarkPackageDirty();

	return InnerBPClass;
}

UApGameWorldModule* UApEditorScripts::GetWorldModule() {
	TSubclassOf<UApGameWorldModule> worldBpClass = LoadClass<UApGameWorldModule>(NULL, TEXT("/Archipelago/RootGameWorld_Archipelago.RootGameWorld_Archipelago_C"));
	fgcheck(worldBpClass != nullptr)
	UApGameWorldModule* worldModule = Cast<UApGameWorldModule>(worldBpClass->GetDefaultObject());
	fgcheck(worldModule != nullptr)

	return worldModule;
}

void UApEditorScripts::RemoveEmptySchematics(UApGameWorldModule* worldModule) {
	//remove `None` references left by manually force deleting the schematics
	worldModule->mSchematics.Remove(TSubclassOf<UFGSchematic>());
}

void UApEditorScripts::RemoveSchematicsContaining(UApGameWorldModule* worldModule, FString name) {
	UE_LOGFMT(LogApEditorScripts, Log, "UApEditorScripts::RemoveSchematicsContaining(, {0})", name);

	TArray<TSubclassOf<UFGSchematic>> schematicsToRemove;
	for (TSubclassOf<UFGSchematic> schematic : worldModule->mSchematics) {
		if (schematic.Get()->GetPackage()->GetName().Contains(name)) {
			schematicsToRemove.Add(schematic);
		}
	}

	UE_LOGFMT(LogApEditorScripts, Log, "UApEditorScripts::RemoveSchematicsContaining() removing {0} schematics", schematicsToRemove.Num());

	for (TSubclassOf<UFGSchematic> schematic : schematicsToRemove) {
		worldModule->mSchematics.Remove(schematic);
	}
}

FString UApEditorScripts::GetRecipeName(UFGRecipe* recipe) {
	//based on https://blueprintue.com/blueprint/-z_nyyih/
	if (recipe->mDisplayNameOverride) {
		return recipe->mDisplayName.ToString();
	}

	if (recipe->mProduct.IsEmpty() || !IsValid(recipe->mProduct[0].ItemClass)) {
		return TEXT("Product was invalid, name detection failed");
	}

	UFGItemDescriptor* itemDescriptor = recipe->mProduct[0].ItemClass.GetDefaultObject();
	if (!IsValid(itemDescriptor)) {
		return TEXT("Cast to UFGItemDescriptor* failed");
	}

	if (itemDescriptor->mUseDisplayNameAndDescription) {
		return itemDescriptor->mDisplayName.ToString();
	}
	else {
		TSubclassOf<UFGItemDescriptor> itemDescriptorClass = recipe->mProduct[0].ItemClass;

		if (itemDescriptorClass->IsChildOf<UFGBuildingDescriptor>()) {
			TSubclassOf<UFGBuildingDescriptor> buildingDescriptor = *itemDescriptorClass;
			if (!IsValid(buildingDescriptor)) {
				return TEXT("Cast to TSubclassOf<UFGBuildingDescriptor> failed");
			}
			
			UFGBuildingDescriptor* buildingDescriptorCdo = buildingDescriptor.GetDefaultObject();
			if (!IsValid(buildingDescriptorCdo)) {
				return TEXT("Retreival of buildingDescriptorCdo class failed");
			}

			TSubclassOf<AFGBuildable> buildable = buildingDescriptorCdo->mBuildableClass;
			if (!IsValid(buildable)) {
				return TEXT("Retreival of buildable class failed");
			}

			AFGBuildable* buildableCdo = buildable.GetDefaultObject();
			if (!IsValid(buildableCdo)) {
				return TEXT("Retreival of buildableCdo failed");
			}

			return buildableCdo->mDisplayName.ToString();
		}

		if (itemDescriptorClass->IsChildOf<UFGVehicleDescriptor>()) {
			TSubclassOf<UFGVehicleDescriptor> vehicleDescriptor = *itemDescriptorClass;
			if (!IsValid(vehicleDescriptor)) {
				return TEXT("Cast to TSubclassOf<UFGVehicleDescriptor> failed");
			}

			UFGVehicleDescriptor* vehicleDescriptorCdo = vehicleDescriptor.GetDefaultObject();
			if (!IsValid(vehicleDescriptorCdo)) {
				return TEXT("Retreival of vehicleDescriptorCdo class failed");
			}

			TSubclassOf<AFGVehicle> vehicle = vehicleDescriptorCdo->mVehicleClass;
			if (!IsValid(vehicle)) {
				return TEXT("Retreival of vehicle class failed");
			}

			AFGVehicle* vehicleCdo = vehicle.GetDefaultObject();
			if (!IsValid(vehicleCdo)) {
				return TEXT("Retreival of vehicleCdo failed");
			}

			return vehicleCdo->mDisplayName.ToString();
		}
	}

	return TEXT("name detection for recipe failed");
}
#endif