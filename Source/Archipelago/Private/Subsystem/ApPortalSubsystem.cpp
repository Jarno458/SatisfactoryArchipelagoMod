#include "Subsystem/ApPortalSubsystem.h"

#include "ApVaultSubsystem.h"
#include "Containers/List.h"
#include "EngineUtils.h"
#include "Buildable/ApPortal.h"
#include "Subsystem/ApServerGiftingSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"

DEFINE_LOG_CATEGORY(LogApPortalSubsystem);

#define VAULT_SEND_INTERVAL 10.0f

AApPortalSubsystem* AApPortalSubsystem::Get(UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApPortalSubsystem>();
}

AApPortalSubsystem::AApPortalSubsystem() : Super() {
	UE_LOG(LogApPortalSubsystem, Display, TEXT("AApPortalSubsystem::AApPortalSubsystem()"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	//needs to thick atleast 1200 times per minute to keep up with a 1200 belt, 1200 / 60sec = 20x per second
	//0.048 is ~20.8 times per second
	//PrimaryActorTick.TickInterval = 0.048f; // Not needed anymore as portals take care of their own output
	PrimaryActorTick.TickInterval = 1.0f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

void AApPortalSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApPortalSubsystem, Display, TEXT("AApPortalSubsystem::BeginPlay()"));

	UWorld* world = GetWorld();
	giftingSubsystem = AApServerGiftingSubsystem::Get(world);
	vaultSubsystem = AApVaultSubsystem::Get(world);

	lastAutoSendTime = FDateTime::Now();
}

void AApPortalSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!isInitialized) {
		isInitialized = true;
	}

	if (lastAutoSendTime + FTimespan::FromSeconds(VAULT_SEND_INTERVAL) < FDateTime::Now()) {
		ProcessAutoVaultStoring();

		lastAutoSendTime = FDateTime::Now();
	}
}

void AApPortalSubsystem::SendBuffer(FApPlayer targetPlayer, TArray<FItemAmount> items) const
{
	if (!targetPlayer.IsValid())
		return;

	if (vaultSubsystem->DoesPlayerAcceptVaultItems(targetPlayer)) {
		for (FItemAmount item : items) {
			vaultSubsystem->Store(item, targetPlayer);
		}
		return;
	}

	for (FItemAmount item : items) {
		static_cast<AApServerGiftingSubsystem*>(giftingSubsystem)->EnqueueForSending(targetPlayer, item);
	}
}

void AApPortalSubsystem::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) {
	SetActorTickEnabled(false);
}

void AApPortalSubsystem::PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) {
	SetActorTickEnabled(true);
}

void AApPortalSubsystem::ProcessAutoVaultStoring() const
{
	TMap<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, uint64>> itemsToStore;

	for (TActorIterator<AApPortal> actorIterator(GetWorld()); actorIterator; ++actorIterator) {
		AApPortal* portal = *actorIterator;
		if (!IsValid(portal) || !portal->targetPlayer.IsValid() || !vaultSubsystem->DoesPlayerAcceptVaultItems(portal->targetPlayer))
			continue;

		UFGInventoryComponent* inventory = portal->GetInventory();
		if (!IsValid(inventory))
			continue;

		TArray<FInventoryStack> contents;
		inventory->GetInventoryStacks(contents);
		inventory->Empty();

		TMap<TSubclassOf<UFGItemDescriptor>, uint64>& itemsForPlayer = itemsToStore.FindOrAdd(portal->targetPlayer);
		for (const FInventoryStack& stack : contents) {
			itemsForPlayer.FindOrAdd(stack.Item.GetItemClass()) += stack.NumItems;
		}
	}

	for (const TPair<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, uint64>>& playerItems : itemsToStore) {
		const FApPlayer& player = playerItems.Key;

		for (const TPair<TSubclassOf<UFGItemDescriptor>, uint64>& item : playerItems.Value) {
			const uint64& amount = item.Value;

			vaultSubsystem->Store(FItemAmount(item.Key, static_cast<int32>(FMath::Clamp<int64>(amount, 0, INT32_MAX))), player);
		}
	}
}
