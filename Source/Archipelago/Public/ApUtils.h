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

	// Shorthand convert from int64 to FString
	// Name is not exactly 'FString' because that causes naming problems
	static FString FStr(int64 inInt);

	// Wrapper for FClassGenerator::GenerateSimpleClass that returns existing classes instead of crashing by creating new ones
	static UClass* FindOrCreateClass(const TCHAR* packageName, const TCHAR* className, UClass* parentClass);

	static FString GetImagePathForItem(UFGItemDescriptor* item);

	UFUNCTION(BlueprintCallable)
	static UApBlueprintDataBridge* GetBlueprintDataBridge(UObject* worldContext);

	static void WriteStringToFile(FString Path, FString text, bool relative);
};
