#pragma once

#include "CoreMinimal.h"

#include "Subsystem/ModSubsystem.h"
#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Data/ApGiftingMappings.h"
#include "Data/ApTypes.h"
#include "ApUtils.h"

#include "ApReplicatedGiftingSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApReplicatedGiftingSubsystem, Log, All);

UENUM(BlueprintType)
enum class EApGiftingServiceState : uint8 {
	Ready UMETA(DisplayName = "Connection Successful"),
	Initializing UMETA(DisplayName = "Connection Initializing"),
	Offline UMETA(DisplayName = "Not Connected to AP"),
	InvalidTarget UMETA(DisplayName = "Invalid Target")
};

UCLASS()
class AApReplicatedGiftingSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApReplicatedGiftingSubsystem();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float dt) override;

	static AApReplicatedGiftingSubsystem* Get(class UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Client Replicated ApGiftingSubsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApReplicatedGiftingSubsystem* Get(UObject* worldContext);

private:
	AApSubsystem* ap;
	AApConnectionInfoSubsystem* connectionInfoSubsystem;
	AApPortalSubsystem* portalSubsystem;
	AApMappingsSubsystem* mappingSubsystem;

	UPROPERTY(ReplicatedUsing = OnRep_AcceptedGiftTraitsPerPlayerReplicated)
	TArray<FApReplicateableGiftBoxMetaData> AcceptedGiftTraitsPerPlayerReplicated;

	UPROPERTY(Replicated)
	TArray<FApPlayer> AllPlayers;

	UPROPERTY(Replicated)
	EApGiftingServiceState ServiceState;

	TSet<EGiftTrait> AllTraits;
	TMap<FApPlayer, FApGiftBoxMetaData> AcceptedGiftTraitsPerPlayer; //build using replication

	bool hasLoadedPlayers = false;

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE EApGiftingServiceState GetState() const { return ServiceState; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool CanSend(FApPlayer targetPlayer, TSubclassOf<UFGItemDescriptor> itemClass);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FApPlayer> GetPlayersAcceptingGifts();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSet<EGiftTrait> GetAcceptedTraitsPerPlayer(FApPlayer player);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSet<EGiftTrait> GetTraitNamesPerItem(TSubclassOf<UFGItemDescriptor> itemClass);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FApGiftTrait> GetTraitsForItem(TSubclassOf<UFGItemDescriptor> itemClass);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool DoesPlayerAcceptGiftTrait(FApPlayer player, EGiftTrait giftTrait);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FApPlayer> GetAllApPlayers();

private:
	void UpdateAcceptedGifts();
	void UpdateAcceptedGiftTraitsPerPlayerReplicatedValue();

	UFUNCTION()
	void OnRep_AcceptedGiftTraitsPerPlayerReplicated();
};
