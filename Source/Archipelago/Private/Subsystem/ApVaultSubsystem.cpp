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
}

void AApVaultSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOGFMT(LogApVaultSubsystem, Display, "AApVaultSubsystem::BeginPlay()");

	if (HasAuthority()) {
		UWorld* world = GetWorld();
		fgcheck(world != nullptr);

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

			MonitorVaultItems(team, slot);

			vaults.Add(FApPlayer(team, GLOBAL_VAULT_SLOT));

			//TODO add personal vaults
			TArray<FString> allGames = playerInfoSubsystem->GetAllGames();

			//TODO check if game has personal vault setup
			//TODO setup personal vault for Satisfactory

			vaultEnabledPlayersReplicated = vaults.Array();

			apInitialized = true;
		}
		else {
			return;
		}
	}

	//ProcessVaultStoreQueue();
}


void AApVaultSubsystem::MonitorVaultItems(int team, int slot)
{
	TArray<FString> keysToMonitor;

	for (const TPair<int64, TSharedRef<FApItemBase>>& ApItem : mappingSubsystem->ApItems)
	{
		if (ApItem.Value->Type != EItemType::Item)
			continue;
		//we should look at > 1339XXX for single items 
		TSharedRef<FApItem> itemInfo = StaticCastSharedRef<FApItem>(ApItem.Value);

		if (!mappingSubsystem->ItemIdToName.Contains(itemInfo->Id))
			continue;

		FString apItemName = GetItemName(itemInfo->Id);

		if (!apItemName.StartsWith(SINGLE_ITEM_PREFIX))
			continue;

		FString itemName = apItemName.RightChop(FString(SINGLE_ITEM_PREFIX).Len());

		FString globalVaultKey = FString::Format(TEXT("V{0}:{1}:{2}"), { team, GLOBAL_VAULT_SLOT, itemName });
		FString personalVaultKey = FString::Format(TEXT("V{0}:{1}:{2}"), { team, slot, itemName });

		keysToMonitor.Add(globalVaultKey);
		keysToMonitor.Add(personalVaultKey);
	}

	ap->MonitorDataStoreValue(keysToMonitor, AP_DataType::Int64, [&](AP_SetReply setReply) { UpdateItemAmount(setReply); });
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

TArray<FApPlayer> AApVaultSubsystem::GetVaultEnabledPlayers() const
{
	return vaults.Array();
}

FApPlayer AApVaultSubsystem::GetCurrentTeamGlobalVault() const
{
	if (!IsValid(connectionInfoSubsystem))
		return FApPlayer();

	int team = connectionInfoSubsystem->GetCurrentPlayerTeam();
	return FApPlayer(team, GLOBAL_VAULT_SLOT);
}


bool AApVaultSubsystem::DoesPlayerAcceptVaultItems(const FApPlayer& player) const
{
	return vaults.Contains(player);
}

FString AApVaultSubsystem::GetItemName(uint64 itemId) const {
	if (!mappingSubsystem->ItemIdToName.Contains(itemId))
	{
		UE_LOGFMT(LogApVaultSubsystem, Error, "AApVaultSubsystem::GetItemName({0}) invallid item id", itemId);
		return TEXT("");
	}

	FString apItemName = mappingSubsystem->ItemIdToName[itemId];

	if (!apItemName.StartsWith(SINGLE_ITEM_PREFIX))
	{
		//]LogApVaultSubsystem: Error: AApVaultSubsystem::GetItemName(1338823) item is not a single: Item1338823 from Satisfactory

		UE_LOGFMT(LogApVaultSubsystem, Error, "AApVaultSubsystem::GetItemName({0}) item is not a single: {1}", itemId, apItemName);
		return TEXT("");
	}

	return apItemName.RightChop(FString(SINGLE_ITEM_PREFIX).Len());
}

FString AApVaultSubsystem::GetItemName(TSubclassOf<UFGItemDescriptor> item) const {
	if (!mappingSubsystem->ItemClassToItemId.Contains(item))
	{
		UE_LOGFMT(LogApVaultSubsystem, Error, "AApVaultSubsystem::GetItemName({0}) unknown item", UFGItemDescriptor::GetItemName(item).ToString());
		return TEXT("");
	}

	return GetItemName(mappingSubsystem->ItemClassToItemId[item]);
}

void AApVaultSubsystem::OnRep_VaultEnabledPlayers()
{
	vaults.Empty();
	vaults.Append(vaultEnabledPlayersReplicated);
}

#pragma optimize("", on)

