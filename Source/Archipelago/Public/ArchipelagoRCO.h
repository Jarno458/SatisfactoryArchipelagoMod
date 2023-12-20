#pragma once

#include "CoreMinimal.h"
#include "FGRemoteCallObject.h"

#include "ApUtils.h"
#include "Buildable/ApPortal.h"

#include "ArchipelagoRCO.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApReplication, Log, All);

/**
 * 
 */
UCLASS()
class ARCHIPELAGO_API UArchipelagoRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	bool Dummy;

	UFUNCTION(BlueprintCallable, unreliable, Server)
	void ServerSetPortalTargetPlayer(AApPortal* Buidling, FApPlayer Player);
};
