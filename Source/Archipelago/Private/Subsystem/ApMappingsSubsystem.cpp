#include "ApMappingsSubsystem.h"
#include "Subsystem/ApSubsystem.h"
#include "Data/ApMappings.h"

#include "Registry/ModContentRegistry.h"

DEFINE_LOG_CATEGORY(LogApMappingsSubsystem);

//TODO REMOVE
#pragma optimize("", off)

AApMappingsSubsystem* AApMappingsSubsystem::Get() {
	return Get(GEngine->GameViewport->GetWorld());
}

AApMappingsSubsystem* AApMappingsSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApMappingsSubsystem>();
}

AApMappingsSubsystem::AApMappingsSubsystem() : Super() {
	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::AApMappingsSubsystem()"));

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AApMappingsSubsystem::DispatchLifecycleEvent(ELifecyclePhase phase) {
	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::DispatchLifecycleEvent()"));

	if (phase == ELifecyclePhase::CONSTRUCTION) {
		ap = AApSubsystem::Get(GetWorld());

		LoadMappings();
	}
}

void AApMappingsSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::BeginPlay()"));
}

void AApMappingsSubsystem::LoadMappings() {
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& registery = AssetRegistryModule.Get();

	TMap<FName, FAssetData> itemDescriptorAssets = GetItemDescriptorAssets(registery);
	TMap<FName, FAssetData> recipeAssets = GetRecipeAssets(registery);

	LoadItemMappings(itemDescriptorAssets);
	LoadRecipeMappings(recipeAssets);
	LoadBuildingMappings(recipeAssets);
	LoadSpecialItemMappings();
	LoadSchematicMappings();
}

void AApMappingsSubsystem::LoadItemMappings(TMap<FName, FAssetData> itemDescriptorAssets) {
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
	for (TPair<int64, EApMappingsSpecailItemType> specailItemMapping : UApMappings::ItemIdToSpecailItemType) {

		FApSpecailItem specialItem;
		specialItem.Id = specailItemMapping.Key;

		switch (specailItemMapping.Value)
		{
			case EApMappingsSpecailItemType::Inventory3:
				specialItem.SpecailType = ESpecailItemType::Inventory3;
				break;
			case EApMappingsSpecailItemType::Inventory6:
				specialItem.SpecailType = ESpecailItemType::Inventory6;
				break;
			case EApMappingsSpecailItemType::Toolbelt1:
				specialItem.SpecailType = ESpecailItemType::Toolbelt1;
				break;
		}

		ApItems.Add(specailItemMapping.Key, MakeShared<FApSpecailItem>(specialItem));
	}
}

void AApMappingsSubsystem::LoadRecipeMappings(TMap<FName, FAssetData> recipeAssets) {
	for (TPair<int64, FString> recipeMapping : UApMappings::ItemIdToGameRecipe) {
		UFGRecipe* recipe = GetRecipeByName(recipeAssets, recipeMapping.Value);
		TSubclassOf<UFGRecipe> recipeClass = recipe->GetClass();

		FApRecipeInfo recipeInfo;
		recipeInfo.Recipe = recipe;
		recipeInfo.Class = recipeClass;

		FApRecipeItem recipeItem;
		recipeItem.Id = recipeMapping.Key;
		recipeItem.Recipes = TArray<FApRecipeInfo>{ recipeInfo };

		ApItems.Add(recipeMapping.Key, MakeShared<FApRecipeItem>(recipeItem));
	}
}

void AApMappingsSubsystem::LoadBuildingMappings(TMap<FName, FAssetData> recipeAssets) {
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

TMap<FName, FAssetData> AApMappingsSubsystem::GetBlueprintAssetsIn(IAssetRegistry& registery, FName&& packagePath, TArray<FString> namePrefixes) {
	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::GetBlueprintAssetsIn(packagePath: %s, namePrefixes: [%i])"), *packagePath.ToString(), namePrefixes.Num());

	FARFilter filter;
	filter.ClassPaths.Add(FTopLevelAssetPath("/Script/Engine", "BlueprintGeneratedClass"));
	filter.PackagePaths.Add(packagePath);
	filter.bRecursivePaths = true;

	TArray<FAssetData> assets;
	registery.GetAssets(filter, assets);

	TMap<FName, FAssetData> assetsMap;
	for (auto asset : assets) {
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

UObject* AApMappingsSubsystem::FindAssetByName(TMap<FName, FAssetData> assets, FString assetName) {
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
	UObject* obj = FindAssetByName(TMap<FName, FAssetData>(), name);
	fgcheck(obj != nullptr);
	return Cast<UFGSchematic>(obj);
}

UFGRecipe* AApMappingsSubsystem::GetRecipeByName(TMap<FName, FAssetData> recipeAssets, FString name) {
	UObject* obj = FindAssetByName(recipeAssets, name);
	fgcheck(obj != nullptr);
	return Cast<UFGRecipe>(obj);
}

UFGItemDescriptor* AApMappingsSubsystem::GetItemDescriptorByName(TMap<FName, FAssetData> itemDescriptorAssets, FString name) {
	UObject* obj = FindAssetByName(itemDescriptorAssets, name);
	fgcheck(obj != nullptr);
	return Cast<UFGItemDescriptor>(obj);
}

TMap<FName, FAssetData> AApMappingsSubsystem::GetItemDescriptorAssets(IAssetRegistry& registery) {
	TMap<FName, FAssetData> itemDescriptorAssets;

	itemDescriptorAssets.Append(GetBlueprintAssetsIn(registery, "/Game/FactoryGame/Resource", TArray<FString>{ "Desc_", "BP_" }));
	itemDescriptorAssets.Append(GetBlueprintAssetsIn(registery, "/Game/FactoryGame/Equipment", TArray<FString>{ "Desc_", "BP_" }));
	itemDescriptorAssets.Append(GetBlueprintAssetsIn(registery, "/Game/FactoryGame/Prototype", TArray<FString>{ "Desc_" })); // Desc_WAT1 and Desc_WAT2 (alien artifacts)

	return itemDescriptorAssets;
}

TMap<FName, FAssetData> AApMappingsSubsystem::GetRecipeAssets(IAssetRegistry& registery) {
	TMap<FName, FAssetData> recipeAssets;

	recipeAssets.Append(GetBlueprintAssetsIn(registery, "/Game/FactoryGame/Recipes", TArray<FString>{ "Recipe_" }));
	recipeAssets.Append(GetBlueprintAssetsIn(registery, "/Game/FactoryGame/Equipment", TArray<FString>{ "Recipe_" }));
	recipeAssets.Append(GetBlueprintAssetsIn(registery, "/Game/FactoryGame/Buildable/Factory", TArray<FString>{ "Recipe_" }));

	return recipeAssets;
}

void AApMappingsSubsystem::InitializeAfterConnectingToAp() {
	LoadNamesFromAP();

	isInitialized = true;
}

void AApMappingsSubsystem::LoadNamesFromAP() {
	for (TPair<int64, FString> itemMapping: UApMappings::ItemIdToGameItemDescriptor) {
		FString name = ((AApSubsystem*)ap)->GetApItemName(itemMapping.Key);

		NameToItemId.Add(name, itemMapping.Key);
		ItemIdToName.Add(itemMapping.Key, name);
	}
}

void AApMappingsSubsystem::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) {

	auto x = ItemIdToName;
}

void AApMappingsSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) {
	if (ItemIdToName.IsEmpty())
		return;

	for (TPair<int64, FString> itemNameMapping : ItemIdToName)
		NameToItemId.Add(itemNameMapping.Value, itemNameMapping.Key);

	isInitialized = true;
}

#pragma optimize("", on)