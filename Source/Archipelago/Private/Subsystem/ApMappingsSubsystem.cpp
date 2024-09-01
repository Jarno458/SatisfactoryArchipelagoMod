#include "ApMappingsSubsystem.h"
#include "Subsystem/ApSubsystem.h"
#include "Data/ApMappings.h"
#include "Data/ApGiftingMappings.h"
#include "Registry/ModContentRegistry.h"
#include "FGGameState.h"
#include "Logging/StructuredLog.h"
#include "ApUtils.h"

DEFINE_LOG_CATEGORY(LogApMappingsSubsystem);

//TODO REMOVE
#pragma optimize("", off)

int64 AApMappingsSubsystem::shopId = 0;
int64 AApMappingsSubsystem::mamId = 0;
TMap<TSubclassOf<UFGItemDescriptor>, int64> AApMappingsSubsystem::ItemClassToItemId;

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

		ApItems.Empty();
		LoadMappings(ApItems);
	}
	else if (phase == ELifecyclePhase::POST_INITIALIZATION) {

	}
}

void AApMappingsSubsystem::BeginPlay() {
	Super::BeginPlay();

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

void AApMappingsSubsystem::LoadMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap) {
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& registery = AssetRegistryModule.Get();

	registery.WaitForCompletion();

	TMap<FName, const FAssetData> itemDescriptorAssets = GetItemDescriptorAssets(registery);
	TMap<FName, const FAssetData> recipeAssets = GetRecipeAssets(registery);

	LoadItemMappings(itemMap, itemDescriptorAssets);
	LoadRecipeMappings(itemMap, recipeAssets);
	LoadBuildingMappings(itemMap, recipeAssets);
	LoadSpecialItemMappings(itemMap);
	LoadSchematicMappings(itemMap);
}

void AApMappingsSubsystem::LoadRecipeMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap) {
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& registery = AssetRegistryModule.Get();

	registery.WaitForCompletion();

	TMap<FName, const FAssetData> recipeAssets = GetRecipeAssets(registery);

	LoadRecipeMappings(itemMap, recipeAssets);
	LoadBuildingMappings(itemMap, recipeAssets);
}

void AApMappingsSubsystem::LoadItemMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap, TMap<FName, const FAssetData>& itemDescriptorAssets) {
	ItemClassToItemId.Empty();

	for (TPair<int64, FString> itemMapping : UApMappings::ItemIdToGameItemDescriptor) {
		UFGItemDescriptor* itemDescriptor = GetItemDescriptorByName(itemDescriptorAssets, itemMapping.Value);
		TSubclassOf<UFGItemDescriptor> itemClass = itemDescriptor->GetClass();

		FApItem itemInfo;
		itemInfo.Id = itemMapping.Key;
		itemInfo.Descriptor = itemDescriptor;
		itemInfo.Class = itemClass; 

		itemMap.Add(itemMapping.Key, MakeShared<FApItem>(itemInfo));

		ItemClassToItemId.Add(itemClass, itemMapping.Key);
	}
}

void AApMappingsSubsystem::LoadSpecialItemMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap) {
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

		itemMap.Add(specialItemMapping.Key, MakeShared<FApSpecialItem>(specialItem));
	}
}

void AApMappingsSubsystem::LoadRecipeMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap, TMap<FName, const FAssetData>& recipeAssets) {
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

		itemMap.Add(recipeMapping.Key, MakeShared<FApRecipeItem>(recipeItem));
	}
}

void AApMappingsSubsystem::LoadBuildingMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap, TMap<FName, const FAssetData>& recipeAssets) {
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

		itemMap.Add(buildingMapping.Key, MakeShared<FApBuildingItem>(buildingItem));
	}
}

void AApMappingsSubsystem::LoadSchematicMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap) {
	for (TPair<int64, FString> schmaticMapping : UApMappings::ItemIdToGameSchematic) {
		UFGSchematic* schematic = GetSchematicByName(schmaticMapping.Value);
		TSubclassOf<UFGSchematic> schematicClass = schematic->GetClass();

		FApSchematicItem schematicInfo;
		schematicInfo.Id = schmaticMapping.Key;
		schematicInfo.Schematic = schematic;
		schematicInfo.Class = schematicClass;

		itemMap.Add(schmaticMapping.Key, MakeShared<FApSchematicItem>(schematicInfo));
	}
}

const TMap<FName, const FAssetData> AApMappingsSubsystem::GetBlueprintAssetsIn(IAssetRegistry& registery, FName&& packagePath, TArray<FString> namePrefixes, bool searchSubFolders) {
	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::GetBlueprintAssetsIn(packagePath: %s, namePrefixes: [%i])"), *packagePath.ToString(), namePrefixes.Num());

	FARFilter filter;

#if WITH_EDITOR
	filter.ClassPaths.Add(FTopLevelAssetPath(UBlueprint::StaticClass()));
	filter.TagsAndValues.Add(FBlueprintTags::GeneratedClassPath, TOptional<FString>());
	filter.TagsAndValues.Add(FBlueprintTags::IsDataOnly, TOptional<FString>(TEXT("True")));
#else
	filter.ClassPaths.Add(FTopLevelAssetPath("/Script/Engine", "BlueprintGeneratedClass"));
#endif
	filter.PackagePaths.Add(packagePath);
	filter.bRecursivePaths = searchSubFolders;

	TArray<FAssetData> assets;
	registery.GetAssets(filter, assets);

	TMap<FName, const FAssetData> assetsMap;
	for (const FAssetData asset : assets) {
		FString nameString = asset.AssetName.ToString();

		for (FString prefix : namePrefixes) {
			if (nameString.StartsWith(prefix)) {
				assetsMap.Add(asset.AssetName, asset);
				UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::GetBlueprintAssetsIn() Adding asset %s that matches prefix %s"), *nameString, *prefix);
			}
		}
	}

	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::GetBlueprintAssetsIn() found %i assets"), assetsMap.Num());

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
	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::FindAssetByName(assets[%i], \"%s\")"), assets.Num(), *assetName);

	assetName.RemoveFromEnd("'");

#if !WITH_EDITOR
	if (!assetName.EndsWith("_C"))
		assetName = assetName.Append("_C");
#else
	if (assetName.EndsWith("_C"))
		assetName = assetName.Mid(0, assetName.Len() - 2);
#endif

	if (assetName.Contains("/")) {
#if !WITH_EDITOR
		UE_LOG(LogApMappingsSubsystem, Error, TEXT("AApMappingsSubsystem::FindAssetByName() cant load asset of name %s"), *assetName);

		//working examples while ingame
		//auto s = LoadObject<UBlueprintGeneratedClass>(NULL, TEXT("/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_Ladders.ResourceSink_Ladders_C"));
		//auto a = registery.GetAssetByObjectPath(TEXT("/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_Ladders.ResourceSink_Ladders_C"));
		//auto c = LoadClass<UObject>(nullptr, TEXT("/Game/FactoryGame/Resource/RawResources/OreIron/Desc_OreIron.Desc_OreIron_C"));
		//TSubclassOf<UFGSchematic> d = LoadClass<UFGSchematic>(nullptr, TEXT("/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_Ladders.ResourceSink_Ladders_C"));
		assetName.RemoveFromStart("/Script/Engine.Blueprint'");

		UBlueprintGeneratedClass* blueprint = LoadObject<UBlueprintGeneratedClass>(NULL, *assetName);
		fgcheck(blueprint != nullptr);
		return blueprint->GetDefaultObject();
#else
		int32 dotPos;
		if (!assetName.FindLastChar('.', dotPos)) {
			UE_LOG(LogApMappingsSubsystem, Fatal, TEXT("AApMappingsSubsystem::FindAssetByName() failed to find . seperator in %s"), *assetName);
		}
		assetName = assetName.Mid(dotPos + 1, assetName.Len() - dotPos);

		UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::FindAssetByName() transformed into %s"), *assetName);
#endif
	}

	FName key = FName(*assetName);
	fgcheck(assets.Contains(key));

	FAssetData assetData = assets[key];

#if WITH_EDITOR
	//Based of BlueprintAssetHelperLibrary.cpp in SML
	//Retrieve GeneratedClass tag containing a text path to generated class
	FString generatedClassExportedPath;
	if (!assetData.GetTagValue(FBlueprintTags::GeneratedClassPath, generatedClassExportedPath)) {
		UE_LOG(LogApMappingsSubsystem, Fatal, TEXT("AApMappingsSubsystem::FindAssetByName() Failed to load GeneratedClassPath for asset %s"), *assetName);
	}

	//Make sure export path represents a valid path and convert it to pure objectt path
	FString generatedClassPath;
	if (!FPackageName::ParseExportTextPath(generatedClassExportedPath, NULL, &generatedClassPath)) {
		UE_LOG(LogApMappingsSubsystem, Fatal, TEXT("AApMappingsSubsystem::FindAssetByName() Failed to parse GeneratedClassPath for path %s"), *generatedClassExportedPath);
	}

	//Load UBlueprintGeneratedClass for provided object and make sure it has been loaded
	UClass* classObject = LoadObject<UClass>(NULL, *generatedClassPath);
	if (classObject == NULL) {
		UE_LOG(LogApMappingsSubsystem, Fatal, TEXT("AApMappingsSubsystem::FindAssetByName() Failed to create UBlueprintGeneratedClass for path %s"), *generatedClassPath);
	}

	return Cast<UBlueprintGeneratedClass>(classObject)->GetDefaultObject();
#else
	return Cast<UBlueprintGeneratedClass>(assets[key].GetAsset())->GetDefaultObject();
#endif
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
	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::GetItemDescriptorByName() *obj->GetClass()->GetName() %s"), *obj->GetClass()->GetName())
	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::GetItemDescriptorByName() *obj->GetClass()->GetDefaultObjectName() %s"), *obj->GetClass()->GetDefaultObjectName().ToString())

	UFGItemDescriptor* itemDesc = Cast<UFGItemDescriptor>(obj);
	fgcheck(itemDesc != nullptr);
	return itemDesc;
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

	recipeAssets.Append(GetBlueprintAssetsIn(registery, "/Game/FactoryGame", TArray<FString>{ "Recipe_" }, false));
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