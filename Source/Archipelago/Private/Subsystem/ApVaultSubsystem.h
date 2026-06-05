#pragma once

#include "CoreMinimal.h"
#include "ApGiftTraitsSubsystem.h"
#include "ApPlayerInfoSubsystem.h"

#include "ApSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "../AdditionalDepots/Public/AdditionalDepotsServerSubsystem.h"

#include "Subsystem/ModSubsystem.h"

#include "ApVaultSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApVaultSubsystem, Log, All);

#define SINGLE_ITEM_PREFIX TEXT("Single: ")
#define GLOBAL_VAULT_SLOT 0

USTRUCT()
struct FVaultItemMapping
{
	GENERATED_BODY()

	TMap<TSubclassOf<UFGItemDescriptor>, FString> ItemNameByClass;
};

UCLASS()
class AApVaultSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApVaultSubsystem();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;
	void MonitorVaultItems(int team, int slot);
	virtual void Tick(float dt) override;

	static AApVaultSubsystem* Get(const UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Ap Vault Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApVaultSubsystem* Get(const UObject* worldContext);

private:
	const FName globalVault = FName(TEXT("ApGlobalVault"));
	const FName personalVault = FName(TEXT("ApPersonalVault"));

	const float pollInterfall = 10.0f;

	bool apInitialized;

	AApSubsystem* ap;
	AApConnectionInfoSubsystem* connectionInfoSubsystem;
	AApMappingsSubsystem* mappingSubsystem;
	AApPlayerInfoSubsystem* playerInfoSubsystem;
	AApGiftTraitsSubsystem* giftTraitsSubsystem;
	AAdditionalDepotsServerSubsystem* additionalDepotsServerSubsystem;

	TMap<TSubclassOf<UFGItemDescriptor>, uint64> globalAdjustedItemAmounts;
	TMap<TSubclassOf<UFGItemDescriptor>, int64> pendingGlobalAdjustments;
	TMap<TSubclassOf<UFGItemDescriptor>, uint64> personalAdjustedItemAmounts;
	TMap<TSubclassOf<UFGItemDescriptor>, int64> pendingPersonalAdjustments;
	TMap<FString, int64> externalVaultAdjustments;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_VaultEnabledPlayers)
	TArray<FApPlayer> vaultEnabledPlayersReplicated;

	UPROPERTY(Replicated)
	FApPlayer currentPlayerGlobalVault;

	TSet<FApPlayer> vaults;
	TMap<FString, FVaultItemMapping> acceptedItemsPerGame;
	TMap<FString, TSubclassOf<UFGItemDescriptor>> lowerCaseToItemClassMapping;
	TMap<TSubclassOf<UFGItemDescriptor>, FString> itemClassToLowerCaseMapping;

	void UpdateItemAmount(const FString& key, const uint64* oldValue, const uint64* newValue, int slot);

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool IsInitialized() const { return apInitialized; };

	void Store(const FItemAmount& item, bool personal = false);
	void Store(const FItemAmount& item, const FApPlayer& vault);
	int32 Take(const FItemAmount& item, bool personal = false);

	bool TryGetSingleItem(TSubclassOf<UFGItemDescriptor> itemClass);

	UFUNCTION(BlueprintPure)
	TSet<FApPlayer> GetVaultEnabledPlayers() const;

	UFUNCTION(BlueprintPure)
	FApPlayer GetCurrentTeamGlobalVault() const;

	UFUNCTION(BlueprintPure)
	bool DoesPlayerAcceptVaultItems(const FApPlayer& player) const;

	UFUNCTION(BlueprintPure)
	TArray<TSubclassOf<UFGItemDescriptor>> GetAcceptedItemsPerVault(const FApPlayer& vault) const;

	bool CanSend(const FApPlayer& targetPlayer, const TSubclassOf<UFGItemDescriptor> itemClass);


private:
	void ParseVaultItemInfo(const FString& game, const TSharedRef<FJsonValue>& value);
	void AddPersonalVaults(const FString& game);

	void SetupPersonalVault();

	void UpdateDepotAmount(TSubclassOf<UFGItemDescriptor> item, bool personal) const;

	void SyncPendingVaultUpdates();

	void StoreToLocallyBufferedVault(const FItemAmount& item, bool personal = false);
	void StoreToExternalPlayerVault(const FItemAmount& item, const FApPlayer& vault);

	 void UpdatePendingAmount(TSubclassOf<UFGItemDescriptor> itemClass, bool personal, int64 changeInPendingAmount);

public: 
	UFUNCTION() //required for event hookup
	void OnRep_VaultEnabledPlayers();

	UFUNCTION()
	void OnItemAdded(FName ListIdentifier, TSubclassOf<UFGItemDescriptor> Class, int Amount, const AFGPlayerState* PlayerState);

	UFUNCTION()
	void OnItemRemoved(FName ListIdentifier, TSubclassOf<UFGItemDescriptor> Class, int Amount, const AFGPlayerState* PlayerState);
};
