#include "ApMappingsSubsystem.h"
#include "Subsystem/ApSubsystem.h"
#include "Data/ApMappings.h"
#include "Data/ApGiftingMappings.h"
#include "Registry/ModContentRegistry.h"
#include "FGGameState.h"
#include "ApUtils.h"

DEFINE_LOG_CATEGORY(LogApMappingsSubsystem);

//TODO REMOVE
#pragma optimize("", off)

AApMappingsSubsystem* AApMappingsSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApMappingsSubsystem>();
}

AApMappingsSubsystem::AApMappingsSubsystem() : Super() {
	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::AApMappingsSubsystem()"));

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnLocal;

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AApMappingsSubsystem::DispatchLifecycleEvent(ELifecyclePhase phase) {
	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem(%p)::DispatchLifecycleEvent()"), this);

	if (phase == ELifecyclePhase::CONSTRUCTION) {
		ap = AApSubsystem::Get(GetWorld());

		LoadMappings();
	}
	else if (phase == ELifecyclePhase::POST_INITIALIZATION) {

	}
}

void AApMappingsSubsystem::BeginPlay() {
	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem(::BeginPlay()"));

	AGameStateBase* gameState = GetWorld()->GetGameState();

	if (gameState->HasAuthority()) {
		LoadTraitMappings();
	} else {
		if (AFGGameState* factoryGameState = Cast<AFGGameState>(gameState)) {
			factoryGameState->mOnClientSubsystemsValid.AddDynamic(this, &AApMappingsSubsystem::OnClientSubsystemsValid);

			if (factoryGameState->AreClientSubsystemsValid()) {
				LoadTraitMappings();
			}
		}
	}
}

void AApMappingsSubsystem::OnClientSubsystemsValid() {
	LoadTraitMappings();
}

void AApMappingsSubsystem::LoadMappings() {
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& registery = AssetRegistryModule.Get();

	const TMap<FName, const FAssetData> itemDescriptorAssets = GetItemDescriptorAssets(registery);
	const TMap<FName, const FAssetData> recipeAssets = GetRecipeAssets(registery);

	LoadItemMappings(itemDescriptorAssets);
	LoadRecipeMappings(recipeAssets);
	LoadBuildingMappings(recipeAssets);
	LoadSpecialItemMappings();
	LoadSchematicMappings();
}

void AApMappingsSubsystem::LoadItemMappings(TMap<FName, const FAssetData> itemDescriptorAssets) {
	for (TPair<int64, FString> itemMapping : UApMappings::ItemIdToGameItemDescriptor) {
		UFGItemDescriptor* itemDescriptor = GetItemDescriptorByName(itemDescriptorAssets, itemMapping.Value);
		TSubclassOf<UFGItemDescriptor> itemClass = itemDescriptor->GetClass();

		FApItem itemInfo;
		itemInfo.Id = itemMapping.Key;
		itemInfo.Descriptor = itemDescriptor;
		itemInfo.Class = itemClass; 

		ApItems.Add(itemMapping.Key, MakeShared<FApItem>(itemInfo));

		ItemClassToItemId.Add(itemClass, itemMapping.Key);
	}
}

void AApMappingsSubsystem::LoadSpecialItemMappings() {
	for (TPair<int64, EApMappingsSpecialItemType> specialItemMapping : UApMappings::ItemIdToSpecialItemType) {

		FApSpecialItem specialItem;
		specialItem.Id = specialItemMapping.Key;

		switch (specialItemMapping.Value)
		{
			case EApMappingsSpecialItemType::Inventory3:
				specialItem.SpecialType = ESpecialItemType::Inventory3;
				break;
			case EApMappingsSpecialItemType::Inventory6:
				specialItem.SpecialType = ESpecialItemType::Inventory6;
				break;
			case EApMappingsSpecialItemType::Toolbelt1:
				specialItem.SpecialType = ESpecialItemType::Toolbelt1;
				break;
		}

		ApItems.Add(specialItemMapping.Key, MakeShared<FApSpecialItem>(specialItem));
	}
}

void AApMappingsSubsystem::LoadRecipeMappings(TMap<FName, const FAssetData> recipeAssets) {
	for (TPair<int64, TArray<FString>> recipeMapping : UApMappings::ItemIdToGameRecipe) {
		TArray<FApRecipeInfo> recipes;

		for (FString recipeName : recipeMapping.Value) {
			UFGRecipe* recipe = GetRecipeByName(recipeAssets, recipeName);
			TSubclassOf<UFGRecipe> recipeClass = recipe->GetClass();

			FApRecipeInfo recipeInfo;
			recipeInfo.Recipe = recipe;
			recipeInfo.Class = recipeClass;
			recipes.Add(recipeInfo);
		}

		FApRecipeItem recipeItem; 
		recipeItem.Id = recipeMapping.Key;
		recipeItem.Recipes = recipes;

		ApItems.Add(recipeMapping.Key, MakeShared<FApRecipeItem>(recipeItem));
	}
}

void AApMappingsSubsystem::LoadBuildingMappings(TMap<FName, const FAssetData> recipeAssets) {
	for (TPair<int64, TArray<FString>> buildingMapping : UApMappings::ItemIdToGameBuilding) {
		TArray<FApRecipeInfo> recipes;

		for (FString recipeShortName : buildingMapping.Value) {
			UFGRecipe* recipe = GetRecipeByName(recipeAssets, recipeShortName);
			TSubclassOf<UFGRecipe> recipeClass = recipe->GetClass();

			FApRecipeInfo recipeInfo;
			recipeInfo.Recipe = recipe;
			recipeInfo.Class = recipeClass;
			recipes.Add(recipeInfo);

			if (recipeShortName.EndsWith("Recipe_Mam")) {
				mamId = buildingMapping.Key;
			}
			if (recipeShortName.EndsWith("Recipe_ResourceSinkShop")) {
				shopId = buildingMapping.Key;
			}
		}

		FApBuildingItem buildingItem;
		buildingItem.Id = buildingMapping.Key;
		buildingItem.Recipes = recipes;

		ApItems.Add(buildingMapping.Key, MakeShared<FApBuildingItem>(buildingItem));
	}
}

void AApMappingsSubsystem::LoadSchematicMappings() {
	for (TPair<int64, FString> schmaticMapping : UApMappings::ItemIdToGameSchematic) {
		UFGSchematic* schematic = GetSchematicByName(schmaticMapping.Value);
		TSubclassOf<UFGSchematic> schematicClass = schematic->GetClass();

		FApSchematicItem schematicInfo;
		schematicInfo.Id = schmaticMapping.Key;
		schematicInfo.Schematic = schematic;
		schematicInfo.Class = schematicClass;

		ApItems.Add(schmaticMapping.Key, MakeShared<FApSchematicItem>(schematicInfo));
	}
}

const TMap<FName, const FAssetData> AApMappingsSubsystem::GetBlueprintAssetsIn(IAssetRegistry& registery, FName&& packagePath, TArray<FString> namePrefixes) {
	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::GetBlueprintAssetsIn(packagePath: %s, namePrefixes: [%i])"), *packagePath.ToString(), namePrefixes.Num());

	FARFilter filter;
	filter.ClassPaths.Add(FTopLevelAssetPath("/Script/Engine", "BlueprintGeneratedClass"));
	filter.PackagePaths.Add(packagePath);
	filter.bRecursivePaths = true;

	TArray<FAssetData> assets;
	registery.GetAssets(filter, assets);

	TMap<FName, const FAssetData> assetsMap;
	for (const FAssetData asset : assets) {
		FString nameString;
		asset.AssetName.ToString(nameString);

		for (FString prefix : namePrefixes) {
			if (nameString.StartsWith(prefix)) {
				assetsMap.Add(asset.AssetName, asset);
				UE_LOG(LogApMappingsSubsystem, VeryVerbose, TEXT("AApMappingsSubsystem::GetBlueprintAssetsIn() Adding asset %s that matches prefix %s"), *asset.AssetName.ToString(), *prefix);
			}
		}
	}

	return assetsMap;
}

void AApMappingsSubsystem::LoadTraitMappings() {
	AFGResourceSinkSubsystem* resourceSinkSubsystem = AFGResourceSinkSubsystem::Get(GetWorld());
	fgcheck(resourceSinkSubsystem)

	TMap<FString, float> defaultSinkPointsPerTrait;

	for (TPair<FString, int64> traitDefault : UApGiftingMappings::TraitDefaultItemIds) {
		fgcheck(ApItems.Contains(traitDefault.Value) && ApItems[traitDefault.Value]->Type == EItemType::Item);

		TSharedRef<FApItem> itemInfo = StaticCastSharedRef<FApItem>(ApItems[traitDefault.Value]);

		int defaultItemSinkPoints = GetResourceSinkPointsForItem(resourceSinkSubsystem, itemInfo->Class, traitDefault.Value);

		defaultSinkPointsPerTrait.Add(traitDefault.Key, defaultItemSinkPoints);
	}

	for (TPair<int64, TSharedRef<FApItemBase>>& itemInfoMapping : ApItems) {
		if (!UApGiftingMappings::TraitsPerItemRatings.Contains(itemInfoMapping.Key))
			continue;

		fgcheck(itemInfoMapping.Value->Type == EItemType::Item)

		TSharedRef<FApItem> itemInfo = StaticCastSharedRef<FApItem>(itemInfoMapping.Value);
		TSubclassOf<UFGItemDescriptor> itemClass = itemInfo->Class;
		int64 itemId = itemInfoMapping.Key;

		int itemValue = GetResourceSinkPointsForItem(resourceSinkSubsystem, itemClass, itemId);

		TMap<FString, float> calucatedTraitsForItem;

		for (TPair<FString, float> traitRelativeRating : UApGiftingMappings::TraitsPerItemRatings[itemInfoMapping.Key]) {
			FString traitName = traitRelativeRating.Key;

			fgcheck(defaultSinkPointsPerTrait.Contains(traitName));
			float traitValue = GetTraitValue(itemValue, defaultSinkPointsPerTrait[traitName], traitRelativeRating.Value);
			calucatedTraitsForItem.Add(traitName, traitValue);

			while (UApGiftingMappings::TraitParents.Contains(traitName)) {
				traitName = UApGiftingMappings::TraitParents[traitName];

				if (!calucatedTraitsForItem.Contains(traitName)) {
					fgcheck(defaultSinkPointsPerTrait.Contains(traitName));
					traitValue = GetTraitValue(itemValue, defaultSinkPointsPerTrait[traitName], traitRelativeRating.Value);
					calucatedTraitsForItem.Add(traitName, traitValue);
				}
			}
		}

		TraitsPerItem.Add(itemInfo->Class, calucatedTraitsForItem);
	}

	//PrintTraitValuesPerItem();

	hasLoadedItemTraits = true;
}

int AApMappingsSubsystem::GetResourceSinkPointsForItem(AFGResourceSinkSubsystem* resourceSinkSubsystem, TSubclassOf<UFGItemDescriptor> itemClass, int64 itemId) {
	if (UApGiftingMappings::HardcodedSinkValues.Contains(itemId))
		return UApGiftingMappings::HardcodedSinkValues[itemId];

	int value = resourceSinkSubsystem->GetResourceSinkPointsForItem(itemClass);

	if (value == 0) {
		FString itemName = UFGItemDescriptor::GetItemName(itemClass).ToString();
		UE_LOG(LogApMappingsSubsystem, Error, TEXT("AApMappingsSubsystem::GetResourceSinkPointsForItem(\"%s\", %i) Not sink value for item"), *itemName, itemId);
		value = 1;
	}

	return value;
}

float AApMappingsSubsystem::GetTraitValue(int itemValue, float avarageItemValueForTrait, float itemSpecificTraitMultiplier) {
	return (FPlatformMath::LogX(10, (double)itemValue + 0.1) / FPlatformMath::LogX(10, (double)avarageItemValueForTrait + 0.1)) * itemSpecificTraitMultiplier;
}

void AApMappingsSubsystem::PrintTraitValuesPerItem() {
	TMap<FString, TSortedMap<float, FString>> valuesPerItem;

	for (TPair<TSubclassOf<UFGItemDescriptor>, TMap<FString, float>> traitsPerItem : TraitsPerItem) {
		for (TPair<FString, float> trait : traitsPerItem.Value) {
			if (!valuesPerItem.Contains(trait.Key))
				valuesPerItem.Add(trait.Key, TSortedMap<float, FString>());

			FString itemName = ItemIdToName[ItemClassToItemId[traitsPerItem.Key]];

			valuesPerItem[trait.Key].Add(trait.Value, itemName);
		}
	}

	TArray<FString> lines;
	for (TPair<FString, TSortedMap<float, FString>> traitsPerItem : valuesPerItem) {
		lines.Add(FString::Printf(TEXT("Trait: \"%s\":"), *traitsPerItem.Key));

		for (TPair<float, FString> valuePerItem : traitsPerItem.Value)
			lines.Add(FString::Printf(TEXT("  - Item: \"%s\": %.2f"), *valuePerItem.Value, valuePerItem.Key));
	}

	FString fileText = FString::Join(lines, TEXT("\n"));

	UApUtils::WriteStringToFile(fileText, TEXT("T:\\ItemTraits.txt"), false);
}

UObject* AApMappingsSubsystem::FindAssetByName(const TMap<FName, const FAssetData> assets, FString assetName) {
	assetName.RemoveFromEnd("'");

	if (!assetName.EndsWith("_C"))
		assetName = assetName.Append("_C");

	if (assetName.Contains("/")) {
		//working examples
		//auto s = LoadObject<UBlueprintGeneratedClass>(NULL, TEXT("/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_Ladders.ResourceSink_Ladders_C"));
		//auto a = registery.GetAssetByObjectPath(TEXT("/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_Ladders.ResourceSink_Ladders_C"));
		//auto c = LoadClass<UObject>(nullptr, TEXT("/Game/FactoryGame/Resource/RawResources/OreIron/Desc_OreIron.Desc_OreIron_C"));
		//TSubclassOf<UFGSchematic> d = LoadClass<UFGSchematic>(nullptr, TEXT("/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_Ladders.ResourceSink_Ladders_C"));
		assetName.RemoveFromStart("/Script/Engine.Blueprint'");

		UBlueprintGeneratedClass* blueprint = LoadObject<UBlueprintGeneratedClass>(NULL, *assetName);
		fgcheck(blueprint != nullptr);
		return blueprint->GetDefaultObject();
	} else {
		FName key = FName(*assetName);
		fgcheck(assets.Contains(key));
		return Cast<UBlueprintGeneratedClass>(assets[key].GetAsset())->GetDefaultObject();
	}
}

UFGSchematic* AApMappingsSubsystem::GetSchematicByName(FString name) {
	UObject* obj = FindAssetByName(TMap<FName, const FAssetData>(), name);
	fgcheck(obj != nullptr);
	return Cast<UFGSchematic>(obj);
}

UFGRecipe* AApMappingsSubsystem::GetRecipeByName(const TMap<FName, const FAssetData> recipeAssets, FString name) {
	UObject* obj = FindAssetByName(recipeAssets, name);
	fgcheck(obj != nullptr);
	return Cast<UFGRecipe>(obj);
}

UFGItemDescriptor* AApMappingsSubsystem::GetItemDescriptorByName(const TMap<FName, const FAssetData> itemDescriptorAssets, FString name) {
	UObject* obj = FindAssetByName(itemDescriptorAssets, name);
	fgcheck(obj != nullptr);
	return Cast<UFGItemDescriptor>(obj);
}

const TMap<FName, const FAssetData> AApMappingsSubsystem::GetItemDescriptorAssets(IAssetRegistry& registery) {
	TMap<FName, const FAssetData> itemDescriptorAssets;

	itemDescriptorAssets.Append(GetBlueprintAssetsIn(registery, "/Game/FactoryGame/Resource", TArray<FString>{ "Desc_", "BP_" }));
	itemDescriptorAssets.Append(GetBlueprintAssetsIn(registery, "/Game/FactoryGame/Equipment", TArray<FString>{ "Desc_", "BP_" }));
	itemDescriptorAssets.Append(GetBlueprintAssetsIn(registery, "/Game/FactoryGame/Prototype", TArray<FString>{ "Desc_" })); // Desc_WAT1 and Desc_WAT2 (alien artifacts)

	return itemDescriptorAssets;
}

const TMap<FName, const FAssetData> AApMappingsSubsystem::GetRecipeAssets(IAssetRegistry& registery) {
	TMap<FName, const FAssetData> recipeAssets;

	recipeAssets.Append(GetBlueprintAssetsIn(registery, "/Game/FactoryGame/Recipes", TArray<FString>{ "Recipe_" }));
	recipeAssets.Append(GetBlueprintAssetsIn(registery, "/Game/FactoryGame/Equipment", TArray<FString>{ "Recipe_" }));
	recipeAssets.Append(GetBlueprintAssetsIn(registery, "/Game/FactoryGame/Buildable/Factory", TArray<FString>{ "Recipe_" }));

	return recipeAssets;
}

void AApMappingsSubsystem::InitializeAfterConnectingToAp() {
	LoadNamesFromAP();
}

void AApMappingsSubsystem::LoadNamesFromAP() {
	for (TPair<int64, FString> itemMapping: UApMappings::ItemIdToGameItemDescriptor) {
		FString name = ((AApSubsystem*)ap)->GetApItemName(itemMapping.Key);

		NameToItemId.Add(name, itemMapping.Key);
		ItemIdToName.Add(itemMapping.Key, name);
	}

	hasLoadedItemNameMappings = true;
}

void AApMappingsSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) {
	if (ItemIdToName.IsEmpty())
		return;

	for (TPair<int64, FString> itemNameMapping : ItemIdToName)
		NameToItemId.Add(itemNameMapping.Value, itemNameMapping.Key);

	hasLoadedItemNameMappings = true;
}

#pragma optimize("", on)