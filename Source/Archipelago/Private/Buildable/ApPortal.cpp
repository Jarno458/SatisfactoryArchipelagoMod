#include "Buildable/ApPortal.h"

#include "FGPipeConnectionComponent.h"
#include "FGPowerInfoComponent.h"

DEFINE_LOG_CATEGORY(LogGame); // A base-game header is using this category so we must do this to avoid unresolved external symbol

//TODO REMOVE
#pragma optimize("", off)

AApPortal::AApPortal() : Super() {
	this->mPowerInfoClass = UFGPowerInfoComponent::StaticClass();
	this->mInventorySizeX = 1;
	this->mInventorySizeY = 1;
	this->mPowerConsumptionExponent = 1.6;
	this->mFluidStackSizeDefault = EStackSize::SS_FLUID;
	this->mSignificanceRange = 18000;
	this->MaxRenderDistance = -1;
	this->mFactoryTickFunction.TickGroup = TG_PrePhysics;
	this->mFactoryTickFunction.EndTickGroup = TG_PrePhysics;
	this->mFactoryTickFunction.bTickEvenWhenPaused = false;
	this->mFactoryTickFunction.bCanEverTick = true;
	this->mFactoryTickFunction.bStartWithTickEnabled = true;
	this->mFactoryTickFunction.bAllowTickOnDedicatedServer = true;
	this->mFactoryTickFunction.TickInterval = 0;
	this->bReplicates = true;
	this->NetCullDistanceSquared = 5624999936;

	this->Registered = false;
}

void AApPortal::BeginPlay() {
	// for some reason we need to set the Power Info here again otherwise power doesn't work -Nog
	// maybe the game expects it to be named something specific -Robb
	Super::BeginPlay();
	if (!mPowerInfo) {
		mPowerInfo = Cast< UFGPowerInfoComponent>(GetComponentByClass(UFGPowerInfoComponent::StaticClass()));
	}

	mPowerInfo->OnHasPowerChanged.BindUFunction(this, "CheckPower");

	// TODO register with subsystem
	//SManager = ANogsResearchSubsystem::Get(this->GetWorld());

	if (!HasAuthority()) {
		return;
	}

	// Change to a fluid slot if our pipe connection is configured as a consumer, remove if not desired
	FOR_EACH_PIPE_INLINE_COMPONENTS(connection) {
		if (connection->GetPipeConnectionType() == EPipeConnectionType::PCT_CONSUMER) {
			connection->SetInventory(GetStorageInventory());
			connection->SetInventoryAccessIndex(0);
			Pipe = connection;
			GetStorageInventory()->AddArbitrarySlotSize(0, 5000 * mFluidStackSizeMultiplier);
		}
	}
}

void AApPortal::CheckPower() {
	const auto factoryHasPower = Factory_HasPower();
	// TODO registration code, this is copied from Nog's Research
	//if (factoryHasPower) {
	//	if (!Registered) {
	//		if (SManager) {
	//			UE_LOG(LogNogsResearchCpp, Warning, TEXT("Registering researcher %s"), *GetName());
	//			Registered = true;
	//			SManager->RegisterResearcher(this);
	//			ProductionStateChanged();
	//		}
	//	}
	//} else {
	//	if (Registered) {
	//		if (SManager) {
	//			UE_LOG(LogNogsResearchCpp, Warning, TEXT("UnRegistering researcher %s"), *GetName());
	//			Registered = false;
	//			SManager->UnRegisterResearcher(this);
	//			ProductionStateChanged();
	//		}
	//	}
	//}
}

void AApPortal::Factory_Tick(float dt) {
	Super::Factory_Tick(dt);

	// Fluid input logic, remove if not desired
	if (HasAuthority() && IsValid(this) && Pipe && GetStorageInventory()) {
		if (Factory_HasPower()) {
			if (Pipe->IsConnected()) {
				if (GetStorageInventory()->IsSomethingOnIndex(0)) {
					FInventoryStack CurrentItem; GetStorageInventory()->GetStackFromIndex(0, CurrentItem);
					if (CurrentItem.Item.GetItemClass()) {
						FInventoryStack Stack;
						Pipe->Factory_PullPipeInput(dt, Stack, CurrentItem.Item.GetItemClass(), FMath::Clamp((5000.f * mFluidStackSizeMultiplier) - CurrentItem.NumItems, 0.f, 300.f * dt));
						if (Stack.HasItems()) {
							GetStorageInventory()->AddStackToIndex(0, Stack);
						}
					}
				} else {
					FInventoryStack Stack;
					Pipe->Factory_PullPipeInput(dt, Stack, nullptr, FMath::Clamp(5000.f * mFluidStackSizeMultiplier, 0.f, 300.f * dt));
					if (Stack.HasItems()) {
						GetStorageInventory()->AddStackToIndex(0, Stack);
					}
				}
			}
		}
	}
}

#pragma optimize("", on)
