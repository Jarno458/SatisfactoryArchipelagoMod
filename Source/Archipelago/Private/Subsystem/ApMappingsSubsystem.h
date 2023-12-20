#pragma once

#include "CoreMinimal.h"

#include "Subsystem/ModSubsystem.h"
#include "Resources/FGItemDescriptor.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "ApMappingsSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApMappingsSubsystem, Log, All);

UENUM()
enum class EItemType : uint8
{
	Item,
	Recipe,
	Building,
	Schematic,
	Specail
};

USTRUCT()
struct ARCHIPELAGO_API FApItemBase
{
	GENERATED_BODY()

	//UPROPERTY()
	//FString Name;

	UPROPERTY()
	int64 Id;

	UPROPERTY()
	EItemType Type;
};

USTRUCT()
struct ARCHIPELAGO_API FApItem : public FApItemBase
{
	GENERATED_BODY()

	FApItem() {
		Type = EItemType::Item;
	} 

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
	UFGRecipe* Recipe;

	UPROPERTY()
	TSubclassOf<UFGRecipe> Class;
};

USTRUCT()
struct ARCHIPELAGO_API FApRecipeItem : public FApItemBase
{
	GENERATED_BODY()

	FApRecipeItem() {
		Type = EItemType::Recipe;
	}

	UPROPERTY()
	TArray<FApRecipeInfo> Recipes;
};

USTRUCT()
struct ARCHIPELAGO_API FApBuildingItem : public FApRecipeItem
{
	GENERATED_BODY()

	FApBuildingItem() {
		Type = EItemType::Building;
	}
};

USTRUCT()
struct ARCHIPELAGO_API FApSchematicItem : public FApItemBase
{
	GENERATED_BODY()

	FApSchematicItem() {
		Type = EItemType::Schematic;
	}

	UPROPERTY()
	UFGSchematic* Schematic;

	UPROPERTY()
	TSubclassOf<UFGSchematic> Class;
};


UCLASS()
class AApMappingsSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApMappingsSubsystem();

	virtual void BeginPlay() override;

	static AApMappingsSubsystem* Get();
	static AApMappingsSubsystem* Get(class UWorld* world);

public:
	TMap<TSubclassOf<UFGItemDescriptor>, int64> ItemClassToItemId;
	TMap<FString, int64> NameToItemId;
	TMap<int64, FString> ItemIdToName;
	TMap<int64, TSharedRef<FApItemBase>> ApItems;

private:
	AModSubsystem* ap;

	bool isInitialized;

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool IsInitialized() const { return isInitialized; };

	void InitializeAfterConnectingToAp();

private:
	void LoadMappings();
	void LoadNames();
	void LoadItemMappings(TMap<FName, FAssetData> itemDescriptorAssets);
	void LoadRecipeMappings(TMap<FName, FAssetData> recipeAssets);
	void LoadBuildingMappings(TMap<FName, FAssetData> recipeAssets);
	void LoadSchematicMappings();

	static TMap<FName, FAssetData> GetItemDescriptorAssets(IAssetRegistry& registery);
	static TMap<FName, FAssetData> GetRecipeAssets(IAssetRegistry& registery);
	static TMap<FName, FAssetData> GetBlueprintAssetsIn(IAssetRegistry& registery, FName&& packagePath, TArray<FString> namePrefixes);
	
	static UFGRecipe* GetRecipeByName(TMap<FName, FAssetData> recipeAssets, FString name);
	static UFGItemDescriptor* GetItemDescriptorByName(TMap<FName, FAssetData> itemDescriptorAssets, FString name);
	static UFGSchematic* GetSchematicByName(FString name);

	static UObject* FindAssetByName(TMap<FName, FAssetData> assets, FString assetName);
};
