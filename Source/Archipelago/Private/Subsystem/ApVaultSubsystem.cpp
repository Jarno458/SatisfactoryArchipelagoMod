#include "ApVaultSubsystem.h"

#include "ApSubsystem.h"
#include "ApUtils.h"
#include "StructuredLog.h"
#include "SubsystemActorManager.h"
#include "UnrealNetwork.h"
#include "Subsystem/ApMappingsSubsystem.h"

#pragma optimize("", off)

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

AApVaultSubsystem::AApVaultSubsystem() : Super() {
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
			}

			int team = connectionInfoSubsystem->GetCurrentPlayerTeam();
			int slot = connectionInfoSubsystem->GetCurrentPlayerSlot();

			currentPlayerGlobalVault = FApPlayer(team, GLOBAL_VAULT_SLOT);

			vaults.Add(currentPlayerGlobalVault);

			MonitorVaultItems(team, slot);

			/* //TODO cross game implementation disabled due to crashes
			TArray<FString> allGames = playerInfoSubsystem->GetAllGames();

			TSet<FString> gameItemInfoKeys;
			for (const FString& game : allGames) {
				gameItemInfoKeys.Add(TEXT("V") + game);
			}
			gameItemInfoKeys.Remove(TEXT("VSatisfactory"));

			ap->GetDataStorageJsonFields(gameItemInfoKeys, [this](FString key, FString value) {
				if (value.IsEmpty())
					return;

				FString game = key.RightChop(1);

				if (game == TEXT("Satisfactory"))
					return;

				ParseVaultItemInfo(game, value);
				AddPersonalVaults(game);
			});*/

			SetupPersonalVault();
			AddPersonalVaults(TEXT("Satisfactory"));

			vaultEnabledPlayersReplicated = vaults.Array();

			apInitialized = true;
		}
	}
	else {
		//ProcessVaultStoreQueue();
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
	int lastDoubleColanIndex;
	if (!key.FindLastChar(TEXT(':'), lastDoubleColanIndex))
		return;

	FString left, right;
	key.Split(TEXT(":"), &left, &right);
	right.Split(TEXT(":"), &left, &right);

	bool isGlobal = left == TEXT("0");

	FString itemNameLowerCase = key.RightChop(lastDoubleColanIndex + 1);

	if (!lowerCaseToItemClassMapping.Contains(itemNameLowerCase))
		return;

	TSubclassOf<UFGItemDescriptor> itemClass = lowerCaseToItemClassMapping[itemNameLowerCase];

	if (value == nullptr)
		return;

	int64 newValue = *value;

	if (newValue < 0)
		newValue = 0;

	if (isGlobal) {
		if (globalItemAmounts.Contains(itemClass)) {
			globalItemAmounts[itemClass] = newValue;
		}
		else {
			globalItemAmounts.Add(itemClass, newValue);
		}

		UpdateDepotAmount(globalVault, itemClass, newValue);
	}
	else {
		if (personalItemAmounts.Contains(itemClass)) {
			personalItemAmounts[itemClass] = newValue;
		}
		else {
			personalItemAmounts.Add(itemClass, newValue);
		}

		UpdateDepotAmount(personalVault, itemClass, newValue);
	}
}

void AApVaultSubsystem::SetupPersonalVault()
{
	static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

	FVaultItemMapping itemMappings;

	TSharedRef<FJsonObject> jsonObject = MakeShareable(new FJsonObject());

	for (const TPair<TSubclassOf<UFGItemDescriptor>, FApTraitValues>& traitsPerItem : giftTraitsSubsystem->TraitsPerItem)
	{
		if (!mappingSubsystem->ItemClassToItemId.Contains(traitsPerItem.Key))
			UE_LOGFMT(LogApVaultSubsystem, Fatal, "AApVaultSubsystem::SetupPersonalVault() itemid not found for class {0}", UFGItemDescriptor::GetItemName(traitsPerItem.Key).ToString());

		int64 itemId = mappingSubsystem->ItemClassToItemId[traitsPerItem.Key];
		FString itemName = mappingSubsystem->ItemIdToName[itemId].ToLower();

		itemMappings.ItemNameByClass.Add(traitsPerItem.Key, itemName);

		TSharedRef<FJsonObject> traitsJsonObject = MakeShareable(new FJsonObject());

		for (const TPair<EGiftTrait, float>& traitValues : traitsPerItem.Value.TraitsValues)
		{
			FString traitName = giftTraitEnum->GetNameStringByValue(static_cast<int64>(traitValues.Key));

			double rounded = FMath::RoundToDouble(traitValues.Value * 1000.0) / 1000.0; //round to 3 decimals
			FString chopped = FString::Printf(TEXT("%.3f"), rounded); // cut off trailing decimals
			double minified = FCString::Atod(*chopped);

			traitsJsonObject->SetNumberField(traitName, minified);
		}

		jsonObject->SetObjectField(itemName, traitsJsonObject);
	}

	acceptedItemsPerGame.Add(TEXT("Satisfactory"), itemMappings);

	/* //TODO cross game implementation disabled due to crashes
	FString json;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&json);
	FJsonSerializer::Serialize(jsonObject, Writer);


	ap->SetRawDataStorageValue("VSatisfactory", json);
	*/
}

void AApVaultSubsystem::UpdateDepotAmount(FName depot, TSubclassOf<UFGItemDescriptor> item, int64 amount) const
{
	int32 newAmount = static_cast<int32>(FMath::Clamp<int64>(amount, 0, INT32_MAX));

	additionalDepotsServerSubsystem->SetItem(depot, item, newAmount);
}

void AApVaultSubsystem::Store(const FItemAmount& item, const FApPlayer& vault)
{
	if (!apInitialized || item.Amount == 0 || !vault.IsValid())
		return;

	if (vault.Team == connectionInfoSubsystem->GetCurrentPlayerTeam())
	{
		if (vault.Slot == connectionInfoSubsystem->GetCurrentPlayerSlot())
		{
			Store(item, true);
			return;
		}

		if (vault.Slot == GLOBAL_VAULT_SLOT)
		{
			Store(item, false);
			return;
		}
	}

	FString itemName = GetItemName(item.ItemClass);
	if (itemName.IsEmpty())
		return;

	FString key = FString::Format(TEXT("V{0}:{1}:{2}"), { vault.Team, vault.Slot, itemName.ToLower() });

	ap->ModifyDataStorageInt64(key, item.Amount);
}

void AApVaultSubsystem::Store(const FItemAmount& item, const bool personal) {
	if (!apInitialized || item.Amount == 0)
		return;

	FString itemName = GetItemName(item.ItemClass);
	if (itemName.IsEmpty())
		return;

	int team = connectionInfoSubsystem->GetCurrentPlayerTeam();
	int slot = GLOBAL_VAULT_SLOT;

	if (personal)
		slot = connectionInfoSubsystem->GetCurrentPlayerSlot();

	FString key = FString::Format(TEXT("V{0}:{1}:{2}"), { team, slot, itemName.ToLower() });

	if (personal) {
		if (personalItemAmounts.Contains(item.ItemClass)) {
			if (personalItemAmounts[item.ItemClass] + static_cast<uint64>(item.Amount) > INT64_MAX) {
				personalItemAmounts[item.ItemClass] = INT64_MAX;
			}
			else {
				personalItemAmounts[item.ItemClass] += item.Amount;
			}
		}
		else {
			personalItemAmounts.Add(item.ItemClass, item.Amount);
		}
	}
	else {
		if (globalItemAmounts.Contains(item.ItemClass)) {
			if (globalItemAmounts[item.ItemClass] + static_cast<uint64>(item.Amount) > INT64_MAX) {
				globalItemAmounts[item.ItemClass] = INT64_MAX;
			}
			else {
				globalItemAmounts[item.ItemClass] += item.Amount;
			}
		}
		else {
			globalItemAmounts.Add(item.ItemClass, item.Amount);
		}
	}

	ap->ModifyDataStorageInt64(key, item.Amount);
}

int32 AApVaultSubsystem::Take(const FItemAmount& item, bool personal) {
	int32 requested = item.Amount;
	int32 yield = 0;

	if (!apInitialized || requested <= 0)
		return yield;

	FString itemName = GetItemName(item.ItemClass);
	if (itemName.IsEmpty())
		return yield;

	int team = connectionInfoSubsystem->GetCurrentPlayerTeam();
	int slot = GLOBAL_VAULT_SLOT;

	if (personal)
		slot = connectionInfoSubsystem->GetCurrentPlayerSlot();

	FString key = FString::Format(TEXT("V{0}:{1}:{2}"), { team, slot, itemName.ToLower() });

	if (personal)
	{
		if (personalItemAmounts.Contains(item.ItemClass) && requested > 0) {
			int64& personalAmount = personalItemAmounts[item.ItemClass];
			if (personalAmount < requested)
			{
				yield = personalAmount;
				personalAmount = 0;
			}
			else {
				yield = requested;
				personalAmount -= requested;
			}
		}
	}
	else
	{
		if (globalItemAmounts.Contains(item.ItemClass) && requested > 0) {
			int64& globalAmount = globalItemAmounts[item.ItemClass];
			if (globalAmount < requested)
			{
				yield = globalAmount;
				globalAmount = 0;
			}
			else {
				yield = requested;
				globalAmount -= requested;
			}

		}
	}

	ap->ModifyDataStorageInt64(key, yield);

	return yield;
}

TArray<FItemAmount> AApVaultSubsystem::GetItems(bool personal) const {
	TArray<FItemAmount> result;

	if (!apInitialized)
		return result;

	const TMap<TSubclassOf<UFGItemDescriptor>, int64>& sourceMap = personal ? personalItemAmounts : globalItemAmounts;

	for (const TPair<TSubclassOf<UFGItemDescriptor>, int64>& kv : sourceMap) {
		const TSubclassOf<UFGItemDescriptor>& itemClass = kv.Key;

		int64 amount64 = kv.Value;

		FItemAmount entry(itemClass, static_cast<int32>(FMath::Clamp<int64>(amount64, 0, INT32_MAX)));
		result.Add(entry);
	}

	return result;
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

TArray<TSubclassOf<UFGItemDescriptor>> AApVaultSubsystem::GetAcceptedItemsPerVault(FApPlayer vault) const
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

FString AApVaultSubsystem::GetItemName(uint64 itemId) const {
	return mappingSubsystem->ItemIdToName[itemId];
}

FString AApVaultSubsystem::GetItemName(TSubclassOf<UFGItemDescriptor> item) const {
	if (!mappingSubsystem->ItemClassToItemId.Contains(item))
	{
		UE_LOGFMT(LogApVaultSubsystem, Error, "AApVaultSubsystem::GetItemName({0}) unknown item", UFGItemDescriptor::GetItemName(item).ToString());
		return TEXT("");
	}

	return GetItemName(mappingSubsystem->ItemClassToItemId[item]);
}

void AApVaultSubsystem::ParseVaultItemInfo(FString game, FString itemInfoString)
{
	static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

	const TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*itemInfoString);

	TSharedPtr<FJsonValue> parsedJson;
	FJsonSerializer::Deserialize(reader, parsedJson);

	if (!parsedJson.IsValid())
		return;

	//parsedJson can be string array like ["item1", "otheritem"]
	// or it can be object like {"item1": { "TraitA": 0.3}, "otheritem": { "TraitB": 0.6, "TraitC": 1}}

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

void AApVaultSubsystem::AddPersonalVaults(FString game)
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

