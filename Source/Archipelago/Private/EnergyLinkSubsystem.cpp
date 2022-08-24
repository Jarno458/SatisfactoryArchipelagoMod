#include "EnergyLinkSubsystem.h"

DEFINE_LOG_CATEGORY(ApEnergyLink);

// Sets default values
AEnergyLinkSubsystem::AEnergyLinkSubsystem() : Super()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEnergyLinkSubsystem::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(ApSubsystem, Display, TEXT("AEnergyLink:BeginPlay()"));
}

void AEnergyLinkSubsystem::SecondThick() {
	UE_LOG(ApSubsystem, Display, TEXT("AEnergyLink::SecondThick()"));


}


// Called every frame
void AEnergyLinkSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

