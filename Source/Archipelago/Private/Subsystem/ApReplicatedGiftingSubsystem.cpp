#include "ApReplicatedGiftingSubsystem.h"
#include "Data/ApGiftingMappings.h"

DEFINE_LOG_CATEGORY(LogApReplicatedGiftingSubsystem);

//TODO REMOVE
#pragma optimize("", off)

AApReplicatedGiftingSubsystem* AApReplicatedGiftingSubsystem::Get(UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApReplicatedGiftingSubsystem* AApReplicatedGiftingSubsystem::Get(class UWorld* world) {
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
	} else {
		PrimaryActorTick.bCanEverTick = false;
	}
}

void AApReplicatedGiftingSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AApReplicatedGiftingSubsystem, AcceptedGiftTraitsPerPlayerReplicated);
	DOREPLIFETIME(AApReplicatedGiftingSubsystem, AllPlayers);
	DOREPLIFETIME(AApReplicatedGiftingSubsystem, ServiceState);
}

void AApReplicatedGiftingSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApReplicatedGiftingSubsystem, Display, TEXT("AApReplicatedGiftingSubsystem::BeginPlay()"));

	// todo check if there is a build in method to get all keys of an enum
	TArray<EGiftTrait> allTraitsArray;
	UApGiftingMappings::TraitDefaultItemIds.GenerateKeyArray(allTraitsArray);
	AllTraits = TSet<EGiftTrait>(allTraitsArray);

	UWorld* world = GetWorld();
	fgcheck(world != nullptr);

	mappingSubsystem = AApMappingsSubsystem::Get(world);

	if (HasAuthority()) {
		ap = AApSubsystem::Get(world);
		connectionInfoSubsystem = AApConnectionInfoSubsystem::Get(world);
		portalSubsystem = AApPortalSubsystem::Get(world);
	}
}

void AApReplicatedGiftingSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!HasAuthority())
		return;

	if (connectionInfoSubsystem->GetConnectionState() != EApConnectionState::Connected) {
		SetActorTickInterval(0.1f);

		ServiceState = EApGiftingServiceState::Offline;
	} else {
		if (!hasLoadedPlayers) {
			AllPlayers = ap->GetAllApPlayers();
			hasLoadedPlayers = true;
		}

		if (!mappingSubsystem->HasLoadedItemTraits() || !portalSubsystem->IsInitialized()) {
			ServiceState = EApGiftingServiceState::Initializing;
		} else {
			SetActorTickEnabled(false);

			TSet<int> teams;
			for (FApPlayer player : AllPlayers) {
				teams.Add(player.Team);
			}
			for (int team : teams) {
				FString giftboxKey = FString::Format(TEXT("GiftBoxes;{0}"), { team });
				ap->MonitorDataStoreValue(giftboxKey, [this]() { UpdateAcceptedGifts();	});
			}

			ServiceState = EApGiftingServiceState::Ready;
		}
	}
}

bool AApReplicatedGiftingSubsystem::CanSend(FApPlayer targetPlayer, TSubclassOf<UFGItemDescriptor> itemClass) {
	if (!AcceptedGiftTraitsPerPlayer.Contains(targetPlayer))
		return false;

	if (!mappingSubsystem->HasLoadedItemTraits() || !mappingSubsystem->TraitsPerItem.Contains(itemClass))
		return false;

	for (TPair<EGiftTrait, float>& trait : mappingSubsystem->TraitsPerItem[itemClass]) {
		if (DoesPlayerAcceptGiftTrait(targetPlayer, trait.Key))
			return true;
	}

	return false;
}

TArray<FApPlayer> AApReplicatedGiftingSubsystem::GetPlayersAcceptingGifts() {
	TArray<FApPlayer> keys;
	AcceptedGiftTraitsPerPlayer.GenerateKeyArray(keys);
	return keys;
}

TSet<EGiftTrait> AApReplicatedGiftingSubsystem::GetAcceptedTraitsPerPlayer(FApPlayer player) {
	if (!AcceptedGiftTraitsPerPlayer.Contains(player))
		return TSet<EGiftTrait>();

	if (AcceptedGiftTraitsPerPlayer[player].AcceptAllTraits) {
		return AllTraits;
	}

	return AcceptedGiftTraitsPerPlayer[player].AcceptedTraits;
}

TSet<EGiftTrait> AApReplicatedGiftingSubsystem::GetTraitNamesPerItem(TSubclassOf<UFGItemDescriptor> itemClass) {
	if (!mappingSubsystem->HasLoadedItemTraits() || !mappingSubsystem->TraitsPerItem.Contains(itemClass))
		return TSet<EGiftTrait>();

	TArray<EGiftTrait> traits;
	mappingSubsystem->TraitsPerItem[itemClass].GenerateKeyArray(traits);

	return TSet<EGiftTrait>(traits);
}

TArray<FApGiftTrait> AApReplicatedGiftingSubsystem::GetTraitsForItem(TSubclassOf<UFGItemDescriptor> itemClass) {
	if (!mappingSubsystem->HasLoadedItemTraits() || !mappingSubsystem->TraitsPerItem.Contains(itemClass))
		return TArray<FApGiftTrait>();

	TArray<FApGiftTrait> traits;

	for (TPair<EGiftTrait, float>& trait : mappingSubsystem->TraitsPerItem[itemClass]) {
		FApGiftTrait traitSpecification;
		traitSpecification.Trait = trait.Key;
		traitSpecification.Quality = trait.Value;
		traitSpecification.Duration = 1.0f;

		traits.Add(traitSpecification);
	}

	return traits;
}

bool AApReplicatedGiftingSubsystem::DoesPlayerAcceptGiftTrait(FApPlayer player, EGiftTrait giftTrait) {
	return AcceptedGiftTraitsPerPlayer.Contains(player)
		&& (AcceptedGiftTraitsPerPlayer[player].AcceptAllTraits || AcceptedGiftTraitsPerPlayer[player].AcceptedTraits.Contains(giftTrait));
}

TArray<FApPlayer> AApReplicatedGiftingSubsystem::GetAllApPlayers() {
	return AllPlayers;
}

void AApReplicatedGiftingSubsystem::UpdateAcceptedGifts() {
	if (!HasAuthority())
		return;

	AcceptedGiftTraitsPerPlayer = ap->GetAcceptedTraitsPerPlayer();

	//only edit if required
	if (AcceptedGiftTraitsPerPlayer.Num() != AcceptedGiftTraitsPerPlayerReplicated.Num()) {
		UpdateAcceptedGiftTraitsPerPlayerReplicatedValue();
	} else {
		for (int i = 0; i < AcceptedGiftTraitsPerPlayerReplicated.Num(); i++) {
			FApPlayer player = AcceptedGiftTraitsPerPlayerReplicated[i].Player;

			if (!AcceptedGiftTraitsPerPlayer.Contains(player) || AcceptedGiftTraitsPerPlayer[player] != AcceptedGiftTraitsPerPlayerReplicated[i].Box) {
				UpdateAcceptedGiftTraitsPerPlayerReplicatedValue();
				break;
			}
		}
	}
}

void AApReplicatedGiftingSubsystem::UpdateAcceptedGiftTraitsPerPlayerReplicatedValue() {
	TArray<FApReplicateableGiftBoxMetaData> toBeReplicated;

	for (TPair<FApPlayer, FApGiftBoxMetaData>& acceptedTraitsPerPlayer : AcceptedGiftTraitsPerPlayer) {
		FApReplicateableGiftBoxMetaData replicatedMetaData;
		replicatedMetaData.Player = acceptedTraitsPerPlayer.Key;
		replicatedMetaData.Box = acceptedTraitsPerPlayer.Value;

		toBeReplicated.Add(replicatedMetaData);
	}

	AcceptedGiftTraitsPerPlayerReplicated = toBeReplicated;
}

void AApReplicatedGiftingSubsystem::OnRep_AcceptedGiftTraitsPerPlayerReplicated() {
	if (HasAuthority())
		return;

	UE_LOG(LogApReplicatedGiftingSubsystem, Display, TEXT("AApReplicatedGiftingSubsystem::OnRep_AcceptedGiftTraitsPerPlayerReplicated()"));

	TMap<FApPlayer, FApGiftBoxMetaData> replicated;

	for (FApReplicateableGiftBoxMetaData& replicatedMetaData : AcceptedGiftTraitsPerPlayerReplicated) {
		replicated.Add(replicatedMetaData.Player, replicatedMetaData.Box);
	}

	AcceptedGiftTraitsPerPlayer = replicated;
}

#pragma optimize("", on)