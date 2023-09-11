#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildableStorage.h"

#include "FGFactoryConnectionComponent.h"
#include "FGPowerInfoComponent.h"

//#include "../Subsystem/ApPortalSubsystem.h" set inside cpp to avoid circular dep



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
	AModSubsystem* portalSubsystem;

	UFGFactoryConnectionComponent* input;
	UFGFactoryConnectionComponent* output;
	UFGInventoryComponent* inventory;

	int portalId;

	int targetPlayerSlot = 1;

	bool camReceiveOutput = false;

public:
	UFUNCTION()
	void CheckPower(bool newHasPower) const;

	FORCEINLINE bool CanReceiveOutput() const { return camReceiveOutput; };

	virtual bool CanProduce_Implementation() const override;

	virtual void Factory_Tick(float dt) override;
	virtual bool Factory_PeekOutput_Implementation(const class UFGFactoryConnectionComponent* connection, TArray<FInventoryItem>& out_items, TSubclassOf<UFGItemDescriptor> type) const override;
	virtual bool Factory_GrabOutput_Implementation(class UFGFactoryConnectionComponent* connection, FInventoryItem& out_item, float& out_OffsetBeyond, TSubclassOf<UFGItemDescriptor> type) override;

private:
	void Register();
	void UnRegister();
};
