#include "Subsystem/ApServerRandomizerSubsystem.h"
#include "Engine\DamageEvents.h"
#include "FGGamePhase.h"
#include "FGCentralStorageSubsystem.h"

#include "data/ApMappings.h"

DEFINE_LOG_CATEGORY(LogApServerRandomizerSubsystem);

//TODO REMOVE
#pragma optimize("", off)

#define LOCTEXT_NAMESPACE "Archipelago"

AApServerRandomizerSubsystem::AApServerRandomizerSubsystem() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.5f;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

AApServerRandomizerSubsystem* AApServerRandomizerSubsystem::Get(class UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApServerRandomizerSubsystem* AApServerRandomizerSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApServerRandomizerSubsystem>();
}

void AApServerRandomizerSubsystem::DispatchLifecycleEvent(ELifecyclePhase phase, const TArray<TSubclassOf<UFGSchematic>>& apHardcodedSchematics) {
	UE_LOG(LogApServerRandomizerSubsystem, Display, TEXT("AApServerRandomizerSubsystem()::DispatchLifecycleEvent(%s)"), *UEnum::GetValueAsString(phase));

	if (!HasAuthority())
		UE_LOG(LogApServerRandomizerSubsystem, Fatal, TEXT("AApServerRandomizerSubsystem()::DispatchLifecycleEvent() Called without authority"));

	if (phase == ELifecyclePhase::CONSTRUCTION) {
		hardcodedSchematics = apHardcodedSchematics;
	}
	else if (phase == ELifecyclePhase::INITIALIZATION) {
		UWorld* world = GetWorld();

		contentRegistry = UModContentRegistry::Get(world);
		fgcheck(contentRegistry)
		ap = AApSubsystem::Get(world);
		fgcheck(ap);
		connectionInfo = AApConnectionInfoSubsystem::Get(world);
		fgcheck(connectionInfo);
		slotData = AApSlotDataSubsystem::Get(world);
		fgcheck(slotData);
		schematicPatcher = AApSchematicPatcherSubsystem::Get(world);
		fgcheck(schematicPatcher);
		mappingSubsystem = AApMappingsSubsystem::Get(world);
		fgcheck(mappingSubsystem)
		RManager = AFGResearchManager::Get(world);
		fgcheck(RManager)
		unlockSubsystem = AFGUnlockSubsystem::Get(world);
		fgcheck(unlockSubsystem)
		hardDriveGachaSubsystem = AApHardDriveGachaSubsystem::Get(world);
		fgcheck(hardDriveGachaSubsystem)

		FApConfigurationStruct config = ap->GetConfig();
		fgcheck(config.IsLoaded()) //should be available from start of INITIALIZATION phase
		if (!config.Enabled)
			return;

		ap->SetItemReceivedCallback([this](int64 itemid, bool isFromServer) { ReceiveItem(itemid, isFromServer); });
		ap->SetLocationCheckedCallback([this](int64 itemid) { CollectLocation(itemid); });
		ap->SetDeathLinkReceivedCallback([this](FText message) { OnDeathLinkReceived(message); });
		ap->SetReconnectCallback([this]() { ResetCurrentItemCounter(); });

		FGenericPlatformProcess::ConditionalSleep([this]() { return InitializeTick(); }, 0.5);

		FinalizeInitialization();
	}
	else if (phase == ELifecyclePhase::POST_INITIALIZATION) {
		SetActorTickEnabled(true);

		if (connectionInfo->GetConnectionState() == EApConnectionState::Connected) {
			TArray<TSubclassOf<UFGSchematic>> unlockedSchematics;

			SManager->GetAllPurchasedSchematics(unlockedSchematics);

			for (TSubclassOf<UFGSchematic>& schematic : unlockedSchematics)
				OnSchematicCompleted(schematic);
		}
	}
}

bool AApServerRandomizerSubsystem::InitializeTick() {
	EApConnectionState connectionState = connectionInfo->GetConnectionState();

	if (connectionState == EApConnectionState::Connected) {
		if (!slotData->HasLoadedSlotData())
			slotData->SetSlotDataJson(connectionInfo->GetSlotDataJson());

		if (scoutedLocations.IsEmpty() && slotData->HasLoadedSlotData())
			ScoutArchipelagoItems();

		if (!mappingSubsystem->HasLoadedItemNameMappings())
			mappingSubsystem->InitializeAfterConnectingToAp();
	}

	if (!areRecipiesAndSchematicsInitialized
		&& !scoutedLocations.IsEmpty()
		&& slotData->HasLoadedSlotData()
		&& mappingSubsystem->HasLoadedItemNameMappings()) {

		ParseScoutedItemsAndCreateRecipiesAndSchematics();
	}
	
	return connectionState == EApConnectionState::ConnectionFailed
		|| (connectionState == EApConnectionState::Connected && areRecipiesAndSchematicsInitialized);
}

void AApServerRandomizerSubsystem::ScoutArchipelagoItems() {
	UE_LOG(LogApServerRandomizerSubsystem, Display, TEXT("AApServerRandomizerSubsystem::ScoutArchipelagoItems()"));

	TSet<int64> locations;

	int maxMilestones = 5;
	int maxSlots = 10;

	int64 hubBaseId = 1338000;

	for (int tier = 1; tier <= slotData->GetNumberOfHubTiers(); tier++) {
		for (int milestone = 1; milestone <= maxMilestones; milestone++) {
			for (int slot = 1; slot <= maxSlots; slot++) {
				if (milestone <= slotData->GetNumberOfMilestonesForTier(tier) && slot <= slotData->NumberOfChecksPerMilestone)
					locations.Add(hubBaseId);

				hubBaseId++;
			}
		}
	}

	//mam locations
	for (int l = 1338500; l <= 1338593; l++)
		locations.Add(l);

	//hardrive locations
	for (int l = 1338600; l <= 1338699; l++)
		locations.Add(l);
	
	//shop locations
	for (int l = 1338700; l <= 1338709; l++)
		locations.Add(l);

	TMap<int64, FApNetworkItem> scoutResults = ap->ScoutLocation(locations);

	scoutedLocations.Empty();

	for (const TPair<int64, const FApNetworkItem> scoutResult : scoutResults) {
		scoutedLocations.Add(scoutResult.Value);
	}
}

void AApServerRandomizerSubsystem::ParseScoutedItemsAndCreateRecipiesAndSchematics() {
	UE_LOG(LogApServerRandomizerSubsystem, Display, TEXT("AApSubsystem::ParseScoutedItemsAndCreateRecipiesAndSchematics()"));

	TMap<FString, TSubclassOf<UFGSchematic>> schematicsPerMilestone = TMap<FString, TSubclassOf<UFGSchematic>>();
	TMap<int64, TSubclassOf<UFGSchematic>> schematicsPerLocation = TMap<int64, TSubclassOf<UFGSchematic>>();

	for (const TSubclassOf<UFGSchematic>& schematic : hardcodedSchematics) {
		//The magic, we store AP id's inside the menu priority, and we set techtier to -1 for item send by the server
		int locationId = FMath::RoundToInt(UFGSchematic::GetMenuPriority(schematic));
		if (locationId > 1338000) {
			bool isItemSchematic = UFGSchematic::GetTechTier(schematic) == -1;

			if (isItemSchematic) {
				ItemSchematics.Add(locationId, schematic);
			} else {
				schematicsPerLocation.Add(locationId, schematic);
			}
		}
	}

	//used by schamtic patcher
	TArray<FApNetworkItem> itemInfoPerSchematicId;
	TMap<int, TMap<int, TArray<FApNetworkItem>>> itemInfosPerMilestone;

	for (const FApNetworkItem& location : scoutedLocations) {
		if (location.locationName.StartsWith("Hub")) {
			//location.locationName = "Hub 1-1, Item 1"
			FString milestoneName = location.locationName.Left(location.locationName.Find(","));

			int delimeterPos;
			milestoneName.FindChar('-', delimeterPos);
			int tier = FCString::Atoi(*milestoneName.Mid(delimeterPos - 1, 1));
			int milestone = FCString::Atoi(*milestoneName.Mid(delimeterPos + 1, 1));

			FString bpName = FString::Format(TEXT("/Archipelago/Schematics/AP_HubSchematics/AP_HUB_{0}_{1}.AP_HUB_{0}_{1}_C"), { tier, milestone });

			TSubclassOf<UFGSchematic> schematic = LoadClass<UFGSchematic>(nullptr, *bpName);
			fgcheck(schematic != nullptr)

			if (!schematicsPerMilestone.Contains(milestoneName)) {
				schematicsPerMilestone.Add(milestoneName, schematic);
			}

			if (!locationsPerMilestone.Contains(schematicsPerMilestone[milestoneName])) {
				locationsPerMilestone.Add(schematic, TArray<FApNetworkItem>{ location });

				if (!itemInfosPerMilestone.Contains(tier)) {
					itemInfosPerMilestone.Add(tier, TMap<int, TArray<FApNetworkItem>>());
				}
				itemInfosPerMilestone[tier].Add(milestone, TArray<FApNetworkItem>{ location });
			} else {
				locationsPerMilestone[schematicsPerMilestone[milestoneName]].Add(location);

				itemInfosPerMilestone[tier][milestone].Add(location);
			}
		} else if (schematicsPerLocation.Contains(location.location)) {
			itemInfoPerSchematicId.Add(location);

			ESchematicType type = UFGSchematic::GetType(schematicsPerLocation[location.location]);

			switch (type) {
				case ESchematicType::EST_MAM:
					locationPerMamNode.Add(schematicsPerLocation[location.location], location);
					break;

				case ESchematicType::EST_Alternate:
					locationPerHardDrive.Add(schematicsPerLocation[location.location], location);
					break;

				case ESchematicType::EST_ResourceSink:
					locationPerShopNode.Add(schematicsPerLocation[location.location], location);
					break;
			}
		}
	}

	int currentPlayer = connectionInfo->GetCurrentPlayerSlot();

	schematicPatcher->Server_SetItemInfoPerSchematicId(currentPlayer, itemInfoPerSchematicId);
	schematicPatcher->Server_SetItemInfoPerMilestone(currentPlayer, itemInfosPerMilestone);

	areRecipiesAndSchematicsInitialized = true;
}

void AApServerRandomizerSubsystem::FinalizeInitialization() {
	//must be called before BeginPlay() and after connecting to AP as FreeSamples caches its exclude radioactive flag there
	if (!UpdateFreeSamplesConfiguration()) {
		UE_LOG(LogApServerRandomizerSubsystem, Error, TEXT("AApServerRandomizerSubsystem::FinalizeInitialization() Failed to update configuration of Free Samples"));
	}

	TArray<TSubclassOf<class UFGResearchTree>> researchTrees;
	RManager->GetAllResearchTrees(researchTrees);

	for (const TSubclassOf<UFGResearchTree>& tree : researchTrees) {
		const UFGResearchTree* treeCDO = Cast<UFGResearchTree>(tree->GetDefaultObject());
		if (treeCDO != nullptr) {
			FString className = treeCDO->GetName();

			if (!className.Contains("AP_") && !className.EndsWith("HardDrive_C") && !className.EndsWith("XMas_C"))
				contentRegistry->RemoveResearchTree(tree);
		}
	};

	TArray<TSubclassOf<class UFGSchematic>> hardDriveSchematics;
	locationPerHardDrive.GetKeys(hardDriveSchematics);
	hardDriveGachaSubsystem->Initialize(hardDriveSchematics);
}

void AApServerRandomizerSubsystem::BeginPlay() {
	UE_LOG(LogApServerRandomizerSubsystem, Display, TEXT("AApServerRandomizerSubsystem::BeginPlay()"));

	Super::BeginPlay();

	UWorld* world = GetWorld();
	SManager = AFGSchematicManager::Get(world);
	RManager = AFGResearchManager::Get(world);

	portalSubsystem = AApPortalSubsystem::Get(world);
	mappingSubsystem = AApMappingsSubsystem::Get(world);
	trapSubsystem = AApTrapSubsystem::Get(world);
	phaseManager = AFGGamePhaseManager::Get(world);

	RManager->ResearchCompletedDelegate.AddDynamic(this, &AApServerRandomizerSubsystem::OnMamResearchCompleted);
	RManager->ResearchTreeUnlockedDelegate.AddDynamic(this, &AApServerRandomizerSubsystem::OnMamResearchTreeUnlocked);
	SManager->PurchasedSchematicDelegate.AddDynamic(this, &AApServerRandomizerSubsystem::OnSchematicCompleted);
}

void AApServerRandomizerSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (portalSubsystem->IsInitialized())
		ProcessReceivedItems();

	if (connectionInfo->GetConnectionState() != EApConnectionState::Connected)
		return;

	HandleCheckedLocations();
	HandleDeathLink();

	if (phaseManager->GetCurrentGamePhase()->mGamePhase > lastGamePhase) {
		OnAvaiableSchematicsChanged();
		lastGamePhase = phaseManager->GetCurrentGamePhase()->mGamePhase;
	}
}

void AApServerRandomizerSubsystem::ReceiveItem(int64 itemid, bool isFromServer) {
	ReceivedItems.Enqueue(TTuple<int64, bool>(itemid, isFromServer));
}

void AApServerRandomizerSubsystem::ProcessReceivedItems() {
	TTuple<int64, bool> item;
	while (ReceivedItems.Dequeue(item)) {
		if (connectionInfo->GetConnectionState() == EApConnectionState::Connected && ++currentItemIndex < lastProcessedItemIndex)
			continue;

		AwardItem(item.Key, item.Value);

		lastProcessedItemIndex++;
	}
}

void AApServerRandomizerSubsystem::ResetCurrentItemCounter() {
	UE_LOG(LogApServerRandomizerSubsystem, Display, TEXT("AApServerRandomizerSubsystem::ResetCurrentItemCounter()"));

	currentItemIndex = 0;
}

void AApServerRandomizerSubsystem::ResetDuplicationCounter() {
	lastProcessedItemIndex = 0;

	FText message = NSLOCTEXT("Archipelago", "ResetDuplicationCounterMessage", "Item duplication counter reset, please save your game and reload that save!");
	ap->AddChatMessage(message, FLinearColor::Blue);
}

void AApServerRandomizerSubsystem::OnDeathLinkReceived(FText message) {
	UE_LOG(LogApServerRandomizerSubsystem, Display, TEXT("AApServerRandomizerSubsystem::OnDeathLinkReceived()"));
	
	instagib = true;
	ap->AddChatMessage(message, FLinearColor::Red);
}

void AApServerRandomizerSubsystem::HandleDeathLink() {
	if (IsRunningDedicatedServer())
		return; // TODO make deathlink work for dedicated servers

	const AFGPlayerController* playerController = UFGBlueprintFunctionLibrary::GetLocalPlayerController(GetWorld());
	if (playerController == nullptr)
		return;

	AFGCharacterPlayer* player = Cast<AFGCharacterPlayer>(playerController->GetControlledCharacter());
	if (player == nullptr)
		return;

	HandleInstagib(player);

	if (player->IsAliveAndWell()) {
		awaitingHealty = false;
	} else {
		if (!awaitingHealty) {
			awaitingHealty = true;

			ap->TriggerDeathLink();
		}
	}
}

void AApServerRandomizerSubsystem::HandleInstagib(AFGCharacterPlayer* player) {
	if (instagib) {
		instagib = false;

		const TSubclassOf<UDamageType> damageType = TSubclassOf<UDamageType>(UDamageType::StaticClass());
		FDamageEvent instagibDamageEvent = FDamageEvent(damageType);
		player->TakeDamage(1333337, instagibDamageEvent, player->GetFGPlayerController(), player);
	}
}

bool AApServerRandomizerSubsystem::UpdateFreeSamplesConfiguration() {
	const FConfigId FreeSamplesConfigId{ "FreeSamples", "" };

	UConfigManager* ConfigManager = GetWorld()->GetGameInstance()->GetSubsystem<UConfigManager>();
	if (ConfigManager == nullptr) {
		return false;
	}

	UConfigPropertySection* configRoot = ConfigManager->GetConfigurationRootSection(FreeSamplesConfigId);
	if (configRoot == nullptr) {
		return false;
	}

	if (configRoot->SectionProperties.Contains("Equipment")) {
		UConfigPropertySection* EquipmentSection = Cast<UConfigPropertySection>(configRoot->SectionProperties["Equipment"]);
		if (EquipmentSection != nullptr && EquipmentSection->SectionProperties.Contains("Quantity")) {
			UConfigPropertyInteger* quantity = Cast<UConfigPropertyInteger>(EquipmentSection->SectionProperties["Quantity"]);
			if (quantity != nullptr)
				quantity->Value = slotData->FreeSampleEquipment;
		}
	}

	if (configRoot->SectionProperties.Contains("Buildings")) {
		UConfigPropertySection* BuildingsSection = Cast<UConfigPropertySection>(configRoot->SectionProperties["Buildings"]);
		if (BuildingsSection != nullptr && BuildingsSection->SectionProperties.Contains("Quantity")) {
			UConfigPropertyInteger* quantity = Cast<UConfigPropertyInteger>(BuildingsSection->SectionProperties["Quantity"]);
			if (quantity != nullptr)
				quantity->Value = slotData->FreeSampleBuildings;
		}
	}

	if (configRoot->SectionProperties.Contains("Parts")) {
		UConfigPropertySection* PartsSection = Cast<UConfigPropertySection>(configRoot->SectionProperties["Parts"]);
		if (PartsSection != nullptr && PartsSection->SectionProperties.Contains("Quantity")) {
			UConfigPropertyInteger* quantity = Cast<UConfigPropertyInteger>(PartsSection->SectionProperties["Quantity"]);
			if (quantity != nullptr)
				quantity->Value = slotData->FreeSampleParts;
		}
	}

	if (configRoot->SectionProperties.Contains("Exclude")) {
		UConfigPropertySection* ExcludeSection = Cast<UConfigPropertySection>(configRoot->SectionProperties["Exclude"]);
		if (ExcludeSection != nullptr && ExcludeSection->SectionProperties.Contains("SkipRadioactive")) {
			UConfigPropertyBool* excludeRadioActive = Cast<UConfigPropertyBool>(ExcludeSection->SectionProperties["SkipRadioactive"]);
			if (excludeRadioActive != nullptr)
				excludeRadioActive->Value = !slotData->FreeSampleRadioactive;
		}
	}

	ConfigManager->MarkConfigurationDirty(FreeSamplesConfigId);
	ConfigManager->FlushPendingSaves();
	ConfigManager->ReloadModConfigurations();

	return true;
}

void AApServerRandomizerSubsystem::OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(LogApServerRandomizerSubsystem, Display, TEXT("AApSubSystem::OnResearchCompleted(schematic)"));

	OnAvaiableSchematicsChanged();
}

void AApServerRandomizerSubsystem::OnMamResearchTreeUnlocked(TSubclassOf<class UFGResearchTree> researchTree) {
	UE_LOG(LogApServerRandomizerSubsystem, Display, TEXT("AApSubSystem::OnMamResearchTreeUnlocked(researchTree)"));

	OnAvaiableSchematicsChanged();
}

void AApServerRandomizerSubsystem::OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(LogApServerRandomizerSubsystem, Display, TEXT("AApSubSystem::OnSchematicCompleted(schematic)"));

	ESchematicType type = UFGSchematic::GetType(schematic);
	TSet<FApNetworkItem*> itemsToUnlock;

	switch (type) {
	case ESchematicType::EST_Milestone:
		if (locationsPerMilestone.Contains(schematic)) {
			for (FApNetworkItem& location : locationsPerMilestone[schematic])
				itemsToUnlock.Add(&location);
		}
		break;
	case ESchematicType::EST_MAM:
		if (locationPerMamNode.Contains(schematic))
			itemsToUnlock.Add(&locationPerMamNode[schematic]);
		break;
	case ESchematicType::EST_ResourceSink:
		if (locationPerShopNode.Contains(schematic))
			itemsToUnlock.Add(&locationPerShopNode[schematic]);
		break;
	case ESchematicType::EST_Custom:
		if (schematic == ItemSchematics[mappingSubsystem->GetAwesomeShopItemId()]
			|| schematic == ItemSchematics[mappingSubsystem->GetMamItemId()])
			OnAvaiableSchematicsChanged();
		break;
	case ESchematicType::EST_Alternate:
		if (locationPerHardDrive.Contains(schematic))
			itemsToUnlock.Add(&locationPerHardDrive[schematic]);
		break;
	default:
		return;
	}

	if (connectionInfo->GetConnectionState() == EApConnectionState::Connected) {
		TSet<int64> unlockedChecks;

		for (const FApNetworkItem* item : itemsToUnlock)
			unlockedChecks.Add(item->location);
		
		if (unlockedChecks.Num() > 0)
			ap->CheckLocation(unlockedChecks);
	} else {
		for (const FApNetworkItem* item : itemsToUnlock) {
			if (item->player == connectionInfo->GetCurrentPlayerSlot())
				ReceivedItems.Enqueue(TTuple<int64, bool>(item->item, false));
		}
	}
}

void AApServerRandomizerSubsystem::OnAvaiableSchematicsChanged() {
	TSet<int64> locationHintsToPublish;

	int maxAvailableTechTier = phaseManager->GetCurrentGamePhase()->mLastTierOfPhase;
	int currentPlayerSlot = connectionInfo->GetCurrentPlayerSlot();

	for (const TPair<TSubclassOf<UFGSchematic>, TArray<FApNetworkItem>>& itemPerMilestone : locationsPerMilestone) {
		if (UFGSchematic::GetTechTier(itemPerMilestone.Key) <= maxAvailableTechTier) {
			for (const FApNetworkItem& item : itemPerMilestone.Value) {
				if (item.player != currentPlayerSlot && (item.flags & 0b011) > 0)
					locationHintsToPublish.Add(item.location);
			}
		}
	}

	if (SManager->IsSchematicPurchased(ItemSchematics[mappingSubsystem->GetMamItemId()])) {
		for (const TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerMamNode : locationPerMamNode) {
			if (itemPerMamNode.Value.player != currentPlayerSlot
				&& (itemPerMamNode.Value.flags & 0b011) > 0
				&& RManager->CanResearchBeInitiated(itemPerMamNode.Key))

				locationHintsToPublish.Add(itemPerMamNode.Value.location);
		}
	}

	if (SManager->IsSchematicPurchased(ItemSchematics[mappingSubsystem->GetAwesomeShopItemId()])) {
		for (const TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerShopNode : locationPerShopNode) {
			if (itemPerShopNode.Value.player != currentPlayerSlot && (itemPerShopNode.Value.flags & 0b011) > 0)
				locationHintsToPublish.Add(itemPerShopNode.Value.location);
		}
	}

	ap->CreateLocationHint(locationHintsToPublish);
}

void AApServerRandomizerSubsystem::AwardItem(int64 itemid, bool isFromServer) {
	if (ItemSchematics.Contains(itemid)) {
		SManager->GiveAccessToSchematic(ItemSchematics[itemid], nullptr);
	}
	else if (auto trapName = UApMappings::ItemIdToTrap.Find(itemid)) {
		trapSubsystem->SpawnTrap(*trapName, nullptr);
	}
	else if (mappingSubsystem->ApItems.Contains(itemid)) {
		if (mappingSubsystem->ApItems[itemid]->Type == EItemType::Item) {
			if (isFromServer && !IsRunningDedicatedServer()) { //TODO fix rewarding starter inventory to newly spawned clients on dead servers
				const AFGCharacterPlayer* player = GetLocalPlayer();
				fgcheck(player);
				UFGInventoryComponent* inventory = player->GetInventory();
				TSubclassOf<UFGItemDescriptor> itemClass = StaticCastSharedRef<FApItem>(mappingSubsystem->ApItems[itemid])->Class;
				int stackSize = UFGItemDescriptor::GetStackSize(itemClass);
				FInventoryStack stack = FInventoryStack(stackSize, itemClass);

				int numAdded = inventory->AddStack(stack, true);
				if (numAdded < stackSize)
					portalSubsystem->Enqueue(itemClass, stackSize - numAdded);
			}
			else {
				TSubclassOf<UFGItemDescriptor> itemClass = StaticCastSharedRef<FApItem>(mappingSubsystem->ApItems[itemid])->Class;
				int stackSize = UFGItemDescriptor::GetStackSize(itemClass);
				portalSubsystem->Enqueue(itemClass, stackSize);
			}
		}
		else if (mappingSubsystem->ApItems[itemid]->Type == EItemType::Schematic) {
			SManager->GiveAccessToSchematic(StaticCastSharedRef<FApSchematicItem>(mappingSubsystem->ApItems[itemid])->Class, nullptr);
		}
		else if (mappingSubsystem->ApItems[itemid]->Type == EItemType::Special) {
			ESpecialItemType specialType = StaticCastSharedRef<FApSpecialItem>(mappingSubsystem->ApItems[itemid])->SpecialType;
			switch (specialType) {
			case ESpecialItemType::Inventory3:
			case ESpecialItemType::Inventory6:
				unlockSubsystem->UnlockInventorySlots((specialType == ESpecialItemType::Inventory3) ? 3 : 6);
				break;
			case ESpecialItemType::Toolbelt1:
				unlockSubsystem->UnlockArmEquipmentSlots(1);
				break;
			case ESpecialItemType::InventoryUpload:
				unlockSubsystem->UnlockCentralStorageUploadSlots(5);
			}
		}
	}
}

void AApServerRandomizerSubsystem::CollectLocation(int64 itemId) {
	CheckedLocations.Enqueue(itemId);
}

void AApServerRandomizerSubsystem::HandleCheckedLocations() {
	TSet<int64> newCheckedLocations;
	int64 locationId;
	while (CheckedLocations.Dequeue(locationId)) {
		if (!schematicPatcher->IsCollected(locationId))
			newCheckedLocations.Add(locationId);
	}

	schematicPatcher->Server_Collect(newCheckedLocations);

	for (const TPair<TSubclassOf<UFGSchematic>, TArray<FApNetworkItem>>& itemsPerMilestone : locationsPerMilestone) {
		for (int index = 0; index < itemsPerMilestone.Value.Num(); index++) {
			const FApNetworkItem& networkItem = itemsPerMilestone.Value[index];

			if (newCheckedLocations.Contains(networkItem.location)) {
				if (!itemsPerMilestone.Value.ContainsByPredicate([this](FApNetworkItem& item) {
							return !schematicPatcher->IsCollected(item.location);	
				})) {
					SManager->GiveAccessToSchematic(itemsPerMilestone.Key, nullptr);
				}
			}
		}
	}

	for (const TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerMamNode : locationPerMamNode) {
		if (newCheckedLocations.Contains(itemPerMamNode.Value.location)) {
			SManager->GiveAccessToSchematic(itemPerMamNode.Key, nullptr);
		}
	}
	for (const TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerShopNode : locationPerShopNode) {
		if (newCheckedLocations.Contains(itemPerShopNode.Value.location)) {
			SManager->GiveAccessToSchematic(itemPerShopNode.Key, nullptr);
		}
	}

	for (const TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerHardDrive : locationPerHardDrive) {
		if (newCheckedLocations.Contains(itemPerHardDrive.Value.location)) {
			for (FHardDriveData& unclaimedHarddrive : RManager->mUnclaimedHardDriveData) {
				if (unclaimedHarddrive.PendingRewards.Contains(itemPerHardDrive.Key))
				{
					if (unclaimedHarddrive.PendingRewardsRerollsExecuted > 0
						&& !unclaimedHarddrive.PendingRewards.ContainsByPredicate([this](TSubclassOf<UFGSchematic>& harddriveOption) {
						return !schematicPatcher->IsCollected(FMath::RoundToInt(UFGSchematic::GetMenuPriority(harddriveOption)));
					})) {
						unclaimedHarddrive.PendingRewardsRerollsExecuted--;
					}
				}
			}

			SManager->GiveAccessToSchematic(itemPerHardDrive.Key, nullptr);
		}
	}
}

AFGCharacterPlayer* AApServerRandomizerSubsystem::GetLocalPlayer() {
	if (IsRunningDedicatedServer())
		return nullptr;

	const AFGPlayerController* playerController = UFGBlueprintFunctionLibrary::GetLocalPlayerController(GetWorld());
	return Cast<AFGCharacterPlayer>(playerController->GetControlledCharacter());
}

#pragma optimize("", on)

#undef LOCTEXT_NAMESPACE
