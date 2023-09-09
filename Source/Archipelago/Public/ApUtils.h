#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Resources/FGItemDescriptor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ApBlueprintDataBridge.h"

#include "ApUtils.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApUtils, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogArchipelagoCpp, Log, All);

/**
 * Archipelago Utility library
 */
UCLASS()
class ARCHIPELAGO_API UApUtils : public UObject
{
	GENERATED_BODY()

public:
	// Shorthand convert from std::string to FText
	static FText FText(std::string inString);

	// Shorthand convert from std::string to FString
	// Name is not exactly 'FString' because that causes naming problems
	static FString FStr(std::string inString);

	// Shorthand convert from int64_t to FString
	// Name is not exactly 'FString' because that causes naming problems
	static FString FStr(int64_t inInt);

	// Wrapper for FClassGenerator::GenerateSimpleClass that returns existing classes instead of crashing by creating new ones
	static UClass* FindOrCreateClass(const TCHAR* packageName, const TCHAR* className, UClass* parentClass);

	static FString GetImagePathForItem(UFGItemDescriptor* item);

	//We might want to move all Assets mamagent to its own place
	static TMap<FName, FAssetData> GetBlueprintAssetsIn(FName&& packagePath, TArray<FString> namePrefixes);
	static UObject* FindAssetByName(TMap<FName, FAssetData> assets, FString assetName);
	static UFGRecipe* GetRecipeByName(TMap<FName, FAssetData> recipeAssets, FString name);
	static UFGItemDescriptor* GetItemDescriptorByName(TMap<FName, FAssetData> itemDescriptorAssets, FString name);
	static TMap<FName, FAssetData> GetItemDescriptorAssets();
	static TMap<FName, FAssetData> GetRecipeAssets();

	UFUNCTION(BlueprintCallable)
	static UApBlueprintDataBridge* GetBlueprintDataBridge(UObject* worldContext);
};
