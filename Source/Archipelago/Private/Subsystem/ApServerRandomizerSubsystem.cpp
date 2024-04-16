#include "Subsystem/ApServerRandomizerSubsystem.h"

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
		slotDataSubsystem = AApSlotDataSubsystem::Get(world);
		fgcheck(slotDataSubsystem);
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

		//TODO: generatic AP Items can be totally hardcoded outside of the initialization phase
		UE_LOG(LogApServerRandomizerSubsystem, Display, TEXT("Generating schematics from AP Item IDs..."));
		for (TPair<int64, TSharedRef<FApItemBase>>& apitem : mappingSubsystem->ApItems) {
			if (apitem.Value->Type == EItemType::Recipe || apitem.Value->Type == EItemType::Building) {
				TSubclassOf<UFGSchematic> schematic =
					schematicPatcher->CreateSchematicBoundToItemId(apitem.Key, StaticCastSharedRef<FApRecipeItem>(apitem.Value));

				ItemSchematics.Add(apitem.Key, schematic);
			}
		}

		ap->SetItemReceivedCallback([this](int64 itemid, bool isFromServer) { ReceiveItem(itemid, isFromServer); });
		ap->SetLocationCheckedCallback([this](int64 itemid) { CollectLocation(itemid); });
		ap->SetDeathLinkReceivedCallback([this](FText message) { OnDeathLinkReceived(message); });

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
		if (!slotDataSubsystem->GetSlotData().hasLoadedSlotData)
			slotDataSubsystem->SetSlotDataJson(connectionInfo->GetSlotDataJson());

		if (scoutedLocations.Num() == 0 && slotDataSubsystem->GetSlotData().hasLoadedSlotData)
			ScoutArchipelagoItems();

		if (!mappingSubsystem->HasLoadedItemNameMappings())
			mappingSubsystem->InitializeAfterConnectingToAp();
	}

	if (!areRecipiesAndSchematicsInitialized
		&& scoutedLocations.Num() > 0
		&& slotDataSubsystem->GetSlotData().hasLoadedSlotData
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

	const FApSlotData slotData = slotDataSubsystem->GetSlotData();

	for (int tier = 1; tier <= slotData.hubLayout.Num(); tier++) {
		for (int milestone = 1; milestone <= maxMilestones; milestone++) {
			for (int slot = 1; slot <= maxSlots; slot++) {
				if (milestone <= slotData.hubLayout[tier - 1].Num() && slot <= slotData.numberOfChecksPerMilestone)
					locations.Add(hubBaseId);

				hubBaseId++;
			}
		}
	}

	//mam locations
	for (int l = 1338500; l <= 1338571; l++)
		locations.Add(l);

	//hardrive locations
	if (slotData.enableHardDriveGacha) {
		//for (int l = 1338600; l <= 1338699; l++)
		//	locations.Add(l);
	}

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

	TMap<FString, TTuple<bool, TSubclassOf<UFGSchematic>>> schematicsPerMilestone = TMap<FString, TTuple<bool, TSubclassOf<UFGSchematic>>>();
	TMap<int64, TSubclassOf<UFGSchematic>> schematicsPerLocation = TMap<int64, TSubclassOf<UFGSchematic>>();

	for (TSubclassOf<UFGSchematic>& schematic : hardcodedSchematics) {
		//UFGSchematic* schematicCDO = Cast<UFGSchematic>(schematic->GetDefaultObject());
		//UFGSchematic* schematicCDO = Cast<UFGSchematic>(schematic);
		int locationId = FMath::RoundToInt(UFGSchematic::GetMenuPriority(schematic));
		if (locationId > 1338000) {
			schematicsPerLocation.Add(locationId, schematic);
		}
	}

	for (const FApNetworkItem& location : scoutedLocations) {
		if (location.locationName.StartsWith("Hub")) {
			FString milestone = location.locationName.Left(location.locationName.Find(","));
			FString uniqueMilestone = milestone + TEXT("_") + connectionInfo->GetRoomSeed();

			if (!schematicsPerMilestone.Contains(milestone)) {
				TTuple<bool, TSubclassOf<UFGSchematic>> schematic = UApUtils::FindOrCreateClass(TEXT("/Archipelago/"), *uniqueMilestone, UFGSchematic::StaticClass());
				schematicsPerMilestone.Add(milestone, schematic);
			}

			if (!locationsPerMilestone.Contains(schematicsPerMilestone[milestone].Value)) {
				locationsPerMilestone.Add(schematicsPerMilestone[milestone].Value, TArray<FApNetworkItem>{ location });
			}
			else {
				locationsPerMilestone[schematicsPerMilestone[milestone].Value].Add(location);
			}
		}
		else if (location.location >= 1338500 && location.location <= 1338571 && schematicsPerLocation.Contains(location.location)) {
			locationPerMamNode.Add(schematicsPerLocation[location.location], location);
		}
		else if (location.location >= 1338600 && location.location <= 1338699 && schematicsPerLocation.Contains(location.location)) {
			locationPerHardDrive.Add(schematicsPerLocation[location.location], location);
		}
		else if (location.location >= 1338700 && location.location <= 1338709 && schematicsPerLocation.Contains(location.location)) {
			locationPerShopNode.Add(schematicsPerLocation[location.location], location);
		}
	}

	UE_LOG(LogApServerRandomizerSubsystem, Display, TEXT("Generating HUB milestones"));

	for (TPair<TSubclassOf<UFGSchematic>, TArray<FApNetworkItem>>& itemPerMilestone : locationsPerMilestone) {
		for (TPair<FString, TTuple<bool, TSubclassOf<UFGSchematic>>>& schematicAndName : schematicsPerMilestone) {
			if (itemPerMilestone.Key == schematicAndName.Value.Value) {
				if (!schematicAndName.Value.Key)
					schematicPatcher->InitializaHubSchematic(schematicAndName.Key, itemPerMilestone.Key, itemPerMilestone.Value);

				contentRegistry->RegisterSchematic(FName(TEXT("Archipelago")), itemPerMilestone.Key);

				break;
			}
		}
	}

	for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerMamNode : locationPerMamNode)
		schematicPatcher->InitializaSchematicForItem(itemPerMamNode.Key, itemPerMamNode.Value, false);
	for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerHardDrive : locationPerHardDrive)
		schematicPatcher->InitializaSchematicForItem(itemPerHardDrive.Key, itemPerHardDrive.Value, false);
	for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerMamNode : locationPerShopNode)
		schematicPatcher->InitializaSchematicForItem(itemPerMamNode.Key, itemPerMamNode.Value, true);

	areRecipiesAndSchematicsInitialized = true;
}

void AApServerRandomizerSubsystem::FinalizeInitialization() {
	//must be called before BeginPlay() and after connecting to AP as FreeSamples caches its exclude radioactive flag there
	if (!UpdateFreeSamplesConfiguration()) {
		UE_LOG(LogApServerRandomizerSubsystem, Error, TEXT("AApServerRandomizerSubsystem::FinalizeInitialization() Failed to update configuration of Free Samples"));
	}

	TArray<TSubclassOf<class UFGResearchTree>> researchTrees;
	RManager->GetAllResearchTrees(researchTrees);

	for (TSubclassOf<UFGResearchTree>& tree : researchTrees) {
		UFGResearchTree* treeCDO = Cast<UFGResearchTree>(tree->GetDefaultObject());
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

	SetMamEnhancerConfigurationHooks();
}

void AApServerRandomizerSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (portalSubsystem->IsInitialized())
		ProcessReceivedItems();

	if (connectionInfo->GetConnectionState() != EApConnectionState::Connected)
		return;

	HandleCheckedLocations();
	HandleDeathLink();

	if (phaseManager->GetGamePhase() > lastGamePhase) {
		OnAvaiableSchematicsChanged();
		lastGamePhase = phaseManager->GetGamePhase();
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

void AApServerRandomizerSubsystem::OnDeathLinkReceived(FText message) {
	UE_LOG(LogApServerRandomizerSubsystem, Display, TEXT("AApServerRandomizerSubsystem::OnDeathLinkReceived()"));
	
	instagib = true;
	ap->AddChatMessage(message, FLinearColor::Red);
}

void AApServerRandomizerSubsystem::HandleDeathLink() {
	if (IsRunningDedicatedServer())
		return; // TODO make deathlink work for dedicated servers

	AFGPlayerController* playerController = UFGBlueprintFunctionLibrary::GetLocalPlayerController(GetWorld());
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

		TSubclassOf<UDamageType> const damageType = TSubclassOf<UDamageType>(UDamageType::StaticClass());
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

	const FApSlotData slotData = slotDataSubsystem->GetSlotData();

	if (configRoot->SectionProperties.Contains("Equipment")) {
		UConfigPropertySection* EquipmentSection = Cast<UConfigPropertySection>(configRoot->SectionProperties["Equipment"]);
		if (EquipmentSection != nullptr && EquipmentSection->SectionProperties.Contains("Quantity")) {
			UConfigPropertyInteger* quantity = Cast<UConfigPropertyInteger>(EquipmentSection->SectionProperties["Quantity"]);
			if (quantity != nullptr)
				quantity->Value = slotData.freeSampleEquipment;
		}
	}

	if (configRoot->SectionProperties.Contains("Buildings")) {
		UConfigPropertySection* BuildingsSection = Cast<UConfigPropertySection>(configRoot->SectionProperties["Buildings"]);
		if (BuildingsSection != nullptr && BuildingsSection->SectionProperties.Contains("Quantity")) {
			UConfigPropertyInteger* quantity = Cast<UConfigPropertyInteger>(BuildingsSection->SectionProperties["Quantity"]);
			if (quantity != nullptr)
				quantity->Value = slotData.freeSampleBuildings;
		}
	}

	if (configRoot->SectionProperties.Contains("Parts")) {
		UConfigPropertySection* PartsSection = Cast<UConfigPropertySection>(configRoot->SectionProperties["Parts"]);
		if (PartsSection != nullptr && PartsSection->SectionProperties.Contains("Quantity")) {
			UConfigPropertyInteger* quantity = Cast<UConfigPropertyInteger>(PartsSection->SectionProperties["Quantity"]);
			if (quantity != nullptr)
				quantity->Value = slotData.freeSampleParts;
		}
	}

	if (configRoot->SectionProperties.Contains("Exclude")) {
		UConfigPropertySection* ExcludeSection = Cast<UConfigPropertySection>(configRoot->SectionProperties["Exclude"]);
		if (ExcludeSection != nullptr && ExcludeSection->SectionProperties.Contains("SkipRadioactive")) {
			UConfigPropertyBool* excludeRadioActive = Cast<UConfigPropertyBool>(ExcludeSection->SectionProperties["SkipRadioactive"]);
			if (excludeRadioActive != nullptr)
				excludeRadioActive->Value = !slotData.freeSampleRadioactive;
		}
	}

	ConfigManager->MarkConfigurationDirty(FreeSamplesConfigId);
	ConfigManager->FlushPendingSaves();
	ConfigManager->ReloadModConfigurations();

	return true;
}

void AApServerRandomizerSubsystem::SetMamEnhancerConfigurationHooks() {
	FConfigId MamEnhancerConfigId{ "MAMTips", "" };

	UConfigManager* ConfigManager = GetWorld()->GetGameInstance()->GetSubsystem<UConfigManager>();
	if (ConfigManager == nullptr) {
		return;
	}

	UConfigPropertySection* configRoot = ConfigManager->GetConfigurationRootSection(MamEnhancerConfigId);
	if (configRoot == nullptr) {
		return;
	}

	if (configRoot->SectionProperties.Contains("ShowHiddenNodesDetails")) {
		UConfigPropertyBool* showHidenNodeDetails = Cast<UConfigPropertyBool>(configRoot->SectionProperties["ShowHiddenNodesDetails"]);
		if (showHidenNodeDetails != nullptr)
			showHidenNodeDetails->OnPropertyValueChanged.AddDynamic(this, &AApServerRandomizerSubsystem::LockMamEnhancerSpoilerConfiguration);
	}

	if (configRoot->SectionProperties.Contains("MakeHiddenPrettyMode")) {
		UConfigPropertyInteger* hidenDisplayMode = Cast<UConfigPropertyInteger>(configRoot->SectionProperties["MakeHiddenPrettyMode"]);
		if (hidenDisplayMode != nullptr)
			hidenDisplayMode->OnPropertyValueChanged.AddDynamic(this, &AApServerRandomizerSubsystem::LockMamEnhancerSpoilerConfiguration);
	}

	LockMamEnhancerSpoilerConfiguration();
}


void AApServerRandomizerSubsystem::LockMamEnhancerSpoilerConfiguration() {
	FConfigId MamEnhancerConfigId{ "MAMTips", "" };

	UConfigManager* ConfigManager = GetWorld()->GetGameInstance()->GetSubsystem<UConfigManager>();
	if (ConfigManager == nullptr) {
		return;
	}

	UConfigPropertySection* configRoot = ConfigManager->GetConfigurationRootSection(MamEnhancerConfigId);
	if (configRoot == nullptr) {
		return;
	}

	bool dirty = false;

	if (configRoot->SectionProperties.Contains("ShowHiddenNodesDetails")) {
		UConfigPropertyBool* showHidenNodeDetails = Cast<UConfigPropertyBool>(configRoot->SectionProperties["ShowHiddenNodesDetails"]);
		if (showHidenNodeDetails != nullptr) {
			if (showHidenNodeDetails->Value) {
				showHidenNodeDetails->Value = false;

				dirty = true;
			}
		}
	}

	// 1 = Empty Gray Boxes (Base Game)
	// 2 = Show Question Mark Icons
	// 5 = Show "Who's That Jace?" Icons (Silly)
	if (configRoot->SectionProperties.Contains("MakeHiddenPrettyMode")) {
		UConfigPropertyInteger* hidenDisplayMode = Cast<UConfigPropertyInteger>(configRoot->SectionProperties["MakeHiddenPrettyMode"]);
		if (hidenDisplayMode != nullptr) {
			if (hidenDisplayMode->Value != 1 && hidenDisplayMode->Value != 2 && hidenDisplayMode->Value != 5) {
				hidenDisplayMode->Value = 1;

				dirty = true;
			}
		}
	}

	if (dirty) {
		ConfigManager->MarkConfigurationDirty(MamEnhancerConfigId);
		ConfigManager->FlushPendingSaves();
		ConfigManager->ReloadModConfigurations();
	}
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

	int maxAvailableTechTier = ((int)phaseManager->GetGamePhase() + 1) * 2;
	int currentPlayerSlot = connectionInfo->GetCurrentPlayerSlot();

	for (TPair<TSubclassOf<UFGSchematic>, TArray<FApNetworkItem>>& itemPerMilestone : locationsPerMilestone) {
		if (UFGSchematic::GetTechTier(itemPerMilestone.Key) <= maxAvailableTechTier) {
			for (FApNetworkItem item : itemPerMilestone.Value) {
				if (item.player != currentPlayerSlot && (item.flags & 0b011) > 0)
					locationHintsToPublish.Add(item.location);
			}
		}
	}

	if (SManager->IsSchematicPurchased(ItemSchematics[mappingSubsystem->GetMamItemId()])) {
		for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerMamNode : locationPerMamNode) {
			if (itemPerMamNode.Value.player != currentPlayerSlot
				&& (itemPerMamNode.Value.flags & 0b011) > 0
				&& RManager->CanResearchBeInitiated(itemPerMamNode.Key))

				locationHintsToPublish.Add(itemPerMamNode.Value.location);
		}
	}

	if (SManager->IsSchematicPurchased(ItemSchematics[mappingSubsystem->GetAwesomeShopItemId()])) {
		for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerShopNode : locationPerShopNode) {
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
				AFGCharacterPlayer* player = GetLocalPlayer();
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
			}
		}
	}
}

void AApServerRandomizerSubsystem::CollectLocation(int64 itemId) {
	CheckedLocations.Enqueue(itemId);
}

void AApServerRandomizerSubsystem::HandleCheckedLocations() {
	int64 location;

	//could use a while loop but we now handle only 1 per tick for perform reasons as this opperation is quite slow
	if (CheckedLocations.Dequeue(location)) {
		for (TPair<TSubclassOf<UFGSchematic>, TArray<FApNetworkItem>>& itemsPerMilestone : locationsPerMilestone) {
			for (int index = 0; index < itemsPerMilestone.Value.Num(); index++) {
				FApNetworkItem& networkItem = itemsPerMilestone.Value[index];
				if (networkItem.location == location) {
					UFGSchematic* schematic = Cast<UFGSchematic>(itemsPerMilestone.Key->GetDefaultObject());

					if (IsValid(schematic))
						schematicPatcher->Collect(schematic, index, networkItem);

					if (!itemsPerMilestone.Value.ContainsByPredicate([this](FApNetworkItem& item) {
								return !schematicPatcher->IsCollected(item.location);	})) {
						SManager->GiveAccessToSchematic(itemsPerMilestone.Key, nullptr);
					}
				}
			}
		}

		for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerMamNode : locationPerMamNode) {
			if (itemPerMamNode.Value.location == location) {
				UFGSchematic* schematic = Cast<UFGSchematic>(itemPerMamNode.Key->GetDefaultObject());

				if (IsValid(schematic))
					schematicPatcher->Collect(schematic, 0, itemPerMamNode.Value);

				SManager->GiveAccessToSchematic(itemPerMamNode.Key, nullptr);
			}
		}
		for (TPair<TSubclassOf<UFGSchematic>, FApNetworkItem>& itemPerShopNode : locationPerShopNode) {
			if (itemPerShopNode.Value.location == location) {
				UFGSchematic* schematic = Cast<UFGSchematic>(itemPerShopNode.Key->GetDefaultObject());

				if (IsValid(schematic))
					schematicPatcher->Collect(schematic, 0, itemPerShopNode.Value);

				SManager->GiveAccessToSchematic(itemPerShopNode.Key, nullptr);
			}
		}
	}
}

AFGCharacterPlayer* AApServerRandomizerSubsystem::GetLocalPlayer() {
	if (IsRunningDedicatedServer())
		return nullptr;

	AFGPlayerController* playerController = UFGBlueprintFunctionLibrary::GetLocalPlayerController(GetWorld());
	return Cast<AFGCharacterPlayer>(playerController->GetControlledCharacter());
}

#pragma optimize("", on)

#undef LOCTEXT_NAMESPACE
