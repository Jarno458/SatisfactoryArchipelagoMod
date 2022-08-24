// Copyright Coffee Stain Studios. All Rights Reserved.


#include "EnergyLinkBuildable.h"

// Sets default values
AEnergyLinkBuildable::AEnergyLinkBuildable()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEnergyLinkBuildable::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEnergyLinkBuildable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AEnergyLinkBuildable::Factory_HasPower() const
{
	bool x = Super::Factory_HasPower();

	return x;
}

void AEnergyLinkBuildable::Factory_Tick(float dt) {

}