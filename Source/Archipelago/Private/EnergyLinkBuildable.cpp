#include "EnergyLinkBuildable.h"

DEFINE_LOG_CATEGORY(EnergyLinkBuildable);

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

	UE_LOG(EnergyLinkBuildable, Display, TEXT("AEnergyLinkBuildable::BeginPlay()"));
	
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
	//UE_LOG(EnergyLinkBuildable, Display, TEXT("AEnergyLinkBuildable::Factory_Tick()"));


}