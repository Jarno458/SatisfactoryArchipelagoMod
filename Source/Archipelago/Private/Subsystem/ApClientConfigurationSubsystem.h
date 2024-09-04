#pragma once

#include "CoreMinimal.h"

#include "Module/ModModule.h"
#include "Subsystem/ModSubsystem.h"

#include "ApClientConfigurationSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApClientConfigurationSubsystem, Log, All);

UCLASS()
class AApClientConfigurationSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApClientConfigurationSubsystem();

	virtual void BeginPlay() override;

	static AApClientConfigurationSubsystem* Get(class UWorld* world);

public:

private:
	void SetMamEnhancerConfigurationHooks();

	UFUNCTION() //required for event binding
	void LockMamEnhancerSpoilerConfiguration();
};
