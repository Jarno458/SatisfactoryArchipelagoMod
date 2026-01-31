#pragma once

#include "CoreMinimal.h"

#include "ApSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"

#include "Subsystem/ModSubsystem.h"

#include "ApVaultSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApVaultSubsystem, Log, All);

#define SINGLE_ITEM_PREFIX TEXT("Single: ")
#define GLOBAL_VAULT_SLOT 0

UCLASS()
class AApVaultSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApVaultSubsystem();

	virtual void BeginPlay() override;

	virtual void Tick(float dt) override;

	static AApVaultSubsystem* Get(const UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get ServerSide ApVaultSubsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApVaultSubsystem* Get(const UObject* worldContext);

private:
	const float pollInterfall = 10.0f;

	bool apInitialized;
	int itemNameInVaultKeyStartPosition;

	AApSubsystem* ap;
	AApConnectionInfoSubsystem* connectionInfoSubsystem;
	AApMappingsSubsystem* mappingSubsystem;

	TMap<FString, int64> globalItemAmounts;
	TMap<FString, int64> personalItemAmounts;

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

private:
	FString GetItemName(uint64 itemId) const;
	FString GetItemName(TSubclassOf<UFGItemDescriptor> item) const;
};
