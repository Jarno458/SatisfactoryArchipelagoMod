// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Buildables/FGBuildablePowerStorage.h"
#include "EnergyLinkBuildable.generated.h"

UCLASS()
class ARCHIPELAGO_API AEnergyLinkBuildable : public AFGBuildablePowerStorage
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnergyLinkBuildable();

	virtual void BeginPlay() override;


public:
	virtual bool Factory_HasPower() const override;

	virtual void Factory_Tick(float dt) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
