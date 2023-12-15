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
	check(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApMappingsSubsystem>();
}

AApMappingsSubsystem::AApMappingsSubsystem() : Super() {
	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::AApMappingsSubsystem()"));

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AApMappingsSubsystem::Initialize() {
	if (!isInitialized) {
		ap = AApSubsystem::Get(GetWorld());

		//Needs connections to AP to load itemNames (altho we could offload that to loading later or lazy loading)
		if (((AApSubsystem*)ap)->ConnectionState == EApConnectionState::Connected) {
			LoadMappings();

			isInitialized = true;

			SetActorTickEnabled(false);
		}
	}
}

void AApMappingsSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::BeginPlay()"));
}

void AApMappingsSubsystem::Tick(float dt) {
	Super::Tick(dt);
}

void AApMappingsSubsystem::LoadMappings() {
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& registery = AssetRegistryModule.Get();

	TMap<FName, FAssetData> itemDescriptorAssets = GetItemDescriptorAssets(registery);
	TMap<FName, FAssetData> recipeAssets = GetRecipeAssets(registery);

	LoadItemMappings(itemDescriptorAssets);
	LoadRecipeMappings(recipeAssets);
	LoadBuildingMappings(recipeAssets);
	LoadSchematicMappings();

	for (TPair<int64, TSharedRef<FApItem>> itemMapping : ApItems) {
		NameToItemId.Add(itemMapping.Value->Name, itemMapping.Key);
	}
}

void AApMappingsSubsystem::LoadItemMappings(TMap<FName, FAssetData> itemDescriptorAssets) {
	for (TPair<int64, FString> itemMapping : UApMappings::ItemIdToGameItemDescriptor) {
		UFGItemDescriptor* itemDescriptor = GetItemDescriptorByName(itemDescriptorAssets, itemMapping.Value);
		TSubclassOf<UFGItemDescriptor> itemClass = itemDescriptor->GetClass();
		FString itemName = ((AApSubsystem*)ap)->GetApItemName(itemMapping.Key);

		FApItemInfo itemInfo;
		itemInfo.Name = itemName;
		itemInfo.Descriptor = itemDescriptor;
		itemInfo.Class = itemClass; 

		ApItems.Add(itemMapping.Key, MakeShared<FApItemInfo>(itemInfo));

		ItemClassToItemId.Add(itemClass, itemMapping.Key);
	}
}

void AApMappingsSubsystem::LoadRecipeMappings(TMap<FName, FAssetData> recipeAssets) {
	for (TPair<int64, FString> recipeMapping : UApMappings::ItemIdToGameRecipe) {
		UFGRecipe* recipe = GetRecipeByName(recipeAssets, recipeMapping.Value);
		TSubclassOf<UFGRecipe> recipeClass = recipe->GetClass();
		FString recipeName = ((AApSubsystem*)ap)->GetApItemName(recipeMapping.Key);

		FApRecipeInfo recipeInfo;
		recipeInfo.Name = recipeName;
		recipeInfo.Recipe = recipe;
		recipeInfo.Class = recipeClass;

		ApItems.Add(recipeMapping.Key, MakeShared<FApRecipeInfo>(recipeInfo));
	}
}

void AApMappingsSubsystem::LoadBuildingMappings(TMap<FName, FAssetData> recipeAssets) {
	for (TPair<int64, TArray<FString>> buildingMapping : UApMappings::ItemIdToGameBuilding) {
		for (FString recipeShortName : buildingMapping.Value) {
			UFGRecipe* recipe = GetRecipeByName(recipeAssets, recipeShortName);
			TSubclassOf<UFGRecipe> recipeClass = recipe->GetClass();
			FString recipeName = ((AApSubsystem*)ap)->GetApItemName(buildingMapping.Key);

			FApRecipeInfo recipeInfo;
			recipeInfo.Name = recipeName;
			recipeInfo.Recipe = recipe;
			recipeInfo.Class = recipeClass;

			ApItems.Add(buildingMapping.Key, MakeShared<FApRecipeInfo>(recipeInfo));
		}
	}
}

void AApMappingsSubsystem::LoadSchematicMappings() {
	for (TPair<int64, FString> schmaticMapping : UApMappings::ItemIdToGameSchematic) {
		UFGSchematic* schematic = GetSchematicByName(schmaticMapping.Value);
		TSubclassOf<UFGSchematic> schematicClass = schematic->GetClass();
		FString schematicName = ((AApSubsystem*)ap)->GetApItemName(schmaticMapping.Key);

		FApSchematicInfo schematicInfo;
		schematicInfo.Name = schematicName;
		schematicInfo.Schematic = schematic;
		schematicInfo.Class = schematicClass;

		ApItems.Add(schmaticMapping.Key, MakeShared<FApSchematicInfo>(schematicInfo));
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

		assetName.RemoveFromStart("/Script/Engine.Blueprint'");

		UBlueprintGeneratedClass* blueprint = LoadObject<UBlueprintGeneratedClass>(NULL, *assetName);
		if (blueprint == nullptr)
		{
			auto yolo = 10;
		}
		verify(blueprint != nullptr);
		return blueprint->GetDefaultObject();
	} else {
		FName key = FName(*assetName);
		if (!assets.Contains(key))
		{
			auto yolo = 10;
		}
		verify(assets.Contains(key));

		return Cast<UBlueprintGeneratedClass>(assets[key].GetAsset())->GetDefaultObject();
	}
}

UFGSchematic* AApMappingsSubsystem::GetSchematicByName(FString name) {
	UObject* obj = FindAssetByName(TMap<FName, FAssetData>(), name);
	verify(obj != nullptr);
	return Cast<UFGSchematic>(obj);
}

UFGRecipe* AApMappingsSubsystem::GetRecipeByName(TMap<FName, FAssetData> recipeAssets, FString name) {
	UObject* obj = FindAssetByName(recipeAssets, name);
	verify(obj != nullptr);
	return Cast<UFGRecipe>(obj);
}

UFGItemDescriptor* AApMappingsSubsystem::GetItemDescriptorByName(TMap<FName, FAssetData> itemDescriptorAssets, FString name) {
	UObject* obj = FindAssetByName(itemDescriptorAssets, name);
	verify(obj != nullptr);
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

#pragma optimize("", on)