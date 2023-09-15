#pragma once

#include "CoreMinimal.h"

#include "Subsystem/ModSubsystem.h"
#include "Resources/FGItemDescriptor.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "ApMappingsSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApMappingsSubsystem, Log, All);


USTRUCT()
struct ARCHIPELAGO_API FApItemInfo
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	UFGItemDescriptor* Descriptor;

	UPROPERTY()
	TSubclassOf<UFGItemDescriptor> Class;
};

USTRUCT()
struct ARCHIPELAGO_API FApRecipeInfo
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	UFGRecipe* Recipe;

	UPROPERTY()
	TSubclassOf<UFGRecipe> Class;
};

UCLASS()
class AApMappingsSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApMappingsSubsystem();

	virtual void BeginPlay() override;

	virtual void Tick(float dt) override;

	static AApMappingsSubsystem* Get();
	static AApMappingsSubsystem* Get(class UWorld* world);

public:
	TMap<FString, int64_t> NameToItemId;
	TMap<int64_t, FApItemInfo> ItemInfo;
	TMap<int64_t, FApRecipeInfo> RecipeInfo;

private:
	AModSubsystem* ap;

	bool isInitialized;

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool IsInitialized() const { return isInitialized; };

	void Initialize();

private:
	void LoadItemMapping();

	static UFGItemDescriptor* GetItemDescriptorByName(TMap<FName, FAssetData> itemDescriptorAssets, FString name);
	static TMap<FName, FAssetData> GetItemDescriptorAssets();
	static TMap<FName, FAssetData> GetRecipeAssets();
	static TMap<FName, FAssetData> GetBlueprintAssetsIn(FName&& packagePath, TArray<FString> namePrefixes);
	static UObject* FindAssetByName(TMap<FName, FAssetData> assets, FString assetName);
	static UFGRecipe* GetRecipeByName(TMap<FName, FAssetData> recipeAssets, FString name);
};
