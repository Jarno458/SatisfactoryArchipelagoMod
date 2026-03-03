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
			//gameItemInfoKeys.Remove(TEXT("Satisfactory")); //TODO

			ap->GetDataStorageJsonFields(gameItemInfoKeys, [this](FString key, FString value) {
				if (value.IsEmpty())
					return;

				FString game = key.RightChop(1);

				ParseVaultItemInfo(game, value);
				AddPersonalVaults(game);
				});

			SetupPersonalVault();

			vaultEnabledPlayersReplicated = vaults.Array();

			apInitialized = true;
		}
	}
	else {
		return;
	}

	//ProcessVaultStoreQueue();
}

void AApVaultSubsystem::MonitorVaultItems(int team, int slot)
{
	TArray<FString> keysToMonitor;

	mappingSubsystem->ItemIdToName.GenerateValueArray(keysToMonitor);

	for (const TPair<int64, FString>& ApItem : mappingSubsystem->ItemIdToName)
	{
		FString itemName = ApItem.Value;

		FString globalVaultKey = FString::Format(TEXT("V{0}:{1}:{2}"), { team, GLOBAL_VAULT_SLOT, itemName });
		FString personalVaultKey = FString::Format(TEXT("V{0}:{1}:{2}"), { team, slot, itemName });

		keysToMonitor.Add(globalVaultKey);
		keysToMonitor.Add(personalVaultKey);
	}

	ap->MonitorDataStoreValue(keysToMonitor, AP_DataType::Int64, [&](const AP_SetReply& setReply) { UpdateItemAmount(setReply); });
}

void AApVaultSubsystem::SetupPersonalVault() const
{
	static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

	TSharedRef<FJsonObject> jsonObject = MakeShareable(new FJsonObject());

	for (const TPair<TSubclassOf<UFGItemDescriptor>, FApTraitValues>& traitsPerItem : giftTraitsSubsystem->TraitsPerItem)
	{
		if (!mappingSubsystem->ItemClassToItemId.Contains(traitsPerItem.Key))
			UE_LOGFMT(LogApVaultSubsystem, Fatal, "AApVaultSubsystem::SetupPersonalVault() itemid not found for class {0}", UFGItemDescriptor::GetItemName(traitsPerItem.Key).ToString());

		int64 itemId = mappingSubsystem->ItemClassToItemId[traitsPerItem.Key];
		FString itemName = mappingSubsystem->ItemIdToName[itemId].ToLower();

		TSharedRef<FJsonObject> traitsJsonObject = MakeShareable(new FJsonObject());

		for (const TPair<EGiftTrait, float>& traitValues : traitsPerItem.Value.TraitsValues)
		{
			FString traitName = giftTraitEnum->GetNameStringByValue(static_cast<int64>(traitValues.Key));

			TSharedRef<FJsonValue> qualityFloat = MakeShareable(new FJsonValueNumber(traitValues.Value));

			traitsJsonObject->SetArrayField(traitName, { qualityFloat });
		}

		jsonObject->SetObjectField(itemName, traitsJsonObject);
	}

	FString json;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&json);
	FJsonSerializer::Serialize(jsonObject, Writer);

	ap->SetRawDataStorageValue("VSatisfactory", json);
}

void AApVaultSubsystem::UpdateItemAmount(const AP_SetReply& newData) {
	FString key = UApUtils::FStr(newData.key);

	int lastDoubleColanIndex;
	if (!key.FindLastChar(TEXT(':'), lastDoubleColanIndex))
		return;

	int firstDoubleColanIndex;
	if (!key.FindChar(TEXT(':'), firstDoubleColanIndex))
		return;

	bool isGlobal = key.Chr(firstDoubleColanIndex + 1) == TEXT("0");

	FString itemName = key.RightChop(lastDoubleColanIndex + 1);

	//FString intergerValueString = UApUtils::YankParseValueString(newData);

	//int256 newValueBig = UApUtils::Int256FromBigIntString(intergerValueString);

	// Might need the YankParseValueString logic from ApEnergyLinkSubsystem if we want to support unsigned int64 max values
	//int64* newValue = static_cast<int64*>(newData.value);

	 // Fuck still returns int64 not unsigned int64
	//int64 newValue = newValueBig.ToInt();

	int64 newValue = *static_cast<int64*>(newData.value);

	if (newValue < 0)
		newValue = 0;

	if (isGlobal) {
		if (globalItemAmounts.Contains(itemName)) {
			globalItemAmounts[itemName] = newValue;
		}
		else {
			globalItemAmounts.Add(itemName, newValue);
		}
	}
	else {
		if (personalItemAmounts.Contains(itemName)) {
			personalItemAmounts[itemName] = newValue;
		}
		else {
			personalItemAmounts.Add(itemName, newValue);
		}
	}
}

void AApVaultSubsystem::Store(const FItemAmount& item, const bool personal) {
	if (!apInitialized)
		return;

	FString itemName = GetItemName(item.ItemClass);
	if (itemName.IsEmpty())
		return;

	int team = connectionInfoSubsystem->GetCurrentPlayerTeam();
	int slot = GLOBAL_VAULT_SLOT;

	if (personal)
		slot = connectionInfoSubsystem->GetCurrentPlayerSlot();

	FString key = FString::Format(TEXT("V{0}:{1}:{2}"), { team, slot, itemName });

	if (personal) {
		if (personalItemAmounts.Contains(itemName)) {
			if (personalItemAmounts[itemName] + static_cast<uint64>(item.Amount) > INT64_MAX) {
				personalItemAmounts[itemName] = INT64_MAX;
			}
			else {
				personalItemAmounts[itemName] += item.Amount;
			}
		}
		else {
			personalItemAmounts.Add(itemName, item.Amount);
		}
	}
	else {
		if (globalItemAmounts.Contains(itemName)) {
			if (globalItemAmounts[itemName] + static_cast<uint64>(item.Amount) > INT64_MAX) {
				globalItemAmounts[itemName] = INT64_MAX;
			}
			else {
				globalItemAmounts[itemName] += item.Amount;
			}
		}
		else {
			globalItemAmounts.Add(itemName, item.Amount);
		}
	}

	ap->ModifyDataStorageInt64(key, item.Amount);
}

int32 AApVaultSubsystem::Take(const FItemAmount& item) {
	int32 requested = item.Amount;
	int32 personalYield = 0;
	int32 globalYield = 0;

	if (!apInitialized)
		return personalYield + globalYield;

	FString itemName = GetItemName(item.ItemClass);
	if (itemName.IsEmpty())
		return personalYield + globalYield;

	int team = connectionInfoSubsystem->GetCurrentPlayerTeam();
	int slot = connectionInfoSubsystem->GetCurrentPlayerSlot();

	if (personalItemAmounts.Contains(itemName) && requested > 0) {
		int64& personalAmount = personalItemAmounts[itemName];
		if (personalAmount < requested)
		{
			requested -= personalAmount;
			personalYield += personalAmount;
			personalAmount = 0;
		}
		else {
			personalYield += requested;
			personalAmount -= requested;
		}

		FString personalKey = FString::Format(TEXT("V{0}:{1}:{2}"), { team, slot, itemName });

		ap->ModifyDataStorageInt64(personalKey, personalYield);
	}

	if (globalItemAmounts.Contains(itemName) && requested > 0) {
		int64& globalAmount = globalItemAmounts[itemName];
		if (globalAmount < requested)
		{
			requested -= globalAmount;
			globalYield += globalAmount;
			globalAmount = 0;
		}
		else {
			globalYield += requested;
			globalAmount -= requested;
		}

		FString globalKey = FString::Format(TEXT("V{0}:{1}:{2}"), { team, GLOBAL_VAULT_SLOT, itemName });

		ap->ModifyDataStorageInt64(globalKey, globalYield);
	}

	return personalYield + globalYield;
}

TArray<FItemAmount> AApVaultSubsystem::GetItems(bool personal) const {
	TArray<FItemAmount> result;

	if (!apInitialized)
		return result;

	const TMap<FString, int64>& sourceMap = personal ? personalItemAmounts : globalItemAmounts;

	for (const TPair<FString, int64>& kv : sourceMap) {
		const FString& itemName = kv.Key;
		int64 amount64 = kv.Value;

		if (!mappingSubsystem)
			continue;

		if (!mappingSubsystem->NameToItemId.Contains(itemName))
			continue;

		int64 itemId = mappingSubsystem->NameToItemId[itemName];
		if (!mappingSubsystem->ApItems.Contains(itemId))
			continue;

		TSharedRef<FApItemBase> baseRef = mappingSubsystem->ApItems[itemId];
		if (StaticCastSharedRef<FApItemBase>(baseRef)->Type != EItemType::Item)
			continue;

		TSharedRef<FApItem> itemRef = StaticCastSharedRef<FApItem>(baseRef);
		TSubclassOf<UFGItemDescriptor> itemClass = itemRef->Class;
		if (!itemClass)
			continue;

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
	// or it can be object like {"item1": [0.3, 0.1], "otheritem": [0.6]}

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

#pragma optimize("", on)

