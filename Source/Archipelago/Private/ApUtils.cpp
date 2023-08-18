#include "ApUtils.h"

#include <string>
#include "Reflection/ClassGenerator.h"

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

TArray<FAssetData> UApUtils::GetBlueprintAssetsIn(FName&& packagePath) {
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& registery = AssetRegistryModule.Get();

	TArray<FAssetData> assets;
	FARFilter filter;
	filter.ClassPaths.Add(FTopLevelAssetPath("/Script/Engine", "BlueprintGeneratedClass"));
	filter.PackagePaths.Add(packagePath);
	filter.bRecursivePaths = true;
	registery.GetAssets(filter, assets);
	return assets;
}

UObject* UApUtils::FindAssetByName(TArray<FAssetData> assets, FString assetName) {
	for (const auto& asset : assets) {
		if (asset.AssetName == FName(*assetName)) {
			return Cast<UBlueprintGeneratedClass>(asset.GetAsset())->GetDefaultObject();
		}
	}
	return nullptr;
}
