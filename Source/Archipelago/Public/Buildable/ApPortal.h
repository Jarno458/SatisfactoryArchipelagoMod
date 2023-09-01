#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildableStorage.h"

#include "FGFactoryConnectionComponent.h"
#include "FGPowerInfoComponent.h"

#include "../Subsystem/ApSubsystem.h"

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

private:
	AApSubsystem* apSubSystem;
	TQueue<FInventoryItem> outputQueue;

	UFGFactoryConnectionComponent* input;
	UFGFactoryConnectionComponent* output;

public:
	UFUNCTION()
	void CheckPower();

	//virtual bool Factory_HasPower() const override;

	virtual void Factory_Tick(float dt) override;
	virtual bool Factory_PeekOutput_Implementation(const class UFGFactoryConnectionComponent* connection, TArray<FInventoryItem>& out_items, TSubclassOf<UFGItemDescriptor> type) const override;
	virtual bool Factory_GrabOutput_Implementation(class UFGFactoryConnectionComponent* connection, FInventoryItem& out_item, float& out_OffsetBeyond, TSubclassOf<UFGItemDescriptor> type) override;

	//UPROPERTY(BlueprintReadWrite)
	//bool Registered;
};
