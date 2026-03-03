#pragma once

#include "CoreMinimal.h"
#include "ApGiftTraitsSubsystem.h"
#include "ApPlayerInfoSubsystem.h"

#include "Subsystem/ModSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Data/ApGiftingMappings.h"
#include "Data/ApTypes.h"

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
	AApPlayerInfoSubsystem* playerInfoSubsystem;
	AApGiftTraitsSubsystem* giftTraitsSubsystem;

	UPROPERTY(ReplicatedUsing = OnRep_AcceptedGiftTraitsPerPlayerReplicated)
	TArray<FApTraitByPlayer> AcceptedGiftTraitsPerPlayerReplicated;

	UPROPERTY(Replicated)
	EApGiftingServiceState ServiceState;

	TMap<FApPlayer, FApTraitBits> AcceptedGiftTraitsPerPlayer; //build using replication

public:
	UFUNCTION(BlueprintPure)
	FORCEINLINE EApGiftingServiceState GetState() const { return ServiceState; };

	UFUNCTION(BlueprintPure)
	bool CanSend(const FApPlayer& targetPlayer, const TSubclassOf<UFGItemDescriptor> itemClass);

	UFUNCTION(BlueprintPure)
	TSet<FApPlayer> GetPlayersAcceptingGifts() const;

	UFUNCTION(BlueprintPure)
	TSet<EGiftTrait> GetAcceptedTraitsPerPlayer(FApPlayer player);

	UFUNCTION(BlueprintPure)
	TArray<TSubclassOf<UFGItemDescriptor>> GetAcceptedItemsPerPlayer(FApPlayer player) const;

private:
	void UpdateAcceptedGifts();
	void UpdateAcceptedGiftTraitsPerPlayerReplicatedValue();

private:
	UFUNCTION() //required for event hookup
	void OnRep_AcceptedGiftTraitsPerPlayerReplicated();
};
