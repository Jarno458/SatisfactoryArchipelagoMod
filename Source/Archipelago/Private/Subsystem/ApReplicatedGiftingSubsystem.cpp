#include "ApReplicatedGiftingSubsystem.h"
#include "Data/ApGiftingMappings.h"
#include "FGGameState.h"

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
	//static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();
	TArray<EGiftTrait> allTraitsArray;
	UApGiftingMappings::TraitDefaultItemIds.GenerateKeyArray(allTraitsArray);
	AllTraits = TSet<EGiftTrait>(allTraitsArray);

	UWorld* world = GetWorld();
	fgcheck(world != nullptr);
	AGameStateBase* gameState = world->GetGameState();

	mappingSubsystem = AApMappingsSubsystem::Get(world);

	if (gameState->HasAuthority()) {
		LoadTraitMappings();

		ap = AApSubsystem::Get(world);
		connectionInfoSubsystem = AApConnectionInfoSubsystem::Get(world);
		portalSubsystem = AApPortalSubsystem::Get(world);
	} else {
		if (AFGGameState* factoryGameState = Cast<AFGGameState>(gameState)) {
			factoryGameState->mOnClientSubsystemsValid.AddDynamic(this, &AApReplicatedGiftingSubsystem::OnClientSubsystemsValid);

			if (factoryGameState->AreClientSubsystemsValid()) {
				LoadTraitMappings();
			}
		}
	}
}

void AApReplicatedGiftingSubsystem::OnClientSubsystemsValid() {
	LoadTraitMappings();
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

		if (!hasLoadedTraits || !portalSubsystem->IsInitialized()) {
			ServiceState = EApGiftingServiceState::Initializing;
		} else {
			SetActorTickEnabled(false);

			TSet<int> teams;
			for (const FApPlayer& player : AllPlayers) {
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

// Performance critical, used by portal belt ingestion
bool AApReplicatedGiftingSubsystem::CanSend(const FApPlayer& targetPlayer, const TSubclassOf<UFGItemDescriptor> itemClass) {
	if (!AcceptedGiftTraitsPerPlayer.Contains(targetPlayer))
		return false;

	if (!hasLoadedTraits || !TraitsPerItem.Contains(itemClass) || !AcceptedGiftTraitsPerPlayer.Contains(targetPlayer))
		return false;

	return AcceptedGiftTraitsPerPlayer[targetPlayer].HasOverlap(TraitsPerItem[itemClass]);
}

TArray<FApPlayer> AApReplicatedGiftingSubsystem::GetPlayersAcceptingGifts() {
	TArray<FApPlayer> keys;
	AcceptedGiftTraitsPerPlayer.GenerateKeyArray(keys);
	return keys;
}

TSet<EGiftTrait> AApReplicatedGiftingSubsystem::GetAcceptedTraitsPerPlayer(FApPlayer player) {
	if (!AcceptedGiftTraitsPerPlayer.Contains(player))
		return TSet<EGiftTrait>();

	if (AcceptedGiftTraitsPerPlayer[player].AcceptsAllTraits()) {
		return AllTraits;
	}

	return AcceptedGiftTraitsPerPlayer[player].GetTraits();
}

TSet<EGiftTrait> AApReplicatedGiftingSubsystem::GetTraitNamesPerItem(TSubclassOf<UFGItemDescriptor> itemClass) {
	if (!hasLoadedTraits || !TraitsPerItem.Contains(itemClass))
		return TSet<EGiftTrait>();

	return TraitsPerItem[itemClass].GetTraits();
}

TArray<FApGiftTrait> AApReplicatedGiftingSubsystem::GetTraitsForItem(TSubclassOf<UFGItemDescriptor> itemClass) {
	if (!hasLoadedTraits || !TraitsPerItem.Contains(itemClass))
		return TArray<FApGiftTrait>();

	TArray<FApGiftTrait> traits;

	for (TPair<EGiftTrait, float>& trait : TraitsPerItem[itemClass].TraitsValues) {
		FApGiftTrait traitSpecification;
		traitSpecification.Trait = trait.Key;
		traitSpecification.Quality = trait.Value;
		traitSpecification.Duration = 1.0f;

		traits.Add(traitSpecification);
	}

	return traits;
}

TArray<FApPlayer> AApReplicatedGiftingSubsystem::GetAllApPlayers() {
	return AllPlayers;
}

void AApReplicatedGiftingSubsystem::UpdateAcceptedGifts() {
	if (!HasAuthority())
		return;

	AcceptedGiftTraitsPerPlayer = ap->GetAcceptedTraitsPerPlayer();

	//only update replicated value if required
	if (AcceptedGiftTraitsPerPlayer.Num() != AcceptedGiftTraitsPerPlayerReplicated.Num()) {
		UpdateAcceptedGiftTraitsPerPlayerReplicatedValue();
	} else {
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
}

void AApReplicatedGiftingSubsystem::LoadTraitMappings() {
	AFGResourceSinkSubsystem* resourceSinkSubsystem = AFGResourceSinkSubsystem::Get(GetWorld());
	fgcheck(resourceSinkSubsystem)

	TMap<EGiftTrait, float> defaultSinkPointsPerTrait;
	for (const TPair<EGiftTrait, int64>& traitDefault : UApGiftingMappings::TraitDefaultItemIds) {
		fgcheck(mappingSubsystem->ApItems.Contains(traitDefault.Value) && mappingSubsystem->ApItems[traitDefault.Value]->Type == EItemType::Item);

		TSharedRef<FApItem> itemInfo = StaticCastSharedRef<FApItem>(mappingSubsystem->ApItems[traitDefault.Value]);

		int defaultItemSinkPoints = GetResourceSinkPointsForItem(resourceSinkSubsystem, itemInfo->Class, traitDefault.Value);

		defaultSinkPointsPerTrait.Add(traitDefault.Key, defaultItemSinkPoints);
	}

	for (TPair<int64, TSharedRef<FApItemBase>>& itemInfoMapping : mappingSubsystem->ApItems) {
		if (!UApGiftingMappings::TraitsPerItemRatings.Contains(itemInfoMapping.Key))
			continue;

		fgcheck(itemInfoMapping.Value->Type == EItemType::Item)

		TSharedRef<FApItem> itemInfo = StaticCastSharedRef<FApItem>(itemInfoMapping.Value);
		TSubclassOf<UFGItemDescriptor> itemClass = itemInfo->Class;
		int64 itemId = itemInfoMapping.Key;

		int itemValue = GetResourceSinkPointsForItem(resourceSinkSubsystem, itemClass, itemId);

		for (const TPair<EGiftTrait, float>& traitRelativeRating : UApGiftingMappings::TraitsPerItemRatings[itemInfoMapping.Key]) {
			EGiftTrait traitName = traitRelativeRating.Key;

			fgcheck(defaultSinkPointsPerTrait.Contains(traitName));
			float traitValue = GetTraitValue(itemValue, defaultSinkPointsPerTrait[traitName], traitRelativeRating.Value);

			TMap<EGiftTrait, float> traitValueForItem;
			traitValueForItem.Add(traitName, traitValue);

			while (UApGiftingMappings::TraitParents.Contains(traitName)) {
				traitName = UApGiftingMappings::TraitParents[traitName];

				if (!traitValueForItem.Contains(traitName)) {
					fgcheck(defaultSinkPointsPerTrait.Contains(traitName));
					traitValue = GetTraitValue(itemValue, defaultSinkPointsPerTrait[traitName], traitRelativeRating.Value);
					traitValueForItem.Add(traitName, traitValue);
				}
			}

			TraitsPerItem.Add(itemInfo->Class, FApTraitValues(traitValueForItem));
		}
	}

	//PrintTraitValuesPerItem();

	hasLoadedTraits = true;
}

int AApReplicatedGiftingSubsystem::GetResourceSinkPointsForItem(AFGResourceSinkSubsystem* resourceSinkSubsystem, TSubclassOf<UFGItemDescriptor> itemClass, int64 itemId) {
	if (UApGiftingMappings::HardcodedSinkValues.Contains(itemId))
		return UApGiftingMappings::HardcodedSinkValues[itemId];

	int value = resourceSinkSubsystem->GetResourceSinkPointsForItem(itemClass);

	if (value == 0) {
		FString itemName = UFGItemDescriptor::GetItemName(itemClass).ToString();
		UE_LOG(LogApMappingsSubsystem, Error, TEXT("AApMappingsSubsystem::GetResourceSinkPointsForItem(\"%s\", %i) No sink value for item"), *itemName, itemId);
		value = 1;
	}

	return value;
}

float AApReplicatedGiftingSubsystem::GetTraitValue(int itemValue, float avarageItemValueForTrait, float itemSpecificTraitMultiplier) {
	return (FPlatformMath::LogX(10, (double)itemValue + 0.1) / FPlatformMath::LogX(10, (double)avarageItemValueForTrait + 0.1)) * itemSpecificTraitMultiplier;
}

void AApReplicatedGiftingSubsystem::PrintTraitValuesPerItem() {
	TMap<EGiftTrait, TSortedMap<float, FString>> valuesPerItem;

	for (const TPair<TSubclassOf<UFGItemDescriptor>, FApTraitValues>& traitsPerItem : TraitsPerItem) {
		for (TPair<EGiftTrait, float> trait : traitsPerItem.Value.TraitsValues) {
			if (!valuesPerItem.Contains(trait.Key))
				valuesPerItem.Add(trait.Key, TSortedMap<float, FString>());

			FString itemName = mappingSubsystem->ItemIdToName[mappingSubsystem->ItemClassToItemId[traitsPerItem.Key]];

			valuesPerItem[trait.Key].Add(trait.Value, itemName);
		}
	}

	TArray<FString> lines;
	for (const TPair<EGiftTrait, TSortedMap<float, FString>>& traitsPerItem : valuesPerItem) {
		static const UEnum* giftTraitEnum = StaticEnum<EGiftTrait>();
		FName traitName = giftTraitEnum->GetNameByValue((int64)traitsPerItem.Key);

		lines.Add(FString::Printf(TEXT("Trait: \"%s\":"), *traitName.ToString()));

		for (TPair<float, FString> valuePerItem : traitsPerItem.Value)
			lines.Add(FString::Printf(TEXT("  - Item: \"%s\": %.2f"), *valuePerItem.Value, valuePerItem.Key));
	}

	FString fileText = FString::Join(lines, TEXT("\n"));

	UApUtils::WriteStringToFile(fileText, TEXT("T:\\ItemTraits.txt"), false);
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

#pragma optimize("", on)