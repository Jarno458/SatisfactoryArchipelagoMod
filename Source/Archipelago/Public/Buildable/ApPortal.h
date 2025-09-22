#pragma once

#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "Misc/ScopeTryLock.h"

#include "Buildables/FGBuildableFactory.h"

#include "FGFactoryConnectionComponent.h"
#include "FGPowerInfoComponent.h"

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

	UPROPERTY(BlueprintReadWrite, SaveGame, Replicated)
	FApPlayer targetPlayer;

private:
	AModSubsystem* portalSubsystem;
	AModSubsystem* replicatedGiftingSubsystem;

	UFGFactoryConnectionComponent* input = nullptr;
	UFGFactoryConnectionComponent* output = nullptr;

	bool camReceiveOutput = false;

	//mutable as used in  Factory_*Output_Implementation const methods
	mutable FCriticalSection outputLock;

	UPROPERTY(SaveGame)
	FInventoryItem nextItemToOutput = FInventoryItem::NullInventoryItem;

public:
	FORCEINLINE bool CanReceiveOutput() const { return camReceiveOutput; };

	bool TrySetOutput(FInventoryItem item);

	virtual bool CanProduce_Implementation() const override;

	virtual void Factory_Tick(float dt) override;
	virtual bool Factory_PeekOutput_Implementation(const class UFGFactoryConnectionComponent* connection, TArray<FInventoryItem>& out_items, TSubclassOf<UFGItemDescriptor> type) const override;
	virtual bool Factory_GrabOutput_Implementation(class UFGFactoryConnectionComponent* connection, FInventoryItem& out_item, float& out_OffsetBeyond, TSubclassOf<UFGItemDescriptor> type) override;
	virtual void Factory_CollectInput_Implementation() override;
};
