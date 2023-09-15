#include "ApMappingsSubsystem.h"
#include "Subsystem/ApSubsystem.h"
#include "Data/ApMappings.h"

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
	//PrimaryActorTick.TickInterval = 0.2f;
}

void AApMappingsSubsystem::Initialize() {
	if (!isInitialized) {
		ap = AApSubsystem::Get(GetWorld());

		//Needs connections to AP to load itemNames (altho we could offload that to loading later or lazy loading)
		if (((AApSubsystem*)ap)->ConnectionState == EApConnectionState::Connected) {
			LoadItemMapping();

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

void AApMappingsSubsystem::LoadItemMapping() {
	TMap<FName, FAssetData> itemDescriptorAssets = GetItemDescriptorAssets();

	for (TPair<int64_t, FString> itemMapping : UApMappings::ItemIdToGameItemDescriptor) {
		UFGItemDescriptor* itemDescriptor = GetItemDescriptorByName(itemDescriptorAssets, itemMapping.Value);
		TSubclassOf<UFGItemDescriptor> itemClass = itemDescriptor->GetClass();
		FString itemName = ((AApSubsystem*)ap)->GetItemName(itemMapping.Key);

		FApItemInfo itemInfo;
		itemInfo.Name = itemName;
		itemInfo.Descriptor = itemDescriptor;
		itemInfo.Class = itemClass;

		ItemInfo.Add(itemMapping.Key, itemInfo);
		NameToItemId.Add(itemName, itemMapping.Key);
	}

	TMap<int64_t, FString> allRecipies;
	allRecipies.Append(UApMappings::ItemIdToGameRecipe);
	allRecipies.Append(UApMappings::ItemIdToGameBuilding);

	TMap<FName, FAssetData> recipeAssets = GetRecipeAssets();
	for (TPair<int64_t, FString> recipeMapping : allRecipies) {
		UFGRecipe* recipe = GetRecipeByName(recipeAssets, recipeMapping.Value);
		TSubclassOf<UFGRecipe> recipeClass = recipe->GetClass();
		FString recipeName = ((AApSubsystem*)ap)->GetItemName(recipeMapping.Key);

		FApRecipeInfo recipeInfo;
		recipeInfo.Name = recipeName;
		recipeInfo.Recipe = recipe;
		recipeInfo.Class = recipeClass;

		RecipeInfo.Add(recipeMapping.Key, recipeInfo);
		NameToItemId.Add(recipeName, recipeMapping.Key);
	}
}


TMap<FName, FAssetData> AApMappingsSubsystem::GetBlueprintAssetsIn(FName&& packagePath, TArray<FString> namePrefixes) {
	UE_LOG(LogApMappingsSubsystem, Display, TEXT("AApMappingsSubsystem::GetBlueprintAssetsIn(packagePath: %s, namePrefixes: [%i])"), *packagePath.ToString(), namePrefixes.Num());
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& registery = AssetRegistryModule.Get();

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
	FName key = FName(*assetName);
	verify(assets.Contains(key));
	return Cast<UBlueprintGeneratedClass>(assets[key].GetAsset())->GetDefaultObject();
}

UFGRecipe* AApMappingsSubsystem::GetRecipeByName(TMap<FName, FAssetData> recipeAssets, FString name) {
	UObject* obj = FindAssetByName(recipeAssets, name.Append("_C"));
	verify(obj != nullptr);
	return Cast<UFGRecipe>(obj);
}

UFGItemDescriptor* AApMappingsSubsystem::GetItemDescriptorByName(TMap<FName, FAssetData> itemDescriptorAssets, FString name) {
	UObject* obj = FindAssetByName(itemDescriptorAssets, name);
	verify(obj != nullptr);
	return Cast<UFGItemDescriptor>(obj);
}

TMap<FName, FAssetData> AApMappingsSubsystem::GetItemDescriptorAssets() {
	TMap<FName, FAssetData> itemDescriptorAssets;

	itemDescriptorAssets.Append(GetBlueprintAssetsIn("/Game/FactoryGame/Resource", TArray<FString>{ "Desc_", "BP_" }));
	itemDescriptorAssets.Append(GetBlueprintAssetsIn("/Game/FactoryGame/Equipment", TArray<FString>{ "Desc_", "BP_" }));
	itemDescriptorAssets.Append(GetBlueprintAssetsIn("/Game/FactoryGame/Prototype", TArray<FString>{ "Desc_" })); // Desc_WAT1 and Desc_WAT2 (alien artifacts)

	return itemDescriptorAssets;
}

TMap<FName, FAssetData> AApMappingsSubsystem::GetRecipeAssets() {
	TMap<FName, FAssetData> recipeAssets;

	recipeAssets.Append(GetBlueprintAssetsIn("/Game/FactoryGame/Recipes", TArray<FString>{ "Recipe_" }));
	recipeAssets.Append(GetBlueprintAssetsIn("/Game/FactoryGame/Equipment", TArray<FString>{ "Recipe_" }));

	return recipeAssets;
}

#pragma optimize("", on)