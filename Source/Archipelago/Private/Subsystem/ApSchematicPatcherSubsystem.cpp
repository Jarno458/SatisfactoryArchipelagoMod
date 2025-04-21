#include "Subsystem/ApSchematicPatcherSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "FGGameState.h"
#include "Module/WorldModuleManager.h"
#include "Module/ApGameWorldModule.h"
#include "Data/ApMappings.h"
#include "PushModel.h"
#include "ApUtils.h"

DEFINE_LOG_CATEGORY(LogApSchematicPatcherSubsystem);

//TODO REMOVE
#pragma optimize("", off)

#define LOCTEXT_NAMESPACE "Archipelago"

AApSchematicPatcherSubsystem::AApSchematicPatcherSubsystem() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.5f;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

AApSchematicPatcherSubsystem* AApSchematicPatcherSubsystem::Get(class UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApSchematicPatcherSubsystem* AApSchematicPatcherSubsystem::Get(class UWorld* world) {
	return UApUtils::GetSubsystemActorIncludingParentClases<AApSchematicPatcherSubsystem>(world);
}

void AApSchematicPatcherSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams replicationParams;
	replicationParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AApSchematicPatcherSubsystem, replicatedItemInfos, replicationParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AApSchematicPatcherSubsystem, replicatedMilestones, replicationParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AApSchematicPatcherSubsystem, replicatedCollectedLocations, replicationParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AApSchematicPatcherSubsystem, replicatedStarterRecipes, replicationParams);
}

void AApSchematicPatcherSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApSchematicPatcherSubsystem, Display, TEXT("AApSchematicPatcherSubsystem::BeginPlay()"));

	AGameStateBase* gameState = GetWorld()->GetGameState();

	if (gameState->HasAuthority()) {
		Initialize();
	}
	else {
		if (AFGGameState* factoryGameState = Cast<AFGGameState>(gameState)) {
			factoryGameState->mOnClientSubsystemsValid.AddDynamic(this, &AApSchematicPatcherSubsystem::OnClientSubsystemsValid);

			if (factoryGameState->AreClientSubsystemsValid()) {
				Initialize();
			}
		}
	}
}

void AApSchematicPatcherSubsystem::OnClientSubsystemsValid() {
	Initialize();
}

void AApSchematicPatcherSubsystem::Initialize() {
	if (isInitialized)
		return;

	UWorld* world = GetWorld();
	contentLibSubsystem = world->GetGameInstance()->GetSubsystem<UContentLibSubsystem>();
	fgcheck(contentLibSubsystem)
	connectionInfo = AApConnectionInfoSubsystem::Get(world);
	fgcheck(connectionInfo);
	slotDataSubsystem = AApSlotDataSubsystem::Get(world);
	fgcheck(slotDataSubsystem);
	mappingSubsystem = AApMappingsSubsystem::Get(world);
	fgcheck(mappingSubsystem)
	RManager = AFGResearchManager::Get(world);
	fgcheck(RManager)

	isInitialized = true;
}

void AApSchematicPatcherSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (hasPatchedSchematics) {
		if (IsRunningDedicatedServer())
			SetActorTickEnabled(false);
		else
			Client_ProcessCollectedLocations();
	}

	if (!isInitialized
		|| connectionInfo->GetConnectionState() != EApConnectionState::Connected 
		|| !receivedItemInfos
		|| !receivedMilestones
		|| !receivedStarterRecipes
		|| !slotDataSubsystem->HasLoadedSlotData())
			return;

	if (!hasPatchedSchematics) {
		InitializeSchematicsBasedOnScoutedData();

		hasPatchedSchematics = true;
	}
}

void AApSchematicPatcherSubsystem::Server_SetTier0Recipes(int currentPlayerId, const TArray<FApNetworkItem>& itemInfos) {
	if (!HasAuthority())
		return;

	replicatedStarterRecipes = MakeReplicateable(currentPlayerId, itemInfos);

	MARK_PROPERTY_DIRTY_FROM_NAME(AApSchematicPatcherSubsystem, replicatedStarterRecipes, this);

	OnRep_StarterRecipesReplicated();
}

void AApSchematicPatcherSubsystem::Server_SetItemInfoPerSchematicId(int currentPlayerId, const TArray<FApNetworkItem>& itemInfo) {
	if (!HasAuthority())
		return;

	replicatedItemInfos = MakeReplicateable(currentPlayerId, itemInfo);

	MARK_PROPERTY_DIRTY_FROM_NAME(AApSchematicPatcherSubsystem, replicatedItemInfos, this);
	
	OnRep_ItemInfosReplicated();
}

void AApSchematicPatcherSubsystem::Server_SetItemInfoPerMilestone(int currentPlayerId, const TMap<int, TMap<int, TArray<FApNetworkItem>>>& itemsPerMilestone) {
	if (!HasAuthority())
		return;

	TArray<FApReplicatedMilestoneInfo> milestonesToReplicate;
		for (const TPair<int, TMap<int, TArray<FApNetworkItem>>>& itemsPerMilestonePerTier : itemsPerMilestone) {
		for (const TPair<int, TArray<FApNetworkItem>>& itemsPerMilestoneToReplicate : itemsPerMilestonePerTier.Value) {
			FApReplicatedMilestoneInfo replicatedMilestone;
			replicatedMilestone.tier = itemsPerMilestonePerTier.Key;
			replicatedMilestone.milestone = itemsPerMilestoneToReplicate.Key;
			replicatedMilestone.items = MakeReplicateable(currentPlayerId, itemsPerMilestoneToReplicate.Value);

			milestonesToReplicate.Add(replicatedMilestone);
		}
	}

	replicatedMilestones = milestonesToReplicate;

	MARK_PROPERTY_DIRTY_FROM_NAME(AApSchematicPatcherSubsystem, replicatedItemInfos, this);

	OnRep_MilestonesReplicated();
}

TArray<FApReplicatedItemInfo> AApSchematicPatcherSubsystem::MakeReplicateable(int currentPlayerId, const TArray<FApNetworkItem>& itemInfo) {
	TArray<FApReplicatedItemInfo> itemsToReplicate;
	for (const FApNetworkItem& itemInfoToReplicate : itemInfo) {
		FApReplicatedItemInfo replicatedItem(
			itemInfoToReplicate.itemName, itemInfoToReplicate.playerName, 
			itemInfoToReplicate.item, itemInfoToReplicate.location, itemInfoToReplicate.flags, 
			itemInfoToReplicate.player == currentPlayerId);

		itemsToReplicate.Add(replicatedItem);
	}

	return itemsToReplicate;
}

void AApSchematicPatcherSubsystem::Server_Collect(TSet<int64> locations) {
	if (!HasAuthority())
		return;

	for (int64 location : locations)
		replicatedCollectedLocations.AddUnique(location - ID_OFFSET);

	MARK_PROPERTY_DIRTY_FROM_NAME(AApSchematicPatcherSubsystem, replicatedCollectedLocations, this);

	OnRep_CollectedLocationsReplicated();
}

void AApSchematicPatcherSubsystem::OnRep_ItemInfosReplicated() {
	receivedItemInfos = true;
}

void AApSchematicPatcherSubsystem::OnRep_MilestonesReplicated() {
	receivedMilestones = true;
}

void AApSchematicPatcherSubsystem::OnRep_StarterRecipesReplicated() {
	receivedStarterRecipes = true;
}

void AApSchematicPatcherSubsystem::OnRep_CollectedLocationsReplicated() {
	for (int16 location : replicatedCollectedLocations) {
		int64 location64 = location + ID_OFFSET;

		if (!collectedLocations.Contains(location64))
		{
			collectedLocations.Add(location64);
			clientCollectedLocationsToProcess.Enqueue(location64);
		}
	}
}

void AApSchematicPatcherSubsystem::InitializeSchematicsBasedOnScoutedData() {
	UWorld* world = GetWorld();
	fgcheck(world != nullptr);
	UWorldModuleManager* wordModuleManager = world->GetSubsystem<UWorldModuleManager>();
	fgcheck(wordModuleManager != nullptr);
	UApGameWorldModule* apWorldModule = Cast<UApGameWorldModule>(wordModuleManager->FindModule(FName("Archipelago")));
	fgcheck(apWorldModule != nullptr);
	workshopComponent = LoadClass<UObject>(nullptr, TEXT("/Game/FactoryGame/Buildable/-Shared/WorkBench/BP_WorkshopComponent.BP_WorkshopComponent_C"));
	fgcheck(workshopComponent != nullptr)

	TArray<TSubclassOf<UFGSchematic>> hardcodedSchematics;
	hardcodedSchematics.Append(apWorldModule->mSchematics);
	hardcodedSchematics.Append(apWorldModule->mTreeNodeSchematics);


	InitializeStarterRecipes();

	TMap<int64, FApReplicatedItemInfo> replicatedItemInfoBySchematicId;
	for (const FApReplicatedItemInfo& replicatedItemInfo : replicatedItemInfos) {
		replicatedItemInfoBySchematicId.Add(replicatedItemInfo.GetLocationId(), replicatedItemInfo);

		if (!IsRunningDedicatedServer() && replicatedItemInfo.GetIsLocalPlayer())
			client_locationsWithLocalItems.Add(replicatedItemInfo.GetLocationId());
	}

	TMap<int, TMap<int, TArray<FApReplicatedItemInfo>>> replicatedItemsPerMilestone;
	for (const FApReplicatedMilestoneInfo& replicatedMilestone : replicatedMilestones) {
		if (!replicatedItemsPerMilestone.Contains(replicatedMilestone.tier)) {
			replicatedItemsPerMilestone.Add(replicatedMilestone.tier, TMap<int, TArray<FApReplicatedItemInfo>>());
		}

		replicatedItemsPerMilestone[replicatedMilestone.tier].Add(replicatedMilestone.milestone, replicatedMilestone.items);

		if (!IsRunningDedicatedServer()) {
			for (const FApReplicatedItemInfo& itemInfo : replicatedMilestone.items) {
				if (itemInfo.GetIsLocalPlayer())
					client_locationsWithLocalItems.Add(itemInfo.GetLocationId());
			}
		}
	}

	for (TSubclassOf<UFGSchematic>& schematic : hardcodedSchematics) {
		//The magic, we store AP id's inside the menu priority, and we set techtier to -1 for item send by the server
		int locationId = FMath::RoundToInt(UFGSchematic::GetMenuPriority(schematic));
		if (locationId > ID_OFFSET) {
			bool isItemSchematic = UFGSchematic::GetTechTier(schematic) == -1;
			bool isShop = UFGSchematic::GetType(schematic) == ESchematicType::EST_ResourceSink;

			if (!isItemSchematic && replicatedItemInfoBySchematicId.Contains(locationId)) {
				if (!IsRunningDedicatedServer())
					client_collectableSchematicPerLocation.Add(locationId, schematic);

				InitializaSchematicForItem(schematic, replicatedItemInfoBySchematicId[locationId], isShop);
			}
		} else if (UFGSchematic::GetType(schematic) == ESchematicType::EST_Milestone) {
			int tier = UFGSchematic::GetTechTier(schematic);
			int milestone = FMath::RoundToInt(UFGSchematic::GetMenuPriority(schematic));

			if (tier <= 0 || milestone <= 0)
				continue;

			if (replicatedItemsPerMilestone.Contains(tier) && replicatedItemsPerMilestone[tier].Contains(milestone)) {
				if (!IsRunningDedicatedServer()) {
					for (const FApReplicatedItemInfo& itemInfo : replicatedItemsPerMilestone[tier][milestone]) {
						client_collectableSchematicPerLocation.Add(itemInfo.GetLocationId(), schematic);
					}
				}

				InitializeHubSchematic(schematic, replicatedItemsPerMilestone[tier][milestone], slotDataSubsystem->GetCostsForMilestone(tier, milestone));
			} else {
				// I dont think this should ever happen, maybe we should yeet here
				InitializeHubSchematic(schematic, TArray<FApReplicatedItemInfo>(), slotDataSubsystem->GetCostsForMilestone(tier, milestone));
			}
		}
	}
}

void AApSchematicPatcherSubsystem::InitializeStarterRecipes() {
	FContentLib_Schematic schematic = FContentLib_Schematic();

	//got to clear entries from previus saves
	UFGSchematic* factorySchematicCDO = Cast<UFGSchematic>(tierOSchematic->GetDefaultObject());
	factorySchematicCDO->mUnlocks.Empty();

	for (const FApReplicatedItemInfo& item : replicatedStarterRecipes)
		schematic.InfoCards.Add(CreateUnlockInfoOnly(item));

	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, tierOSchematic, contentLibSubsystem);
}

void AApSchematicPatcherSubsystem::InitializeHubSchematic(TSubclassOf<UFGSchematic> factorySchematic, const TArray<FApReplicatedItemInfo>& items, const TMap<int64, int>& costs) {
	FContentLib_Schematic schematic = FContentLib_Schematic();

	TMap<FString, int> costsByClassName;
	for (const TPair<int64, int> costByItemId : costs) {
		if (UApMappings::ItemIdToGameItemDescriptor.Contains(costByItemId.Key)) {
			costsByClassName.Add(UApMappings::ItemIdToGameItemDescriptor[costByItemId.Key], costByItemId.Value);
		}
	}

	schematic.Cost = costsByClassName;

	//got to clear entries from previus saves
	UFGSchematic* factorySchematicCDO = Cast<UFGSchematic>(factorySchematic->GetDefaultObject());
	factorySchematicCDO->mUnlocks.Empty();

	if (items.IsEmpty()) {
		schematic.DependsOn.Add("Schematic_AP_lock"); //hide empty milestones
	} else {
		schematic.ClearDeps = true; //unhide if it was hidden
		for (const FApReplicatedItemInfo& item : items)
			schematic.InfoCards.Add(CreateUnlockInfoOnly(item));
	}

	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, contentLibSubsystem);
}

void AApSchematicPatcherSubsystem::InitializaSchematicForItem(TSubclassOf<UFGSchematic> factorySchematic, const FApReplicatedItemInfo& item, bool updateSchemaName) {
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
		// we carefully remove here as we still expect mUnlocks[0] to be an UFGUnlockInfoOnly
		if (factorySchematicCDO->mUnlocks.Num() > 1) {
			factorySchematicCDO->mUnlocks.RemoveAt(1, 1, true);
		}
		
		UTexture2D* bigText = LoadObject<UTexture2D>(nullptr, *unlockOnlyInfo.BigIcon);

		factorySchematicCDO->mSchematicIcon.SetResourceObject(bigText);
		factorySchematicCDO->mSmallSchematicIcon = bigText;
	}
}

FContentLib_UnlockInfoOnly AApSchematicPatcherSubsystem::CreateUnlockInfoOnly(const FApReplicatedItemInfo& item) {
	int flags = item.GetFlags();
	FFormatNamedArguments Args;
	if (flags == 0b001) {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeAdvancement", "progression item"));
	}
	else if (flags == 0b010) {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeUseful", "useful item"));
	}
	else if (flags == 0b100) {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeTrap", "trap"));
	}
	else {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeJunk", "normal item"));
	}

	FContentLib_UnlockInfoOnly infoCard;

	if (item.GetIsLocalPlayer()) {
		FString itemName = item.itemName;

		Args.Add(TEXT("ApPlayerName"), LOCTEXT("NetworkItemDescriptionYourOwnName", "your"));
		Args.Add(TEXT("ApItemName"), FText::FromString(itemName));

		infoCard.mUnlockName = FText::FromString(itemName);

		int64 itemId = item.GetItemId();
		if (mappingSubsystem->ApItems.Contains(itemId)) {
			TSharedRef<FApItemBase> apItem = mappingSubsystem->ApItems[itemId];

			switch (apItem->Type) {
				case EItemType::Building:
					UpdateInfoOnlyUnlockWithBuildingInfo(&infoCard, Args, item, StaticCastSharedRef<FApBuildingItem>(apItem));
					break;
				case EItemType::Recipe:
					UpdateInfoOnlyUnlockWithRecipeInfo(&infoCard, Args, item, StaticCastSharedRef<FApRecipeItem>(apItem));
					break;
				case EItemType::Item:
					UpdateInfoOnlyUnlockWithItemBundleInfo(&infoCard, Args, item, StaticCastSharedRef<FApItem>(apItem));
					break;
				case EItemType::Schematic:
					UpdateInfoOnlyUnlockWithSchematicInfo(&infoCard, Args, item, StaticCastSharedRef<FApSchematicItem>(apItem));
					break;
				case EItemType::Special:
					UpdateInfoOnlyUnlockWithSpecialInfo(&infoCard, Args, item, StaticCastSharedRef<FApSpecialItem>(apItem));
					break;
			}
		} else {
			UpdateInfoOnlyUnlockWithGenericApInfo(&infoCard, Args, item);
		}
	}	else {
		Args.Add(TEXT("ApPlayerName"), FText::FormatNamed(LOCTEXT("NetworkItemPlayerOwnerPossessive", "{remotePlayerName}'s"),
			TEXT("remotePlayerName"), FText::FromString(item.playerName)
		));
		Args.Add(TEXT("ApItemName"), FText::FromString(item.itemName));

		infoCard.mUnlockName = FText::Format(LOCTEXT("NetworkItemUnlockDisplayName", "{ApPlayerName} {ApItemName}"), Args);

		UpdateInfoOnlyUnlockWithGenericApInfo(&infoCard, Args, item);
	}

	return infoCard;
}

void AApSchematicPatcherSubsystem::UpdateInfoOnlyUnlockWithBuildingInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApBuildingItem> itemInfo) {
	UFGRecipe* recipe = itemInfo->Recipes[0].Recipe;
	UFGItemDescriptor* itemDescriptor = recipe->GetProducts()[0].ItemClass.GetDefaultObject();

	infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
	infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Building.Recipe_Icon_Building");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalBuildingDescription", "This will unlock your {ApItemName}"), Args);
}

void AApSchematicPatcherSubsystem::UpdateInfoOnlyUnlockWithRecipeInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApRecipeItem> itemInfo) {
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
		} else if (buildingObject->IsChildOf(workshopComponent)) {
			BuildingArray.Add("Equipment Workshop");
		}
	}

	TArray<FString> CostsArray;
	for (const FItemAmount& cost : recipe->GetIngredients()) {
		UFGItemDescriptor* costItemDescriptor = cost.ItemClass.GetDefaultObject();
		CostsArray.Add(costItemDescriptor->GetItemNameFromInstanceAsString());
	}

	TArray<FString> OutputArray;
	for (const FItemAmount& product : recipe->GetProducts()) {
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

void AApSchematicPatcherSubsystem::UpdateInfoOnlyUnlockWithItemBundleInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApItem> itemInfo) {
	UFGItemDescriptor* itemDescriptor = itemInfo->Descriptor;

	infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
	infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Item.Recipe_Icon_Item");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalItemDescription", "This will give {ApPlayerName} {ApItemName}. It can be collected by building an Archipelago Portal."), Args);
}

void AApSchematicPatcherSubsystem::UpdateInfoOnlyUnlockWithSchematicInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApSchematicItem> itemInfo) {
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

void AApSchematicPatcherSubsystem::UpdateInfoOnlyUnlockWithSpecialInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApSpecialItem> itemInfo) {
	switch (itemInfo->SpecialType) {
	case ESpecialItemType::Inventory3:
	case ESpecialItemType::Inventory6:
		Args.Add(TEXT("Amount"), itemInfo->SpecialType == ESpecialItemType::Inventory3 ? FText::FromString("3") : FText::FromString("6"));

		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Game/FactoryGame/Buildable/Factory/Mam/UI/TXUI_InventoryUpgrade_256.TXUI_InventoryUpgrade_256");
		infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Upgrade.Recipe_Icon_Upgrade");
		infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalInventoryDescription", "This will inflate {ApPlayerName} Pocket Dimension by {Amount} slots."), Args);
		break;

	case ESpecialItemType::Toolbelt1:
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Game/FactoryGame/Buildable/Factory/Mam/UI/TXUI_HandUpgrade_256.TXUI_HandUpgrade_256");
		infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Upgrade.Recipe_Icon_Upgrade");
		infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalHandSlotDescription", "This will expand {ApPlayerName} Toolbelt by 1 slot."), Args);
		break;

	case ESpecialItemType::InventoryUpload:
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Game/FactoryGame/Buildable/Factory/Mam/UI/TXUI_UploadUpgrade_256.TXUI_UploadUpgrade_256");
		infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Upgrade.Recipe_Icon_Upgrade");
		infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockInventoryUpload", "This will enable {ApPlayerName} uploading to the Dimensional Depot from the inventory."), Args);
		break;
	}
}

void AApSchematicPatcherSubsystem::UpdateInfoOnlyUnlockWithGenericApInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item) {
	infoCard->CategoryIcon = TEXT("/Archipelago/Assets/SourceArt/ArchipelagoAssetPack/ArchipelagoIconWhite128.ArchipelagoIconWhite128");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockDescription", "This will unlock {ApPlayerName} {ApItemName} which is considered a {ProgressionType}."), Args);

	int flags = item.GetFlags();
	if ((flags & 0b001) > 0) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/ApLogo/AP-Purple.AP-Purple");
	}
	else if ((flags & 0b010) > 0) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/ApLogo/AP-Blue.AP-Blue");
	}
	else if (flags == 0b100) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/ApLogo/AP-Red.AP-Red");
	}
	else {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/DerivedArt/ApLogo/AP-Cyan.AP-Cyan");
	}
}

void AApSchematicPatcherSubsystem::Client_ProcessCollectedLocations() {
	if (IsRunningDedicatedServer())
		return;

	int64 location;
	if (clientCollectedLocationsToProcess.Dequeue(location)) {
		if (!client_collectableSchematicPerLocation.Contains(location) || !IsValid(client_collectableSchematicPerLocation[location]))
			return;

		TSubclassOf<UFGSchematic> schematic = client_collectableSchematicPerLocation[location];
		TArray<UFGUnlock*> unlocks = UFGSchematic::GetUnlocks(schematic);

		if (unlocks.Num() == 0)
			return;

		if (UFGSchematic::GetType(schematic) == ESchematicType::EST_Milestone) {
			Client_Collect(unlocks[location % 10], client_locationsWithLocalItems.Contains(location));
		} 
		else {
			Client_Collect(unlocks[0], client_locationsWithLocalItems.Contains(location));
		}
	}
}

void AApSchematicPatcherSubsystem::Client_Collect(UFGUnlock* unlock, bool isLocalItem) {
	UFGUnlockInfoOnly* unlockInfo = Cast<UFGUnlockInfoOnly>(unlock);

	if (IsValid(unlockInfo)) {
		unlockInfo->mUnlockIconSmall = collectedIcon;

		if (!isLocalItem) {
			unlockInfo->mUnlockName = FText::Format(LOCTEXT("Collected", "Collected: {0}"), unlockInfo->mUnlockName);
			unlockInfo->mUnlockIconBig = collectedIcon;
		}
	}
}

#pragma optimize("", on)

#undef LOCTEXT_NAMESPACE
