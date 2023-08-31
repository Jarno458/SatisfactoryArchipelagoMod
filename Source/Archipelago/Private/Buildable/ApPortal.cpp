#include "Buildable/ApPortal.h"

DEFINE_LOG_CATEGORY(LogGame); // A base-game header is using this category so we must do this to avoid unresolved external symbol

//TODO REMOVE
#pragma optimize("", off)

AApPortal::AApPortal() : Super() {
	mPowerInfoClass = UFGPowerInfoComponent::StaticClass();
	mInventorySizeX = 20;
	mInventorySizeY = 20;
	mSignificanceRange = 18000;
	MaxRenderDistance = -1;

	//this->mFactoryTickFunction.TickGroup = TG_PrePhysics;
	//this->mFactoryTickFunction.EndTickGroup = TG_PrePhysics;
	// 
	//mFactoryTickFunction.bTickEvenWhenPaused = false;
	//mFactoryTickFunction.bCanEverTick = true;
	//mFactoryTickFunction.bStartWithTickEnabled = true;
	//mFactoryTickFunction.bAllowTickOnDedicatedServer = true;
	//mFactoryTickFunction.TickInterval = 0;

	bReplicates = true;
	//this->NetCullDistanceSquared = 5624999936;

	//this->Registered = false;

	mPowerConsumption = 10;
}

void AApPortal::BeginPlay() {
	// for some reason we need to set the Power Info here again otherwise power doesn't work
	// maybe the game expects it to be named something specific -Robb
	Super::BeginPlay();

	apSubSystem = AApSubsystem::Get(GetWorld());

	mPowerInfo = Cast<UFGPowerInfoComponent>(GetComponentByClass(UFGPowerInfoComponent::StaticClass()));
	mPowerInfo->OnHasPowerChanged.BindUFunction(this, "CheckPower");
	
	for (UFGFactoryConnectionComponent* connection : GetConnectionComponents()) {
		if (connection->GetDirection() == EFactoryConnectionDirection::FCD_INPUT)
			input = connection;
		else
			output = connection;
	}

	if (!HasAuthority()) {
		return;
	}
}

void AApPortal::CheckPower() {
	bool factoryHasPower = Factory_HasPower();

	if (factoryHasPower)	{
		input->SetActive(false, true);
		output->SetActive(true, true);
	} else {
		input->SetActive(false, true);
		output->SetActive(false, true);
	}
}

void AApPortal::Factory_Tick(float dt) {
	Super::Factory_Tick(dt);

	UFGInventoryComponent* inventory = GetStorageInventory();

	if (inventory != nullptr) {
		TSubclassOf<UFGItemDescriptor> item;

		if (apSubSystem->PortalItems.Dequeue(item)) {
			int stackSize = UFGItemDescriptor::GetStackSize(item);

			FInventoryStack stack(stackSize, item);

			int added = inventory->AddStack(stack, true);

			if (added != stackSize) {
				//Tobad!!
			}
		}
	}


	// Fluid input logic, remove if not desired
	/*if (HasAuthority() && IsValid(this) && GetStorageInventory()) {
		if (Factory_HasPower()) {
			if (output->IsConnected()) {
				if (GetStorageInventory()) {
					//output->

				}
			}
		}
	}*/
			/*
			 if (Pipe->IsConnected()) {
				if (GetStorageInventory() && GetStorageInventory()->IsSomethingOnIndex(0)) {
					FInventoryStack CurrentItem; GetStorageInventory()->GetStackFromIndex(0, CurrentItem);
					if (CurrentItem.Item.GetItemClass()) {
						FInventoryStack Stack;
						Pipe->Factory_PullPipeInput(dt, Stack, CurrentItem.Item.GetItemClass(), FMath::Clamp((5000.f * mFluidStackSizeMultiplier) - CurrentItem.NumItems, 0.f, 300.f * dt));
						if (Stack.HasItems()) {
							GetStorageInventory()->AddStackToIndex(0, Stack);
						}
					}
				}
			*/
}

/*bool AApPortal::Factory_PeekOutput_Implementation(const class UFGFactoryConnectionComponent* connection, TArray<FInventoryItem>& out_items, TSubclassOf<UFGItemDescriptor> type) const {
	
	
	
	return false;
}

bool AApPortal::Factory_GrabOutput_Implementation(class UFGFactoryConnectionComponent* connection, FInventoryItem& out_item, float& out_OffsetBeyond, TSubclassOf<UFGItemDescriptor> type) {
	
	
	
	
	
	
	return false;
}*/

#pragma optimize("", on)
