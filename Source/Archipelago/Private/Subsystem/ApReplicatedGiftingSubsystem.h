#pragma once

#include "CoreMinimal.h"

#include "Subsystem/ModSubsystem.h"
#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "ApUtils.h"

#include "ApReplicatedGiftingSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApReplicatedGiftingSubsystem, Log, All);

UENUM(BlueprintType)
enum class EApGiftingSeriveState : uint8 {
	Ready UMETA(DisplayName = "Connected Successful"),
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
	AApPortalSubsystem* portalSubsystem;
	AApMappingsSubsystem* mappingSubsystem;

	UPROPERTY(ReplicatedUsing = OnRep_AcceptedGiftTraitsPerPlayerReplicated)
	TArray<FApReplicateableGiftBoxMetaData> AcceptedGiftTraitsPerPlayerReplicated;

	UPROPERTY(Replicated)
	TArray<FApPlayer> AllPlayers;

	UPROPERTY(Replicated)
	EApGiftingSeriveState ServiceState;

	TArray<FString> AllTraits;
	TMap<FApPlayer, FApGiftBoxMetaData> AcceptedGiftTraitsPerPlayer;

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE EApGiftingSeriveState GetState() const { return ServiceState; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool CanSend(FApPlayer targetPlayer, TSubclassOf<UFGItemDescriptor> itemClass);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FApPlayer> GetPlayersAcceptingGifts();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FString> GetAcceptedTraitsPerPlayer(FApPlayer player);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FString> GetTraitNamesPerItem(TSubclassOf<UFGItemDescriptor> itemClass);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FApGiftTrait> GetTraitsForItem(TSubclassOf<UFGItemDescriptor> itemClass);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool DoesPlayerAcceptGiftTrait(FApPlayer player, FString giftTrait);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FApPlayer> GetAllApPlayers();

private:
	void UpdateAcceptedGifts();
	void UpdateAcceptedGiftTraitsPerPlayerReplicatedValue();

	UFUNCTION()
	void OnRep_AcceptedGiftTraitsPerPlayerReplicated();
};
