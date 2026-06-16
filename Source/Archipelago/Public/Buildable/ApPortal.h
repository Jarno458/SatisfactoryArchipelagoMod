#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildableFactory.h"

#include "FGFactoryConnectionComponent.h"
#include "FGPowerInfoComponent.h"

#include "Subsystem/ApVaultSubsystem.h"
#include "Subsystem/ModSubsystem.h"
#include "Data/ApTypes.h"

#include "ApPortal.generated.h"

UCLASS()
class ARCHIPELAGO_API AApPortal : public AFGBuildableFactory
{
	GENERATED_BODY()

public:
	AApPortal();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(SaveGame, Replicated, BlueprintReadOnly)
	FApPlayer targetPlayer;

private:
	bool initialized = false;

	AModSubsystem* portalSubsystem;
	AModSubsystem* replicatedGiftingSubsystem;
	AApVaultSubsystem* vaultSubsystem;

	UFGFactoryConnectionComponent* input = nullptr;
	UFGFactoryConnectionComponent* output = nullptr;

	bool camReceiveOutput = false;

	UPROPERTY(SaveGame)
	int roundRobinIndex;

	UPROPERTY(SaveGame)
	TArray<TSubclassOf<UFGItemDescriptor>> allowedOutput;

protected:
	UPROPERTY(SaveGame) //Save?
	UFGInventoryComponent* mInputInventory;

	UPROPERTY(SaveGame) //Save?
	UFGInventoryComponent* mOutputInventory;

public:
	FORCEINLINE bool CanReceiveOutput() const { return camReceiveOutput; }

	FORCEINLINE UFGInventoryComponent* GetInventory() const { return mInputInventory; }

	void ServerSetTarget(const FApPlayer& player);

	UFUNCTION(BlueprintPure, Category = "FactoryGame|Factory|Inventory")
	FORCEINLINE UFGInventoryComponent* GetInputInventory() const { return mInputInventory; }

	void ServerSendManually() const;

	virtual bool CanProduce_Implementation() const override;

	virtual void Factory_Tick(float dt) override;
	virtual void Factory_CollectInput_Implementation() override;

public:
	/** Set this to filter out what items are allowed and not allowed in the inventory */
	UFUNCTION()
	bool FilterInventoryClasses(TSubclassOf<UObject> object, int32 idx) const;
};
