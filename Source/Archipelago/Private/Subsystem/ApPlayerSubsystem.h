#pragma once

#include "CoreMinimal.h"
#include "ApSlotDataSubsystem.h"
#include "ApSubsystem.h"

#include "Subsystem/ModSubsystem.h"

#include "ApPlayerSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApPlayerSubsystem, Log, All);

UCLASS()
class AApPlayerSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApPlayerSubsystem();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	static AApPlayerSubsystem* Get(UWorld* world);
	UFUNCTION(BlueprintPure, DisplayName = "Get AP Player Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApPlayerSubsystem* Get(UObject* worldContext);

private:
	AApSubsystem* ap;
	AApSlotDataSubsystem* slotData;

	bool isInitialized = false;
	bool canTriggerDeathlinks = false;

	bool IsOnlinePlayer(AFGCharacterPlayer* player);
	void MassMurder();

public:
	UFUNCTION(BlueprintCallable)
	void OnPlayerDeath(AActor* deadActor, const UDamageType* damageType, AActor* instigatedBy, AActor* damageCauser);

private:
	void OnDeathLinkReceived(FString source, FString cause);

	void AddDeathLinkMessageToChat(const FString& source, const FString& cause) const;

	static FString GetDamageSuffix(const UDamageType* damageType, AActor* self, AActor* instigatedBy, AActor* damageCauser);
};
