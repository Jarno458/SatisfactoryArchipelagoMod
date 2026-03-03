#include "ApReplicatedGiftingSubsystem.h"
#include "Data/ApGiftingMappings.h"
#include "PushModel.h"
#include "UnrealNetwork.h"
#include "SubsystemActorManager.h"

DEFINE_LOG_CATEGORY(LogApReplicatedGiftingSubsystem);

AApReplicatedGiftingSubsystem* AApReplicatedGiftingSubsystem::Get(UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApReplicatedGiftingSubsystem* AApReplicatedGiftingSubsystem::Get(UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApReplicatedGiftingSubsystem>();
}

AApReplicatedGiftingSubsystem::AApReplicatedGiftingSubsystem() : Super() {
	UE_LOG(LogApReplicatedGiftingSubsystem, Display, TEXT("AApReplicatedGiftingSubsystem::AApReplicatedGiftingSubsystem()"));

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;

	if (HasAuthority()) {
		PrimaryActorTick.bCanEverTick = true;
		PrimaryActorTick.bStartWithTickEnabled = true;
		PrimaryActorTick.TickInterval = 0.1f;
	}
	else {
		PrimaryActorTick.bCanEverTick = false;
	}
}

void AApReplicatedGiftingSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams replicationParams;
	replicationParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AApReplicatedGiftingSubsystem, AcceptedGiftTraitsPerPlayerReplicated, replicationParams);

	DOREPLIFETIME(AApReplicatedGiftingSubsystem, ServiceState);
}

void AApReplicatedGiftingSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApReplicatedGiftingSubsystem, Display, TEXT("AApReplicatedGiftingSubsystem::BeginPlay()"));

	UWorld* world = GetWorld();
	fgcheck(world != nullptr);

	giftTraitsSubsystem = AApGiftTraitsSubsystem::Get(world);

	if (HasAuthority()) {
		ap = AApSubsystem::Get(world);
		connectionInfoSubsystem = AApConnectionInfoSubsystem::Get(world);
		portalSubsystem = AApPortalSubsystem::Get(world);
		playerInfoSubsystem = AApPlayerInfoSubsystem::Get(world);
	}
}

void AApReplicatedGiftingSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!HasAuthority())
		return;

	if (connectionInfoSubsystem->GetConnectionState() != EApConnectionState::Connected) {
		SetActorTickInterval(0.1f);

		ServiceState = EApGiftingServiceState::Offline;
	}
	else {
		if (!giftTraitsSubsystem->HasLoadedItemTraits() || !portalSubsystem->IsInitialized() || !playerInfoSubsystem->IsInitialized()) {
			ServiceState = EApGiftingServiceState::Initializing;
		}
		else {
			SetActorTickEnabled(false);

			TSet<int> teams = playerInfoSubsystem->GetTeams();

			for (int team : teams) {
				FString giftboxKey = FString::Format(TEXT("GiftBoxes;{0}"), { team });
				ap->MonitorDataStoreValue(giftboxKey, [this]() { UpdateAcceptedGifts();	});
			}

			ServiceState = EApGiftingServiceState::Ready;
		}
	}
}

// Performance critical, used by portal belt ingestion
bool AApReplicatedGiftingSubsystem::CanSend(const FApPlayer& targetPlayer, const TSubclassOf<UFGItemDescriptor> itemClass) {
	if (!AcceptedGiftTraitsPerPlayer.Contains(targetPlayer))
		return false;

	if (!giftTraitsSubsystem->HasLoadedItemTraits() || !AcceptedGiftTraitsPerPlayer.Contains(targetPlayer))
		return false;

	FApTraitValues traits = giftTraitsSubsystem->GetTraitsForItem(itemClass);

	return AcceptedGiftTraitsPerPlayer[targetPlayer].HasOverlap(static_cast<FApTraitBits>(traits));
}

TSet<FApPlayer> AApReplicatedGiftingSubsystem::GetPlayersAcceptingGifts() const {
	TArray<FApPlayer> keysArray;
	AcceptedGiftTraitsPerPlayer.GenerateKeyArray(keysArray);
	return TSet<FApPlayer>(keysArray);
}

TSet<EGiftTrait> AApReplicatedGiftingSubsystem::GetAcceptedTraitsPerPlayer(FApPlayer player) {
	if (!AcceptedGiftTraitsPerPlayer.Contains(player))
		return TSet<EGiftTrait>();

	if (AcceptedGiftTraitsPerPlayer[player].AcceptsAllTraits()) {
		return giftTraitsSubsystem->GetAllTraits();
	}

	return AcceptedGiftTraitsPerPlayer[player].GetTraits();
}

TArray<TSubclassOf<UFGItemDescriptor>> AApReplicatedGiftingSubsystem::GetAcceptedItemsPerPlayer(FApPlayer player) const
{
	TArray<TSubclassOf<UFGItemDescriptor>> items;

	if (!AcceptedGiftTraitsPerPlayer.Contains(player))
		return items;

	return giftTraitsSubsystem->GetItemsWithOverlappingTraits(AcceptedGiftTraitsPerPlayer[player]);
}

void AApReplicatedGiftingSubsystem::UpdateAcceptedGifts() {
	if (!HasAuthority())
		return;

	AcceptedGiftTraitsPerPlayer = ap->GetAcceptedTraitsPerPlayer();

	//only update replicated value if required
	if (AcceptedGiftTraitsPerPlayer.Num() != AcceptedGiftTraitsPerPlayerReplicated.Num()) {
		UpdateAcceptedGiftTraitsPerPlayerReplicatedValue();
	}
	else {
		for (int i = 0; i < AcceptedGiftTraitsPerPlayerReplicated.Num(); i++) {
			FApPlayer player = AcceptedGiftTraitsPerPlayerReplicated[i].Player;

			if (!AcceptedGiftTraitsPerPlayer.Contains(player) || AcceptedGiftTraitsPerPlayer[player] != AcceptedGiftTraitsPerPlayerReplicated[i]) {
				UpdateAcceptedGiftTraitsPerPlayerReplicatedValue();
				break;
			}
		}
	}
}

void AApReplicatedGiftingSubsystem::UpdateAcceptedGiftTraitsPerPlayerReplicatedValue() {
	TArray<FApTraitByPlayer> toBeReplicated;

	for (TPair<FApPlayer, FApTraitBits>& acceptedTraitsPerPlayer : AcceptedGiftTraitsPerPlayer) {
		toBeReplicated.Add(FApTraitByPlayer(acceptedTraitsPerPlayer.Key, acceptedTraitsPerPlayer.Value));
	}

	AcceptedGiftTraitsPerPlayerReplicated = toBeReplicated;

	MARK_PROPERTY_DIRTY_FROM_NAME(AApReplicatedGiftingSubsystem, AcceptedGiftTraitsPerPlayerReplicated, this);
}

void AApReplicatedGiftingSubsystem::OnRep_AcceptedGiftTraitsPerPlayerReplicated() {
	if (HasAuthority())
		return;

	UE_LOG(LogApReplicatedGiftingSubsystem, Display, TEXT("AApReplicatedGiftingSubsystem::OnRep_AcceptedGiftTraitsPerPlayerReplicated()"));

	TMap<FApPlayer, FApTraitBits> replicated;

	for (const FApTraitByPlayer& replicatedMetaData : AcceptedGiftTraitsPerPlayerReplicated) {
		replicated.Add(replicatedMetaData.Player, replicatedMetaData);
	}

	AcceptedGiftTraitsPerPlayer = replicated;
}
