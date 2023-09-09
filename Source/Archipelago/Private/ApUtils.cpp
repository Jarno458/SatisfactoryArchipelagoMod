#include "ApUtils.h"

#include <string>
#include "Reflection/ClassGenerator.h"
#include "Module/GameInstanceModuleManager.h"
#include "Module/ApGameInstanceModule.h"

//TODO REMOVE
#pragma optimize("", off)

DEFINE_LOG_CATEGORY(LogApUtils);

DEFINE_LOG_CATEGORY(LogArchipelagoCpp);

FText UApUtils::FText(std::string inString) {
	return FText::FromString(FString(inString.c_str()));
}

FString UApUtils::FStr(std::string inString) {
	return FString(inString.c_str());
}

FString UApUtils::FStr(int64_t inInt) {
	return FString(std::to_string(inInt).c_str());
}

UClass* UApUtils::FindOrCreateClass(const TCHAR* packageName, const TCHAR* className, UClass* parentClass) {
	if (auto found = FindObject<UClass>(FindPackage(nullptr, packageName), className, false)) {
		UE_LOG(LogApUtils, Verbose, TEXT("Class %s already exists, returning that instead"), className);
		return found;
	} else {
		UE_LOG(LogApUtils, Verbose, TEXT("Creating class %s in package %s"), className, packageName);
		return FClassGenerator::GenerateSimpleClass(packageName, className, parentClass);
	}
}

FString UApUtils::GetImagePathForItem(UFGItemDescriptor* item) {
	return item->GetBigIconFromInstance()->GetPathName();
}

TMap<FName, FAssetData> UApUtils::GetBlueprintAssetsIn(FName&& packagePath, TArray<FString> namePrefixes) {
	UE_LOG(LogApUtils, Display, TEXT("Processing assets in path: %s"), *packagePath.ToString());
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
				UE_LOG(LogApUtils, VeryVerbose, TEXT("Adding asset %s that matches prefix %s"), *asset.AssetName.ToString(), *prefix);
			}
		}
	}

	return assetsMap;
}

UObject* UApUtils::FindAssetByName(TMap<FName, FAssetData> assets, FString assetName) {
	FName key = FName(*assetName);
	
	if (!assets.Contains(key)) {
		UE_LOG(LogApUtils, Fatal, TEXT("Key '%s' not present in assets to search"), *assetName);
	}

	return Cast<UBlueprintGeneratedClass>(assets[key].GetAsset())->GetDefaultObject();
}

UFGRecipe* UApUtils::GetRecipeByName(TMap<FName, FAssetData> recipeAssets, FString name) {
	UObject* obj = FindAssetByName(recipeAssets, name.Append("_C"));
	verify(obj != nullptr);
	return Cast<UFGRecipe>(obj);
}

UFGItemDescriptor* UApUtils::GetItemDescriptorByName(TMap<FName, FAssetData> itemDescriptorAssets, FString name) {
	UObject* obj = FindAssetByName(itemDescriptorAssets, name);
	verify(obj != nullptr);
	return Cast<UFGItemDescriptor>(obj);
}

TMap<FName, FAssetData> UApUtils::GetItemDescriptorAssets() {
	TMap<FName, FAssetData> itemDescriptorAssets;
		
	itemDescriptorAssets.Append(GetBlueprintAssetsIn("/Game/FactoryGame/Resource", TArray<FString>{ "Desc_", "BP_" }));
	itemDescriptorAssets.Append(GetBlueprintAssetsIn("/Game/FactoryGame/Equipment", TArray<FString>{ "Desc_", "BP_" }));
	itemDescriptorAssets.Append(GetBlueprintAssetsIn("/Game/FactoryGame/Prototype", TArray<FString>{ "Desc_", "BP_" })); // BP_WAT1 and BP_WAT2 (alien artifacts)

	//should prob cache this results
	return itemDescriptorAssets;
}

TMap<FName, FAssetData> UApUtils::GetRecipeAssets() {
	TMap<FName, FAssetData> recipeAssets;

	recipeAssets.Append(GetBlueprintAssetsIn("/Game/FactoryGame/Recipes", TArray<FString>{ "Recipe_" }));
	recipeAssets.Append(GetBlueprintAssetsIn("/Game/FactoryGame/Equipment", TArray<FString>{ "Recipe_" }));

	//should prob cache this results
	return recipeAssets;
}

UApBlueprintDataBridge* UApUtils::GetBlueprintDataBridge(UObject* worldContext) {
	if (const auto world = worldContext->GetWorld()) {
		const auto moduleManager = world->GetGameInstance()->GetSubsystem<UGameInstanceModuleManager>();
		const auto module = moduleManager->FindModule("Archipelago");
		const auto modModule = Cast<UApGameInstanceModule>(module);
		return modModule->BlueprintData;
	}
	return nullptr;
}

#pragma optimize("", on)