#pragma once

#include "CoreMinimal.h"
#include "ApGiftTraitsSubsystem.h"
#include "ApPlayerInfoSubsystem.h"

#include "ApSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"

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
	const float pollInterfall = 10.0f;

	bool apInitialized;
	int itemNameInVaultKeyStartPosition;

	AApSubsystem* ap;
	AApConnectionInfoSubsystem* connectionInfoSubsystem;
	AApMappingsSubsystem* mappingSubsystem;
	AApPlayerInfoSubsystem* playerInfoSubsystem;
	AApGiftTraitsSubsystem* giftTraitsSubsystem;

	TMap<FString, int64> globalItemAmounts;
	TMap<FString, int64> personalItemAmounts;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_VaultEnabledPlayers)
	TArray<FApPlayer> vaultEnabledPlayersReplicated;

	UPROPERTY(Replicated)
	FApPlayer currentPlayerGlobalVault;

	TSet<FApPlayer> vaults;
	TMap<FString, FVaultItemMapping> acceptedItemsPerGame;

	void UpdateItemAmount(const AP_SetReply& newData);

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool IsInitialized() const { return apInitialized; };

	UFUNCTION(BlueprintCallable)
	void Store(const FItemAmount& item, bool personal = false);

	UFUNCTION(BlueprintCallable)
	int32 Take(const FItemAmount& item);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FItemAmount> GetItems(bool personal = false) const;

	UFUNCTION(BlueprintPure)
	TSet<FApPlayer> GetVaultEnabledPlayers() const;

	UFUNCTION(BlueprintPure)
	FApPlayer GetCurrentTeamGlobalVault() const;

	UFUNCTION(BlueprintPure)
	bool DoesPlayerAcceptVaultItems(const FApPlayer& player) const;

	UFUNCTION(BlueprintPure)
	TArray<TSubclassOf<UFGItemDescriptor>> GetAcceptedItemsPerVault(FApPlayer vault) const;

private:
	FString GetItemName(uint64 itemId) const;
	FString GetItemName(TSubclassOf<UFGItemDescriptor> item) const;

	void ParseVaultItemInfo(FString game, FString itemInfoString);
	void AddPersonalVaults(FString game);

	void SetupPersonalVault() const;

public: 
	UFUNCTION() //required for event hookup
	void OnRep_VaultEnabledPlayers();
};
