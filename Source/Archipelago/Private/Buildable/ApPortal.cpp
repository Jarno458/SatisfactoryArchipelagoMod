#include "Buildable/ApPortal.h"
#include "Subsystem/ApPortalSubsystem.h"

DEFINE_LOG_CATEGORY(LogGame); // A base-game header is using this category so we must do this to avoid unresolved external symbol

//TODO REMOVE
#pragma optimize("", off)

AApPortal::AApPortal() : Super() {
	mPowerInfoClass = UFGPowerInfoComponent::StaticClass();
	mInventorySizeX = 5;
	mInventorySizeY = 1;
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

	mPowerConsumption = 10;
}

void AApPortal::BeginPlay() {
	Super::BeginPlay();

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

	//Factory_StartProducing();

	

	//TODO lock item from getting inserted until we implement sending
	UFGInventoryComponent* inventory = GetStorageInventory();
	//inventory->SetLocked(true);
}

void AApPortal::EndPlay(const EEndPlayReason::Type reason) {
	Super::EndPlay(reason);

	if (!HasAuthority()) {
		return;
	}
}

/*bool AApPortal::IsConfigured() const {
	return true;
}*/

/*bool AApPortal::Factory_HasPower() const {
	bool hasPowah = Super::Factory_HasPower();

	CheckPower(false);

	return hasPowah;
}*/

/*bool AApPortal::Factory_IsProducing() const {
	return true;
}*/

bool AApPortal::CanProduce_Implementation() const {
	return true;
}

void AApPortal::CheckPower(bool newHasPower) {
	if (Factory_HasPower()) {
		AApPortalSubsystem::Get(GetWorld())->RegisterPortal(this);
	}	else {
		AApPortalSubsystem::Get(GetWorld())->UnRegisterPortal(this);
	}
}

void AApPortal::Factory_Tick(float dt) {
	Super::Factory_Tick(dt);

	/*UFGInventoryComponent* inventory = GetStorageInventory();

	if (inventory != nullptr) {
		if (targetPlayerSlot <= 0)
			inventory->SetLocked(true);
		else
			inventory->SetLocked(false);
	}*/
}

bool AApPortal::Factory_PeekOutput_Implementation(const class UFGFactoryConnectionComponent* connection, TArray<FInventoryItem>& out_items, TSubclassOf<UFGItemDescriptor> type) const {
	if (!Factory_HasPower() || outputQueue.IsEmpty())
		return false;
	
	FInventoryItem item = *outputQueue.Peek();
	
	out_items.Emplace(item);
	
	return true;
}

bool AApPortal::Factory_GrabOutput_Implementation(class UFGFactoryConnectionComponent* connection, FInventoryItem& out_item, float& out_OffsetBeyond, TSubclassOf<UFGItemDescriptor> type) {
	out_OffsetBeyond = 0;
	
	return Factory_HasPower() && outputQueue.Dequeue(out_item);
}

#pragma optimize("", on)
