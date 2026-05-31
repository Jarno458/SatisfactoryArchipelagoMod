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
				ap->MonitorDataStoreJsonObjectValue(giftboxKey, [this](const FString& key, const TSharedPtr<FJsonValue>& oldJsonValue, const TSharedPtr<FJsonValue>& newJsonValue, int slot) {
					UpdateAcceptedGifts(key, oldJsonValue, newJsonValue, slot);
				});
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

/*
TMap<FApPlayer, FApTraitBits> AApSubsystem::GetAcceptedTraitsPerPlayer() const {
	std::map<std::pair<int, int>, AP_GiftBoxProperties> giftboxes =
		CallOnGameThread<std::map<std::pair<int, int>, AP_GiftBoxProperties>>([]() { return AP_QueryGiftBoxes(); });

	static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

	TMap<FApPlayer, FApTraitBits> openGiftBoxes;
	for (const auto& giftbox : giftboxes) {
		if (giftbox.second.IsOpen) {
			FApPlayer player;
			player.Team = giftbox.first.first;
			player.Slot = giftbox.first.second;

			TSet<EGiftTrait> acceptedTraits;
			for (std::string trait : giftbox.second.DesiredTraits) {
				const int64 enumValue = giftTraitEnum->GetValueByNameString(UApUtils::FStr(trait));
				if (enumValue != INDEX_NONE) {
					acceptedTraits.Add(static_cast<EGiftTrait>(enumValue));
				}
			}

			FApTraitBits metaData(giftbox.second.AcceptsAnyGift, acceptedTraits);

			openGiftBoxes.Add(player, metaData);
		}
	}

	return openGiftBoxes;
}
*/

void AApReplicatedGiftingSubsystem::UpdateAcceptedGifts(const FString& key, const TSharedPtr<FJsonValue>& oldJsonValue, const TSharedPtr<FJsonValue>& newJsonValue, int triggeringSlot) {
	if (!HasAuthority())
		return;

	static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();

	FString discard, teamString;
	if (!key.Split(TEXT(";"), &discard, &teamString))
		return;

	int team = FCString::Atoi(*teamString);

	TSharedPtr<FJsonObject>* newValueObject;
	if (!newJsonValue->TryGetObject(newValueObject))
		return;

	TMap<FApPlayer, FApTraitBits> newAcceptedGiftTraitsPerPlayer;

	for (const TPair<FString, TSharedPtr<FJsonValue>>& giftJson : (*newValueObject)->Values) {
		 if (!FCString::IsNumeric(*giftJson.Key))
			 continue;

		 int slot = FCString::Atoi(*giftJson.Key);

		 TSharedPtr<FJsonObject>* giftboxProperties;
		 if (!giftJson.Value->TryGetObject(giftboxProperties))
			 continue;
		 
		 int minimal_supported_version;
		 if (!(*giftboxProperties)->TryGetNumberField("minimum_gift_data_version", minimal_supported_version))
		 {
			 minimal_supported_version = 999;
		 }

		 int maximum_supported_version;
		 if (!(*giftboxProperties)->TryGetNumberField("maximum_gift_data_version", maximum_supported_version))
		 {
			 maximum_supported_version = 1;
		 }

		  if (minimal_supported_version > 3 || maximum_supported_version < 3)
			  continue; //we cant deal with this data

		  bool isOpen;
		  if (!(*giftboxProperties)->TryGetBoolField("is_open", isOpen) || !isOpen)
		  {
			  continue;
		  }

		  bool acceptsAll;
		  if (!(*giftboxProperties)->TryGetBoolField("accepts_any_gift", acceptsAll))
		  {
			  acceptsAll = false;
		  }

		  TSet<EGiftTrait> acceptedTraits;

		  const TArray<TSharedPtr<FJsonValue>>* traitsJson;
		  if ((*giftboxProperties)->TryGetArrayField("desired_traits", traitsJson)) //optional value
		  {
				for (const TSharedPtr<FJsonValue>& traitJson : *traitsJson) {
					FString traitName;
					if (!traitJson->TryGetString(traitName))
						continue;

					const int64 enumValue = giftTraitEnum->GetValueByNameString(traitName);
					if (enumValue == INDEX_NONE)
						continue;

					EGiftTrait trait = static_cast<EGiftTrait>(enumValue);
					acceptedTraits.Add(trait);
				}
		  }

		  newAcceptedGiftTraitsPerPlayer.Add(FApPlayer{team, slot}, FApTraitBits{acceptsAll, acceptedTraits});
	}

	AcceptedGiftTraitsPerPlayer = newAcceptedGiftTraitsPerPlayer;

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
