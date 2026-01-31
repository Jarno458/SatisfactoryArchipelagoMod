#pragma once

#include "CoreMinimal.h"

#include "Subsystem/ModSubsystem.h"
#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "Subsystem/ApReplicatedGiftingSubsystem.h"

#include "ApServerGiftingSubsystem.generated.h"

class AApVaultSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogApServerGiftingSubsystem, Log, All);

UCLASS()
class AApServerGiftingSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApServerGiftingSubsystem();

	virtual void BeginPlay() override;

	virtual void Tick(float dt) override;

	static AApServerGiftingSubsystem* Get(class UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get ServerSide ApGiftingSubsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApServerGiftingSubsystem* Get(UObject* worldContext);

private:
	const float pollInterfall = 10.0f;

	bool apInitialized;

	TMap<uint32, TSubclassOf<UFGItemDescriptor>> ItemPerTraitsHashCache;

	TMap<FApPlayer, TSharedPtr<TQueue<FItemAmount, EQueueMode::Mpsc>>> InputQueue;

	TSet<FString> ProcessedIds;

	AApSubsystem* ap;
	AApConnectionInfoSubsystem* connectionInfoSubsystem;
	AApPortalSubsystem* portalSubSystem;
	AApMappingsSubsystem* mappingSubsystem;
	AApReplicatedGiftingSubsystem* replicatedGiftingSubsystem;
	AApVaultSubsystem* vaultSubsystem;

public:
	void EnqueueForSending(FApPlayer targetPlayer, FItemAmount itemStack);

private:
	void PullAllGiftsAsync();
	void ProcessInputQueue();

	void Send(TMap<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>>& itemsToSend);

	TSubclassOf<UFGItemDescriptor> TryGetItemClassByTraits(TArray<FApGiftTrait>& traits);

	void UpdatedProcessedIds(TArray<FApReceiveGift>& gifts);

	bool HasTraitKnownToSatisfactory(TArray<FApGiftTrait>& traits);

	uint32 GetTraitsHash(TArray<FApGiftTrait>& traits);
};
