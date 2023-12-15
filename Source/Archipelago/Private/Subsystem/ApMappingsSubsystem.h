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
	InventorySlot
};

USTRUCT()
struct ARCHIPELAGO_API FApItem 
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	EItemType Type;
};

USTRUCT()
struct ARCHIPELAGO_API FApItemInfo : public FApItem
{
	GENERATED_BODY()

	FApItemInfo() { 
		Type = EItemType::Item;
	} 

	UPROPERTY()
	UFGItemDescriptor* Descriptor;

	UPROPERTY()
	TSubclassOf<UFGItemDescriptor> Class;
};

USTRUCT()
struct ARCHIPELAGO_API FApRecipeInfo : public FApItem
{
	GENERATED_BODY()

	FApRecipeInfo() {
		Type = EItemType::Recipe;
	}

	UPROPERTY()
	UFGRecipe* Recipe;

	UPROPERTY()
	TSubclassOf<UFGRecipe> Class;
};

USTRUCT()
struct ARCHIPELAGO_API FApBuildingRecipeInfo : public FApRecipeInfo
{
	GENERATED_BODY()

	FApBuildingRecipeInfo() {
		Type = EItemType::Building;
	}
};

USTRUCT()
struct ARCHIPELAGO_API FApSchematicInfo : public FApItem
{
	GENERATED_BODY()

	FApSchematicInfo() {
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

	virtual void Tick(float dt) override;

	static AApMappingsSubsystem* Get();
	static AApMappingsSubsystem* Get(class UWorld* world);

public:
	TMap<TSubclassOf<UFGItemDescriptor>, int64> ItemClassToItemId;
	TMap<FString, int64> NameToItemId;

	TMap<int64, TSharedRef<FApItem>> ApItems;

private:
	AModSubsystem* ap;

	bool isInitialized;

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool IsInitialized() const { return isInitialized; };

	void Initialize();

private:
	void LoadMappings();
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
