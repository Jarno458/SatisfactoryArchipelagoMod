#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildableStorage.h"
#include "FGPipeConnectionComponent.h"
#include "ApPortal.generated.h"

/**
 * 
 */
UCLASS()
class ARCHIPELAGO_API AApPortal : public AFGBuildableStorage
{
	GENERATED_BODY()
		AApPortal();

	virtual void BeginPlay() override;
public:

	UFUNCTION(BlueprintCallable)
		void CheckPower();

	//virtual bool Factory_HasPower() const override;

	virtual void Factory_Tick(float dt) override;

	UPROPERTY(BlueprintReadWrite)
		bool Registered;
};
