#include "Buildable/ApPortal.h"

#include "FGPipeConnectionComponent.h"
#include "FGPowerInfoComponent.h"

DEFINE_LOG_CATEGORY(LogGame); // A base-game header is using this category so we must do this to avoid unresolved external symbol

//TODO REMOVE
#pragma optimize("", off)

AApPortal::AApPortal() : Super() {
	this->mPowerInfoClass = UFGPowerInfoComponent::StaticClass();
	this->mInventorySizeX = 20;
	this->mInventorySizeY = 20;
	this->mSignificanceRange = 18000;
	this->MaxRenderDistance = -1;
	//this->mFactoryTickFunction.TickGroup = TG_PrePhysics;
	//this->mFactoryTickFunction.EndTickGroup = TG_PrePhysics;
	//this->mFactoryTickFunction.bTickEvenWhenPaused = false;
	//this->mFactoryTickFunction.bCanEverTick = true;
	//this->mFactoryTickFunction.bStartWithTickEnabled = true;
	//this->mFactoryTickFunction.bAllowTickOnDedicatedServer = true;
	//this->mFactoryTickFunction.TickInterval = 0;
	this->bReplicates = true;
	//this->NetCullDistanceSquared = 5624999936;

	//this->Registered = false;
}

void AApPortal::BeginPlay() {
	// for some reason we need to set the Power Info here again otherwise power doesn't work
	// maybe the game expects it to be named something specific -Robb
	Super::BeginPlay();

	if (!mPowerInfo) {
		mPowerInfo = Cast<UFGPowerInfoComponent>(GetComponentByClass(UFGPowerInfoComponent::StaticClass()));
	}

	mPowerInfo->OnHasPowerChanged.BindUFunction(this, "CheckPower");

	if (!HasAuthority()) {
		return;
	}
}

void AApPortal::CheckPower() {
	const auto factoryHasPower = Factory_HasPower();


}

void AApPortal::Factory_Tick(float dt) {
	Super::Factory_Tick(dt);

	// Fluid input logic, remove if not desired
	if (HasAuthority() && IsValid(this) && GetStorageInventory()) {
		if (Factory_HasPower()) {
		}
	}
}

#pragma optimize("", on)
