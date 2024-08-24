#include "Subsystem/ApSchematicPatcherSubsystem.h"

DEFINE_LOG_CATEGORY(LogApSchematicPatcherSubsystem);

//TODO REMOVE
#pragma optimize("", off)

#define LOCTEXT_NAMESPACE "Archipelago"

AApSchematicPatcherSubsystem::AApSchematicPatcherSubsystem() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.5f;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

AApSchematicPatcherSubsystem* AApSchematicPatcherSubsystem::Get(class UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApSchematicPatcherSubsystem* AApSchematicPatcherSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApSchematicPatcherSubsystem>();
}

void AApSchematicPatcherSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//TODO Replication
}

void AApSchematicPatcherSubsystem::BeginPlay() {
	UE_LOG(LogApSchematicPatcherSubsystem, Display, TEXT("AApSchematicPatcherSubsystem::BeginPlay()"));

	Super::BeginPlay();
}

void AApSchematicPatcherSubsystem::Initialize() {
	if (isInitialized)
		return;

	isInitialized = true;

	UWorld* world = GetWorld();
	contentLibSubsystem = world->GetGameInstance()->GetSubsystem<UContentLibSubsystem>();
	fgcheck(contentLibSubsystem)
	//contentRegistry = UModContentRegistry::Get(world);
	//fgcheck(contentRegistry)
	ap = AApSubsystem::Get(world);
	fgcheck(ap);
	connectionInfo = AApConnectionInfoSubsystem::Get(world);
	fgcheck(connectionInfo);
	slotDataSubsystem = AApSlotDataSubsystem::Get(world);
	fgcheck(slotDataSubsystem);
	mappingSubsystem = AApMappingsSubsystem::Get(world);
	fgcheck(mappingSubsystem)
}

void AApSchematicPatcherSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (connectionInfo->GetConnectionState() != EApConnectionState::Connected)
		return;
}

/*
TSubclassOf<UFGSchematic> AApSchematicPatcherSubsystem::CreateSchematicBoundToItemId(int64 itemid, TSharedRef<FApRecipeItem> apitem) {
	Initialize();

	FString name = FString::Printf(TEXT("AP_ItemId_%i"), itemid);

	TTuple<bool, TSubclassOf<UFGSchematic>> foundSchematic = UApUtils::FindOrCreateClass(TEXT("/Archipelago/"), *name, UFGSchematic::StaticClass());
	if (!foundSchematic.Key) {
		TArray<FString> recipesToUnlock;
		for (FApRecipeInfo& recipe : apitem->Recipes) {
			recipesToUnlock.Add(recipe.Class->GetName());
		}

		FString typePrefix = apitem->Type == EItemType::Building ? "Building: " : "Recipe: ";
		FString recipeName = apitem->Recipes[0].Recipe->GetDisplayName().ToString();

		FContentLib_Schematic schematic = FContentLib_Schematic();
		schematic.Name = typePrefix + recipeName;
		schematic.Type = "Custom";
		schematic.Recipes = recipesToUnlock;

		UCLSchematicBPFLib::InitSchematicFromStruct(schematic, foundSchematic.Value, contentLibSubsystem);

		contentRegistry->RegisterSchematic(FName(TEXT("Archipelago")), foundSchematic.Value);
	}

	return foundSchematic.Value;
}
*/

void AApSchematicPatcherSubsystem::InitializaHubSchematic(FString name, TSubclassOf<UFGSchematic> factorySchematic, TArray<FApNetworkItem> items) {
	Initialize();

	int delimeterPos;
	name.FindChar('-', delimeterPos);
	int32 tier = FCString::Atoi(*name.Mid(delimeterPos - 1, 1));
	int32 milestone = FCString::Atoi(*name.Mid(delimeterPos + 1, 1));

	FContentLib_Schematic schematic = FContentLib_Schematic();
	schematic.Name = name;
	schematic.Type = "Milestone";
	schematic.Time = 200;
	schematic.Tier = tier;
	schematic.MenuPriority = items[0].location;
	schematic.VisualKit = "Kit_AP_Logo";
	schematic.Cost = slotDataSubsystem->GetSlotData().hubLayout[tier - 1][milestone - 1];

	for (auto& item : items)
		schematic.InfoCards.Add(CreateUnlockInfoOnly(item));

	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, contentLibSubsystem);
}

void AApSchematicPatcherSubsystem::InitializaSchematicForItem(TSubclassOf<UFGSchematic> factorySchematic, FApNetworkItem item, bool updateSchemaName) {
	Initialize();

	FContentLib_UnlockInfoOnly unlockOnlyInfo = CreateUnlockInfoOnly(item);

	FContentLib_Schematic schematic = FContentLib_Schematic();
	schematic.Description = unlockOnlyInfo.mUnlockDescription.ToString();
	//schematic.VisualKit = unlockOnlyInfo.BigIcon; using visual kit didnt work here so we manually patch the icons below
	schematic.InfoCards = TArray<FContentLib_UnlockInfoOnly>{ unlockOnlyInfo };

	if (updateSchemaName)
		schematic.Name = unlockOnlyInfo.mUnlockName.ToString();

	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, contentLibSubsystem);

	UFGSchematic* factorySchematicCDO = Cast<UFGSchematic>(factorySchematic->GetDefaultObject());
	if (factorySchematicCDO != nullptr) {
		// ContentLib keeps adding new `InfoCards`
		if (factorySchematicCDO->mUnlocks.Num() > 1) {
			factorySchematicCDO->mUnlocks.RemoveAt(1, 1, true);
		}

		UTexture2D* bigText = LoadObject<UTexture2D>(nullptr, *unlockOnlyInfo.BigIcon);

		factorySchematicCDO->mSchematicIcon.SetResourceObject(bigText);
		factorySchematicCDO->mSmallSchematicIcon = bigText;

		if (!IsCollected(factorySchematicCDO->mUnlocks[0])) {
			UFGUnlockInfoOnly* unlockInfo = Cast<UFGUnlockInfoOnly>(factorySchematicCDO->mUnlocks[0]);

			if (unlockInfo != nullptr) {
				unlockInfo->mUnlockIconBig = bigText;
				unlockInfo->mUnlockIconSmall = bigText;
			}
		}
	}
}

FContentLib_UnlockInfoOnly AApSchematicPatcherSubsystem::CreateUnlockInfoOnly(FApNetworkItem item) {
	FFormatNamedArguments Args;
	if (item.flags == 0b001) {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeAdvancement", "progression item"));
	}
	else if (item.flags == 0b010) {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeUseful", "useful item"));
	}
	else if (item.flags == 0b100) {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeTrap", "trap"));
	}
	else {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeJunk", "normal item"));
	}

	Args.Add(TEXT("ApItemName"), FText::FromString(item.itemName));

	FContentLib_UnlockInfoOnly infoCard;

	if (item.player == connectionInfo->GetCurrentPlayerSlot()) {
		Args.Add(TEXT("ApPlayerName"), LOCTEXT("NetworkItemDescriptionYourOwnName", "your"));

		infoCard.mUnlockName = FText::FromString(item.itemName);

		if (mappingSubsystem->ApItems.Contains(item.item)) {
			TSharedRef<FApItemBase> apItem = mappingSubsystem->ApItems[item.item];

			switch (apItem->Type) {
				case EItemType::Building:
					UpdateInfoOnlyUnlockWithBuildingInfo(&infoCard, Args, &item, StaticCastSharedRef<FApBuildingItem>(apItem));
					break;
				case EItemType::Recipe:
					UpdateInfoOnlyUnlockWithRecipeInfo(&infoCard, Args, &item, StaticCastSharedRef<FApRecipeItem>(apItem));
					break;
				case EItemType::Item:
					UpdateInfoOnlyUnlockWithItemBundleInfo(&infoCard, Args, &item, StaticCastSharedRef<FApItem>(apItem));
					break;
				case EItemType::Schematic:
					UpdateInfoOnlyUnlockWithSchematicInfo(&infoCard, Args, &item, StaticCastSharedRef<FApSchematicItem>(apItem));
					break;
				case EItemType::Special:
					UpdateInfoOnlyUnlockWithSpecialInfo(&infoCard, Args, &item, StaticCastSharedRef<FApSpecialItem>(apItem));
					break;
			}
		} else {
			UpdateInfoOnlyUnlockWithGenericApInfo(&infoCard, Args, &item);
		}
	}	else {
		Args.Add(TEXT("ApPlayerName"), FText::FormatNamed(LOCTEXT("NetworkItemPlayerOwnerPossessive", "{remotePlayerName}'s"),
			TEXT("remotePlayerName"), FText::FromString(item.playerName)
		));

		infoCard.mUnlockName = FText::Format(LOCTEXT("NetworkItemUnlockDisplayName", "{ApPlayerName} {ApItemName}"), Args);

		UpdateInfoOnlyUnlockWithGenericApInfo(&infoCard, Args, &item);
	}

	return infoCard;
}

void AApSchematicPatcherSubsystem::UpdateInfoOnlyUnlockWithBuildingInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApBuildingItem> itemInfo) {
	UFGRecipe* recipe = itemInfo->Recipes[0].Recipe;
	UFGItemDescriptor* itemDescriptor = recipe->GetProducts()[0].ItemClass.GetDefaultObject();

	infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
	infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Building.Recipe_Icon_Building");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalBuildingDescription", "This will unlock your {ApItemName}"), Args);
}

void AApSchematicPatcherSubsystem::UpdateInfoOnlyUnlockWithRecipeInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApRecipeItem> itemInfo) {
	UFGRecipe* recipe = itemInfo->Recipes[0].Recipe;
	UFGItemDescriptor* itemDescriptor = recipe->GetProducts()[0].ItemClass.GetDefaultObject();

	TArray<FString> BuildingArray;
	TArray<TSubclassOf<UObject>> buildings;
	recipe->GetProducedIn(buildings);
	for (TSubclassOf<UObject>& buildingObject : buildings) {
		if (buildingObject->IsChildOf(AFGBuildable::StaticClass())) {
			AFGBuildable* building = Cast<AFGBuildable>(buildingObject.GetDefaultObject());
			if (building != nullptr) {
				if (building->IsA(AFGBuildableAutomatedWorkBench::StaticClass()))
					BuildingArray.Add("Workbench");
				else
					BuildingArray.Add(building->mDisplayName.ToString());
			}
		}
		else if (buildingObject->IsChildOf(workshopComponent)) {
			BuildingArray.Add("Equipment Workshop");
		}
	}

	TArray<FString> CostsArray;
	for (FItemAmount cost : recipe->GetIngredients()) {
		UFGItemDescriptor* costItemDescriptor = cost.ItemClass.GetDefaultObject();
		CostsArray.Add(costItemDescriptor->GetItemNameFromInstanceAsString());
	}

	TArray<FString> OutputArray;
	for (FItemAmount product : recipe->GetProducts()) {
		UFGItemDescriptor* productItemDescriptor = product.ItemClass.GetDefaultObject();
		OutputArray.Add(productItemDescriptor->GetItemNameFromInstanceAsString());
	}

	Args.Add(TEXT("Building"), FText::FromString(FString::Join(BuildingArray, TEXT(", "))));
	Args.Add(TEXT("Costs"), FText::FromString(FString::Join(CostsArray, TEXT(", "))));
	Args.Add(TEXT("Output"), FText::FromString(FString::Join(OutputArray, TEXT(", "))));

	infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
	infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Decor.Recipe_Icon_Decor");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalRecipeDescription", "This will unlock {ApPlayerName} {ApItemName} which is considered a {ProgressionType}.\nProduced in: {Building}.\nCosts: {Costs}.\nProduces: {Output}."), Args);
}

void AApSchematicPatcherSubsystem::UpdateInfoOnlyUnlockWithItemBundleInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApItem> itemInfo) {
	UFGItemDescriptor* itemDescriptor = itemInfo->Descriptor;

	infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
	infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Item.Recipe_Icon_Item");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalItemDescription", "This will give {ApPlayerName} {ApItemName}. It can be collected by building an Archipelago Portal."), Args);
}

void AApSchematicPatcherSubsystem::UpdateInfoOnlyUnlockWithSchematicInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApSchematicItem> itemInfo) {
	UFGRecipe* recipe = nullptr;
	for (UFGUnlock* unlock : itemInfo->Schematic->mUnlocks) {
		UFGUnlockRecipe* recipeUnlockInfo = Cast<UFGUnlockRecipe>(unlock);
		if (recipeUnlockInfo == nullptr)
			continue;

		TArray<TSubclassOf<UFGRecipe>> recipes = recipeUnlockInfo->GetRecipesToUnlock();
		if (recipes.IsEmpty())
			continue;

		recipe = recipes[0].GetDefaultObject();
		break;
	}

	if (recipe != nullptr) {
		UFGItemDescriptor* itemDescriptor = recipe->GetProducts()[0].ItemClass.GetDefaultObject();

		infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
		infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Building.Recipe_Icon_Building");
		infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalBuildingDescription", "This will unlock your {ApItemName}"), Args);
	}
	else {
		UpdateInfoOnlyUnlockWithGenericApInfo(infoCard, Args, item);
	}
}

void AApSchematicPatcherSubsystem::UpdateInfoOnlyUnlockWithSpecialInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item, TSharedRef<FApSpecialItem> itemInfo) {
	switch (itemInfo->SpecialType) {
	case ESpecialItemType::Inventory3:
	case ESpecialItemType::Inventory6: {
		Args.Add(TEXT("Amount"), itemInfo->SpecialType == ESpecialItemType::Inventory3 ? FText::FromString("3") : FText::FromString("6"));

		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Game/FactoryGame/Interface/UI/Assets/Shared/ThumbsUp_64.ThumbsUp_64");
		infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Upgrade.Recipe_Icon_Upgrade");
		infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalInventoryDescription", "This will inflate {ApPlayerName} pocket-dimension by {Amount}."), Args);
	}
												break;
	case ESpecialItemType::Toolbelt1:
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Game/FactoryGame/Interface/UI/Assets/Shared/ThumbsUp_64.ThumbsUp_64");
		infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Upgrade.Recipe_Icon_Upgrade");
		infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalHandSlotDescription", "This will expand {ApPlayerName} tool-chain by 1."), Args);
		break;
	}
}

void AApSchematicPatcherSubsystem::UpdateInfoOnlyUnlockWithGenericApInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, FApNetworkItem* item) {
	infoCard->CategoryIcon = TEXT("/Archipelago/Assets/SourceArt/ArchipelagoAssetPack/ArchipelagoIconWhite128.ArchipelagoIconWhite128");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockDescription", "This will unlock {ApPlayerName} {ApItemName} which is considered a {ProgressionType}."), Args);

	if (item->flags == 0b001) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/ApLogo/AP-Purple.AP-Purple");
	}
	else if (item->flags == 0b010) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/ApLogo/AP-Blue.AP-Blue");
	}
	else if (item->flags == 0b100) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/ApLogo/AP-Red.AP-Red");
	}
	else {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/ApLogo/AP-Cyan.AP-Cyan");
	}
}

bool AApSchematicPatcherSubsystem::IsCollected(UFGUnlock* unlock) {
	UFGUnlockInfoOnly* unlockInfo = Cast<UFGUnlockInfoOnly>(unlock);
	return IsValid(unlockInfo) && unlockInfo->mUnlockIconSmall == collectedIcon;
}

void AApSchematicPatcherSubsystem::Collect(UFGSchematic* schematic, int unlockIndex, FApNetworkItem& networkItem) {
	Collect(schematic->mUnlocks[unlockIndex], networkItem);
}

void AApSchematicPatcherSubsystem::Collect(UFGUnlock* unlock, FApNetworkItem& networkItem) {
	UFGUnlockInfoOnly* unlockInfo = Cast<UFGUnlockInfoOnly>(unlock);

	if (IsValid(unlockInfo)) {
		if (unlockInfo->mUnlockIconSmall == collectedIcon)
			return;

		unlockInfo->mUnlockIconSmall = collectedIcon;

		if (networkItem.player != connectionInfo->GetCurrentPlayerSlot()) {
			unlockInfo->mUnlockName = FText::Format(LOCTEXT("Collected", "Collected: {0}"), unlockInfo->mUnlockName);
			unlockInfo->mUnlockIconBig = collectedIcon;
		}
	}
}

#pragma optimize("", on)

#undef LOCTEXT_NAMESPACE
