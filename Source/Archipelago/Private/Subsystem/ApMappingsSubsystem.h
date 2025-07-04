#pragma once

#include "CoreMinimal.h"

#include "FGSaveInterface.h"
#include "FGResourceSinkSubsystem.h"
#include "FGRecipe.h"
#include "Subsystem/ModSubsystem.h"
#include "Resources/FGItemDescriptor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Module/ModModule.h"

#include "ApMappingsSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApMappingsSubsystem, Log, All);

UENUM()
enum class EItemType : uint8
{
	Item,
	Recipe,
	Building,
	Schematic,
	Special
};

UENUM()
enum class ESpecialItemType : uint8
{
	Inventory3,
	Inventory6,
	Toolbelt1,
	InventoryUpload
};

USTRUCT()
struct ARCHIPELAGO_API FApItemBase
{
	GENERATED_BODY()

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

	UPROPERTY()
	int stackSize;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	int couponCost;
#endif
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

USTRUCT()
struct ARCHIPELAGO_API FApSpecialItem : public FApItemBase
{
	GENERATED_BODY()

	FApSpecialItem() {
		Type = EItemType::Special;
	}

	UPROPERTY()
	ESpecialItemType SpecialType;
};

UCLASS()
class AApMappingsSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	AApMappingsSubsystem();

	virtual void BeginPlay() override;

	static AApMappingsSubsystem* Get(class UWorld* world);

protected:
	// Begin IFGSaveInterface
	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void GatherDependencies_Implementation(TArray<UObject*>& out_dependentObjects) override {};
	virtual bool NeedTransform_Implementation() override { return false; };
	virtual bool ShouldSave_Implementation() const override { return true; };
	// End IFSaveInterface

public:
	static TMap<TSubclassOf<UFGItemDescriptor>, int64> ItemClassToItemId;
	TMap<FString, int64> NameToItemId;
	TMap<int64, TSharedRef<FApItemBase>> ApItems;

	UPROPERTY(SaveGame)
	TMap<int64, FString> ItemIdToName;

private:
	AModSubsystem* ap;

	bool hasLoadedItemNameMappings = false;
	bool hasLoadedItemTraits = false;

	static int64 mamId;
	static int64 shopId;

public:
	FORCEINLINE bool HasLoadedItemNameMappings() const { return hasLoadedItemNameMappings; };

	void InitializeAfterConnectingToAp();

	void DispatchLifecycleEvent(ELifecyclePhase phase);

	FORCEINLINE int64 GetMamItemId() const { return mamId; }
	FORCEINLINE int64 GetAwesomeShopItemId() const { return shopId; }

	static void LoadMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap);
	static void LoadRecipeMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap);
	static void LoadItemMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap);

private:
	static void LoadItemMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap, TMap<FName, const FAssetData>& itemDescriptorAssets);
	static void LoadSpecialItemMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap);
	static void LoadRecipeMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap, TMap<FName, const FAssetData>& recipeAssets);
	static void LoadBuildingMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap, TMap<FName, const FAssetData>& recipeAssets);
	static void LoadSchematicMappings(TMap<int64, TSharedRef<FApItemBase>>& itemMap);

	void LoadNamesFromAP();
	
	static const TMap<FName, const FAssetData> GetItemDescriptorAssets(IAssetRegistry& registery);
	static const TMap<FName, const FAssetData> GetRecipeAssets(IAssetRegistry& registery);
	static const TMap<FName, const FAssetData> GetBlueprintAssetsIn(IAssetRegistry& registery, FName&& packagePath, TArray<FString> namePrefixes, bool searchSubFolders = true);
	
	static UFGRecipe* GetRecipeByName(const TMap<FName, const FAssetData> recipeAssets, FString name);
	static UFGItemDescriptor* GetItemDescriptorByName(const TMap<FName, const FAssetData> itemDescriptorAssets, FString name);
	static UFGSchematic* GetSchematicByName(FString name);

	static UObject* FindAssetByName(const TMap<FName, const FAssetData> assets, FString assetName);
};
