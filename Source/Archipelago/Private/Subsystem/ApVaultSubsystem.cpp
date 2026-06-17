#include "ApVaultSubsystem.h"

#include "Subsystem/ApSubsystem.h"
#include "ApUtils.h"
#include "Logging/StructuredLog.h"
#include "Misc/ScopeTryLock.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Net/UnrealNetwork.h"
#include "Subsystem/ApMappingsSubsystem.h"

#pragma optimize("", off)

/*
 * Design pattern for vaults:
 *
 * We can only Take from our own vault
 * Store and Take from our own vaults will update the corresponding Adjustments, and cause the depots to update
 *		(this might be bottleneck if 100 vaults update at 1200/min)
 * Store on another player their vault will create pending updates
 * Adjustments and other pending updates are synced with the server every interval
 *
 * When we receive updates values from the server, we check who updated the value,
 * if the update came from us, we know it was the sync from adjustments to the server,
 * so we subtract the change from our adjustments
 * either way we update the current amount with the value from the server
 *
 * NEW PLAN
 *
 * since we global/personal item amounts are only editied in the UpdateItemAmount callback from setnotify
 * we can instead store the whole adjusted amount in there, this allow us have the adjust amount readily available for other parts
 * such as GetItems and Take
 *
 * Portals can try to get a single item from the vault at rapid speeds
 *
 */

DEFINE_LOG_CATEGORY(LogApVaultSubsystem);

AApVaultSubsystem* AApVaultSubsystem::Get(const UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApVaultSubsystem* AApVaultSubsystem::Get(const UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApVaultSubsystem>();
}

AApVaultSubsystem::AApVaultSubsystem() {
	UE_LOGFMT(LogApVaultSubsystem, Display, "AApVaultSubsystem::AApVaultSubsystem()");

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.1f;
}

void AApVaultSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AApVaultSubsystem, vaultEnabledPlayersReplicated);
	DOREPLIFETIME(AApVaultSubsystem, currentPlayerGlobalVault);
}

void AApVaultSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOGFMT(LogApVaultSubsystem, Display, "AApVaultSubsystem::BeginPlay()");

	UWorld* world = GetWorld();
	fgcheck(world != nullptr);

	giftTraitsSubsystem = AApGiftTraitsSubsystem::Get(world);

	if (HasAuthority()) {
		ap = AApSubsystem::Get(world);
		connectionInfoSubsystem = AApConnectionInfoSubsystem::Get(world);
		mappingSubsystem = AApMappingsSubsystem::Get(world);
		playerInfoSubsystem = AApPlayerInfoSubsystem::Get(world);
		additionalDepotsServerSubsystem = AAdditionalDepotsServerSubsystem::Get(world);

		additionalDepotsServerSubsystem->OnItemAdded.AddDynamic(this, &AApVaultSubsystem::OnItemAdded);
		additionalDepotsServerSubsystem->OnItemRemoved.AddDynamic(this, &AApVaultSubsystem::OnItemRemoved);
	}
}

void AApVaultSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!HasAuthority())
		return;

	if (!apInitialized) {
		if (connectionInfoSubsystem->GetConnectionState() == EApConnectionState::Connected
			&& mappingSubsystem->HasLoadedItemNameMappings()
			&& playerInfoSubsystem->IsInitialized())
		{
			SetActorTickInterval(pollInterfall);

			for (const TPair<FString, int64>& ApItem : mappingSubsystem->NameToItemId)
			{
				FString itemNameLowerCase = ApItem.Key.ToLower();

				TSharedRef<FApItemBase> baseRef = mappingSubsystem->ApItems[ApItem.Value];
				if (StaticCastSharedRef<FApItemBase>(baseRef)->Type != EItemType::Item)
					continue;

				TSharedRef<FApItem> itemRef = StaticCastSharedRef<FApItem>(baseRef);
				TSubclassOf<UFGItemDescriptor> itemClass = itemRef->Class;
				if (!itemClass)
					continue;

				lowerCaseToItemClassMapping.Add(itemNameLowerCase, itemClass);
				itemClassToLowerCaseMapping.Add(itemClass, itemNameLowerCase);
			}

			int team = connectionInfoSubsystem->GetCurrentPlayerTeam();
			int slot = connectionInfoSubsystem->GetCurrentPlayerSlot();

			currentPlayerGlobalVault = FApPlayer(team, GLOBAL_VAULT_SLOT);

			vaults.Add(currentPlayerGlobalVault);

			MonitorVaultItems(team, slot);

			TArray<FString> allGames = playerInfoSubsystem->GetAllGames();

			TSet<FString> gameItemInfoKeys;
			for (const FString& game : allGames) {
				gameItemInfoKeys.Add(TEXT("V") + game);
			}
			gameItemInfoKeys.Remove(TEXT("VSatisfactory"));

			ap->GetDataStorageJsonFields(gameItemInfoKeys, [this](const FString& key, const TSharedRef<FJsonValue>& value) {
				if (value->IsNull())
					return;

				FString game = key.RightChop(1);
				if (game == TEXT("Satisfactory"))
					return;

				ParseVaultItemInfo(game, value);

				AddPersonalVaults(game);
				});

			SetupPersonalVault();
			AddPersonalVaults(TEXT("Satisfactory"));

			vaultEnabledPlayersReplicated = vaults.Array();

			apInitialized = true;
		}
	}
	else {
		SyncPendingVaultUpdates();
	}
}

void AApVaultSubsystem::MonitorVaultItems(int team, int slot)
{
	TArray<FString> keysToMonitor;

	for (const TPair<int64, FString>& ApItem : mappingSubsystem->ItemIdToName)
	{
		FString itemName = ApItem.Value.ToLower();

		FString globalVaultKey = FString::Format(TEXT("V{0}:{1}:{2}"), { team, GLOBAL_VAULT_SLOT, itemName });
		FString personalVaultKey = FString::Format(TEXT("V{0}:{1}:{2}"), { team, slot, itemName });

		keysToMonitor.Add(globalVaultKey);
		keysToMonitor.Add(personalVaultKey);
	}

	TArray<FString>truncatedKeysToMonitor;

	for (const FString& key : keysToMonitor) {
		if (key.Len() > 100)
			break;
		truncatedKeysToMonitor.Add(key);
	}

	ap->MonitorInt64DataStoreValue(truncatedKeysToMonitor, [this](const FString& key, const uint64* oldValue, const uint64* newValue, int slot) {
		UpdateItemAmount(key, oldValue, newValue, slot);
	});
}

void AApVaultSubsystem::UpdateItemAmount(const FString& key, const uint64* oldValue, const uint64* value, int slot) {
	if (value == nullptr)
		return;

	FString left, right;
	key.Split(TEXT(":"), &left, &right);
	right.Split(TEXT(":"), &left, &right);

	bool personal = left != TEXT("0");

	FString itemNameLowerCase = right;

	if (!lowerCaseToItemClassMapping.Contains(itemNameLowerCase))
		return;

	TSubclassOf<UFGItemDescriptor> itemClass = lowerCaseToItemClassMapping[itemNameLowerCase];

	uint64 newValue = *value;

	uint64& adjustedAmount = personal ? personalAdjustedItemAmounts.FindOrAdd(itemClass) : globalAdjustedItemAmounts.FindOrAdd(itemClass);
	int64& pendingAdjustment = personal ? pendingPersonalAdjustments.FindOrAdd(itemClass) : pendingGlobalAdjustments.FindOrAdd(itemClass);

	{
		FScopeLock lock(adjustmentsLocks.FindOrAdd(itemClass, MakeUnique<FCriticalSection>()).Get());

		// update pending adjustments if this callback is in response to our periodic sync
		if (slot == connectionInfoSubsystem->GetCurrentPlayerSlot())
		{
			const uint64 previousValue = oldValue ? *oldValue : 0;

			if (newValue == previousValue)
				return;

			int64 change;
			if (newValue >= previousValue)
			{
				const uint64 delta = newValue - previousValue;
				change = delta > static_cast<uint64>(INT64_MAX)
					? INT64_MAX
					: static_cast<int64>(delta);
			}
			else
			{
				const uint64 delta = previousValue - newValue;
				change = delta > static_cast<uint64>(INT64_MAX)
					? INT64_MIN
					: -static_cast<int64>(delta);
			}

			pendingAdjustment -= change;
		}

		if (pendingAdjustment < 0) {
			const uint64 decrease = pendingAdjustment == MIN_int64
				? static_cast<uint64>(INT64_MAX) + 1ULL
				: static_cast<uint64>(-pendingAdjustment);
			adjustedAmount = decrease >= newValue ? 0 : newValue - decrease;
		}
		else if (pendingAdjustment > 0) {
			const uint64 increase = static_cast<uint64>(pendingAdjustment);
			adjustedAmount = newValue > UINT64_MAX - increase ? UINT64_MAX : newValue + increase;
		}
		else {
			adjustedAmount = newValue;
		}
	}

	UpdateDepotAmount(itemClass, personal);
}

void AApVaultSubsystem::SetupPersonalVault()
{
	static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

	FVaultItemMapping itemMappings;

	TMap<FString, TMap<FString, float>> vaultItemTraitMapping;

	for (const TPair<TSubclassOf<UFGItemDescriptor>, FApTraitValues>& traitsPerItem : giftTraitsSubsystem->TraitsPerItem)
	{
		if (!mappingSubsystem->ItemClassToItemId.Contains(traitsPerItem.Key))
			UE_LOGFMT(LogApVaultSubsystem, Fatal, "AApVaultSubsystem::SetupPersonalVault() item-id not found for class {0}", UFGItemDescriptor::GetItemName(traitsPerItem.Key).ToString());

		int64 itemId = mappingSubsystem->ItemClassToItemId[traitsPerItem.Key];
		FString itemName = mappingSubsystem->ItemIdToName[itemId].ToLower();

		itemMappings.ItemNameByClass.Add(traitsPerItem.Key, itemName);

		TMap<FString, float> traitsMap;

		for (const TPair<EGiftTrait, float>& traitValues : traitsPerItem.Value.TraitsValues)
		{
			FString traitName = giftTraitEnum->GetNameStringByValue(static_cast<int64>(traitValues.Key));

			traitsMap.Add(traitName, traitValues.Value);
		}

		vaultItemTraitMapping.Add(itemName, traitsMap);
	}

	acceptedItemsPerGame.Add(TEXT("Satisfactory"), itemMappings);

	ap->SetVaultState(vaultItemTraitMapping);
}

void AApVaultSubsystem::UpdateDepotAmount(TSubclassOf<UFGItemDescriptor> item, bool personal) const
{
	uint64 adjustedAmount = personal ? personalAdjustedItemAmounts.FindRef(item) : globalAdjustedItemAmounts.FindRef(item);

	int32 adjustedAmount32 = static_cast<int32>(FMath::Clamp<uint64>(adjustedAmount, 0, INT32_MAX));

	FName vault = personal ? personalVault : globalVault;

	additionalDepotsServerSubsystem->SetItem(vault, item, adjustedAmount32);
}

void AApVaultSubsystem::SyncPendingVaultUpdates()
{
	int team = connectionInfoSubsystem->GetCurrentPlayerTeam();
	int slot = connectionInfoSubsystem->GetCurrentPlayerSlot();

	for (const TPair<TSubclassOf<UFGItemDescriptor>, int64>& globalAdjustment : pendingGlobalAdjustments) {
		if (!itemClassToLowerCaseMapping.Contains(globalAdjustment.Key))
			continue;

		FString itemName = itemClassToLowerCaseMapping[globalAdjustment.Key];
		FString key = FString::Format(TEXT("V{0}:{1}:{2}"), { team, GLOBAL_VAULT_SLOT, itemName });

		externalVaultAdjustments.Add(key, globalAdjustment.Value);
	}

	for (const TPair<TSubclassOf<UFGItemDescriptor>, int64>& personalAdjustment : pendingPersonalAdjustments) {
		if (!itemClassToLowerCaseMapping.Contains(personalAdjustment.Key))
			continue;

		FString itemName = itemClassToLowerCaseMapping[personalAdjustment.Key];
		FString key = FString::Format(TEXT("V{0}:{1}:{2}"), { team, slot, itemName });

		externalVaultAdjustments.Add(key, personalAdjustment.Value);
	}

	ap->ModifyDataStorageInt64(externalVaultAdjustments);

	externalVaultAdjustments.Empty();
}

void AApVaultSubsystem::Store(const FItemAmount& item, const FApPlayer& vault)
{
	if (!apInitialized || item.Amount == 0 || !vault.IsValid() || !itemClassToLowerCaseMapping.Contains(item.ItemClass))
		return;

	if (vault.Team == connectionInfoSubsystem->GetCurrentPlayerTeam())
	{
		if (vault.Slot == connectionInfoSubsystem->GetCurrentPlayerSlot())
		{
			StoreToLocallyBufferedVault(item, true);
			return;
		}

		if (vault.Slot == GLOBAL_VAULT_SLOT)
		{
			StoreToLocallyBufferedVault(item, false);
			return;
		}
	}

	StoreToExternalPlayerVault(item, vault);
}

void AApVaultSubsystem::Store(const FItemAmount& item, const bool personal) {
	StoreToLocallyBufferedVault(item, personal);
}

void AApVaultSubsystem::StoreToLocallyBufferedVault(const FItemAmount& item, bool personal)
{
	if (!apInitialized || item.Amount <= 0 || !IsValid(item.ItemClass) || !itemClassToLowerCaseMapping.Contains(item.ItemClass))
		return;

	{
		FScopeLock lock(adjustmentsLocks.FindOrAdd(item.ItemClass, MakeUnique<FCriticalSection>()).Get());

		UpdatePendingAmount(item.ItemClass, personal, item.Amount);
	}

	UpdateDepotAmount(item.ItemClass, personal);
}

void AApVaultSubsystem::StoreToExternalPlayerVault(const FItemAmount& item, const FApPlayer& vault)
{
	FString itemName = itemClassToLowerCaseMapping[item.ItemClass];
	FString key = FString::Format(TEXT("V{0}:{1}:{2}"), { vault.Team, vault.Slot, itemName });

	externalVaultAdjustments.FindOrAdd(key) += item.Amount;
}

void AApVaultSubsystem::UpdatePendingAmount(TSubclassOf<UFGItemDescriptor> itemClass, bool personal, int64 changeInPendingAmount)
{
	uint64& adjustedAmount = personal ? personalAdjustedItemAmounts.FindOrAdd(itemClass) : globalAdjustedItemAmounts.FindOrAdd(itemClass);
	int64& pendingAdjustment = personal ? pendingPersonalAdjustments.FindOrAdd(itemClass) : pendingGlobalAdjustments.FindOrAdd(itemClass);

	{
		FScopeLock lock(adjustmentsLocks.FindOrAdd(itemClass, MakeUnique<FCriticalSection>()).Get());

		uint64 normalizedAmount = adjustedAmount;
		if (pendingAdjustment < 0) {
			const uint64 decrease = pendingAdjustment == MIN_int64
				? static_cast<uint64>(INT64_MAX) + 1ULL
				: static_cast<uint64>(-pendingAdjustment);
			normalizedAmount = decrease >= adjustedAmount ? 0 : adjustedAmount - decrease;
		}
		else if (pendingAdjustment > 0) {
			const uint64 increase = static_cast<uint64>(pendingAdjustment);
			normalizedAmount = adjustedAmount > UINT64_MAX - increase ? UINT64_MAX : adjustedAmount + increase;
		}

		if (changeInPendingAmount > 0)
		{
			if (pendingAdjustment > INT64_MAX - changeInPendingAmount)
				pendingAdjustment = INT64_MAX;
			else
				pendingAdjustment += changeInPendingAmount;
		}
		else if (changeInPendingAmount < 0)
		{
			if (pendingAdjustment < INT64_MIN - changeInPendingAmount)
				pendingAdjustment = INT64_MIN;
			else
				pendingAdjustment += changeInPendingAmount;
		}

		if (pendingAdjustment < 0) {
			const uint64 decrease = pendingAdjustment == MIN_int64
				? static_cast<uint64>(INT64_MAX) + 1ULL
				: static_cast<uint64>(-pendingAdjustment);
			adjustedAmount = decrease >= normalizedAmount ? 0 : normalizedAmount - decrease;
		}
		else if (pendingAdjustment > 0) {
			const uint64 increase = static_cast<uint64>(pendingAdjustment);
			adjustedAmount = normalizedAmount > UINT64_MAX - increase ? UINT64_MAX : normalizedAmount + increase;
		}
		else {
			adjustedAmount = normalizedAmount;
		}
	}
}

int32 AApVaultSubsystem::Take(const FItemAmount& item, bool personal) {
	int32 requested = item.Amount;
	uint64 yield = 0;

	if (!apInitialized || requested <= 0 || !IsValid(item.ItemClass) || !itemClassToLowerCaseMapping.Contains(item.ItemClass))
		return yield;

	uint64 adjustedAmount = personal ? personalAdjustedItemAmounts.FindRef(item.ItemClass) : globalAdjustedItemAmounts.FindRef(item.ItemClass);
	if (adjustedAmount < requested)
		yield = adjustedAmount;
	else
		yield = requested;

	{
		FScopeLock lock(adjustmentsLocks.FindOrAdd(item.ItemClass, MakeUnique<FCriticalSection>()).Get());

		//TODO UpdatePendingAmount should likely return the amound changed
		UpdatePendingAmount(item.ItemClass, personal, -static_cast<int64>(yield));
	}

	UpdateDepotAmount(item.ItemClass, personal);

	return static_cast<int32>(FMath::Clamp<uint64>(yield, 0, INT32_MAX));
}

bool AApVaultSubsystem::TryGetSingleItem(TSubclassOf<UFGItemDescriptor> itemClass)
{
	bool hasResult = false;

	FScopeTryLock lock(adjustmentsLocks.FindOrAdd(itemClass, MakeUnique<FCriticalSection>()).Get());

	if (lock.IsLocked() && personalAdjustedItemAmounts.FindRef(itemClass) > 0)
	{
		UpdatePendingAmount(itemClass, true, -1);

		hasResult = true;
	}

	if (hasResult)
		UpdateDepotAmount(itemClass, true);

	return hasResult;
}

TSet<FApPlayer> AApVaultSubsystem::GetVaultEnabledPlayers() const
{
	return vaults;
}

FApPlayer AApVaultSubsystem::GetCurrentTeamGlobalVault() const
{
	return currentPlayerGlobalVault;
}


bool AApVaultSubsystem::DoesPlayerAcceptVaultItems(const FApPlayer& player) const
{
	return vaults.Contains(player);
}

TArray<TSubclassOf<UFGItemDescriptor>> AApVaultSubsystem::GetAcceptedItemsPerVault(const FApPlayer& vault) const
{
	if (!vault.IsValid())
		return TArray<TSubclassOf<UFGItemDescriptor>>();

	if (vault.Slot == GLOBAL_VAULT_SLOT)
		return giftTraitsSubsystem->GetAllItems();

	FString game = playerInfoSubsystem->GetPlayerGame(vault);

	if (!acceptedItemsPerGame.Contains(game))
		return TArray<TSubclassOf<UFGItemDescriptor>>();

	TArray<TSubclassOf<UFGItemDescriptor>> result;
	acceptedItemsPerGame[game].ItemNameByClass.GenerateKeyArray(result);

	return result;
}

bool AApVaultSubsystem::CanSend(const FApPlayer& vault, const TSubclassOf<UFGItemDescriptor> itemClass)
{
	if (!vaults.Contains(vault))
		return false;

	if (vault.Slot == GLOBAL_VAULT_SLOT)
		return true;

	//maybe cache accepted items per vault
	FString game = playerInfoSubsystem->GetPlayerGame(vault);

	if (!acceptedItemsPerGame.Contains(game))
		return false;

	return acceptedItemsPerGame[game].ItemNameByClass.Contains(itemClass);
}

TArray<TSubclassOf<UFGItemDescriptor>> AApVaultSubsystem::GetItemsStoredInPersonalVault() const
{
	TArray<TSubclassOf<UFGItemDescriptor>> items;

	for (const FItemAmount& item : additionalDepotsServerSubsystem->GetItems(personalVault))
		items.Add(item.ItemClass);

	return items;
}

TArray<TSubclassOf<UFGItemDescriptor>> AApVaultSubsystem::GetAllAcceptedItems() const
{
	return giftTraitsSubsystem->GetAllItems();
}

void AApVaultSubsystem::ParseVaultItemInfo(const FString& game, const TSharedRef<FJsonValue>& parsedJson)
{
	static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

	//parsedJson can be string array like ["item1", "other item"]
	// or it can be object like {"item1": { "TraitA": 0.3}, "other item": { "TraitB": 0.6, "TraitC": 1}}

	FVaultItemMapping itemMappings;

	if (parsedJson->Type == EJson::Array)
	{
		TArray<TSharedPtr<FJsonValue>> array = parsedJson->AsArray();

		TSet<FString> acceptedItemNames;
		for (const TSharedPtr<FJsonValue>& itemInfo : array) {
			FString itemName;
			if (itemInfo->TryGetString(itemName))
				acceptedItemNames.Add(itemName);
		}

		for (const TPair<int64, FString>& ApItem : mappingSubsystem->ItemIdToName) {
			if (acceptedItemNames.Contains(ApItem.Value)) {
				if (const TSubclassOf<UFGItemDescriptor>* itemClass = mappingSubsystem->ItemClassToItemId.FindKey(ApItem.Key)) {
					itemMappings.ItemNameByClass.Add(*itemClass, ApItem.Value);
				}
			}
		}
	}
	else if (parsedJson->Type == EJson::Object)
	{
		TSharedPtr<FJsonObject> jsonObject = parsedJson->AsObject();

		for (const TPair<FString, TSharedPtr<FJsonValue>>& itemInfo : jsonObject->Values)
		{
			// can only map on name if no traits are supplied, this might be out of the protocol spec
			if (itemInfo.Value->Type != EJson::Null)
			{
				if (const int64* itemId = mappingSubsystem->ItemIdToName.FindKey(itemInfo.Key))
				{
					if (const TSubclassOf<UFGItemDescriptor>* itemClass = mappingSubsystem->ItemClassToItemId.FindKey(*itemId)) {
						itemMappings.ItemNameByClass.Add(*itemClass, itemInfo.Key);
					}
				}
			}
			if (itemInfo.Value->Type != EJson::Object)
			{
				TSharedPtr<FJsonObject> traitsJsonObject = itemInfo.Value->AsObject();

				TArray<FApGiftTrait> traits;

				for (const TPair<FString, TSharedPtr<FJsonValue>>& traitsJson : traitsJsonObject->Values)
				{
					const int64 enumValue = giftTraitEnum->GetValueByNameString(traitsJson.Key);
					if (enumValue == INDEX_NONE)
						continue;

					FApGiftTrait trait;
					trait.Trait = static_cast<EGiftTrait>(enumValue);
					trait.Duration = 1.0f;

					if (!traitsJson.Value->TryGetNumber(trait.Quality))
						trait.Quality = 1.0f;

					traits.Add(trait);
				}

				if (traits.Num() == 0) { //we got no matches so we cant handle this item, unless we match on name
					if (const int64* itemId = mappingSubsystem->ItemIdToName.FindKey(itemInfo.Key))
					{
						if (const TSubclassOf<UFGItemDescriptor>* itemClass = mappingSubsystem->ItemClassToItemId.FindKey(*itemId)) {
							itemMappings.ItemNameByClass.Add(*itemClass, itemInfo.Key);
						}
					}
				}
				else if (const TSubclassOf<UFGItemDescriptor> satisfactoryItem = giftTraitsSubsystem->TryGetItemClassByTraits(traits))
					itemMappings.ItemNameByClass.Add(satisfactoryItem, itemInfo.Key);
			}
		}
	}

	if (!itemMappings.ItemNameByClass.IsEmpty())
		acceptedItemsPerGame.Add(game, itemMappings);
}

void AApVaultSubsystem::AddPersonalVaults(const FString& game)
{
	//TODO this is bad as it re-replicated for each game it receives players for
	vaults.Append(playerInfoSubsystem->GetAllPlayersPlayingGame(game));
	vaultEnabledPlayersReplicated = vaults.Array();
}

void AApVaultSubsystem::OnRep_VaultEnabledPlayers()
{
	vaults.Empty();
	vaults.Append(vaultEnabledPlayersReplicated);
}

void AApVaultSubsystem::OnItemAdded(FName ListIdentifier, TSubclassOf<UFGItemDescriptor> Class, int Amount, const AFGPlayerState* PlayerState)
{
	FItemAmount itemAmount;
	itemAmount.ItemClass = Class;
	itemAmount.Amount = Amount;

	Store(itemAmount, ListIdentifier == personalVault);
}

void AApVaultSubsystem::OnItemRemoved(FName ListIdentifier, TSubclassOf<UFGItemDescriptor> Class, int Amount, const AFGPlayerState* PlayerState)
{
	FItemAmount itemAmount;
	itemAmount.ItemClass = Class;
	itemAmount.Amount = Amount;

	Take(itemAmount, ListIdentifier == personalVault);
}

#pragma optimize("", on)

