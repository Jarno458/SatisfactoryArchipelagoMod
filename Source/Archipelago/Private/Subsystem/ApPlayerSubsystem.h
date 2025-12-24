#pragma once

#include "CoreMinimal.h"
#include "ApSubsystem.h"
#include "FGCharacterPlayer.h"

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

	static AApPlayerSubsystem* Get(UWorld* world);
	UFUNCTION(BlueprintPure, DisplayName = "Get AP Player Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApPlayerSubsystem* Get(UObject* worldContext);

private:
	AApSubsystem* ap;

	bool canTriggerDeathlinks = false;

	void MassMurder();

public:
	UFUNCTION(BlueprintCallable)
	void OnPlayerDeath(AActor* deadActor, AActor* causee, const UDamageType* damageType);

private:
	void OnDeathLinkReceived(FText message);
};
