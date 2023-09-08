#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildableStorage.h"

#include "FGFactoryConnectionComponent.h"
#include "FGPowerInfoComponent.h"

//#include "../Subsystem/ApPortalSubsystem.h"

#include "ApPortal.generated.h"

/**
 * 
 */
UCLASS()
class ARCHIPELAGO_API AApPortal : public AFGBuildableStorage
{
	GENERATED_BODY()

public:
	AApPortal();

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type reason) override;

	mutable TQueue<FInventoryItem> outputQueue;

private:
	UFGFactoryConnectionComponent* input;
	UFGFactoryConnectionComponent* output;

	int portalId;

	int targetPlayerSlot = 1;

public:
	UFUNCTION()
	void CheckPower(bool newHasPower) const;

	virtual bool Factory_HasPower() const override;
	//virtual bool IsConfigured() const override;
	//virtual bool Factory_IsProducing() const override;
	virtual bool CanProduce_Implementation() const override;

	virtual void Factory_Tick(float dt) override;
	virtual bool Factory_PeekOutput_Implementation(const class UFGFactoryConnectionComponent* connection, TArray<FInventoryItem>& out_items, TSubclassOf<UFGItemDescriptor> type) const override;
	virtual bool Factory_GrabOutput_Implementation(class UFGFactoryConnectionComponent* connection, FInventoryItem& out_item, float& out_OffsetBeyond, TSubclassOf<UFGItemDescriptor> type) override;

private:
	void Register();
	void UnRegister();
};
