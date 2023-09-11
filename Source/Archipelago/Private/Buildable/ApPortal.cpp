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

	bReplicates = true;

	mPowerConsumption = 10;
}

void AApPortal::BeginPlay() {
	Super::BeginPlay();

	portalSubsystem = AApPortalSubsystem::Get(GetWorld());

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

	inventory = GetStorageInventory();
	inventory->SetLocked(true);
}

void AApPortal::EndPlay(const EEndPlayReason::Type reason) {
	Super::EndPlay(reason);

	if (!HasAuthority()) {
		return;
	}
}

bool AApPortal::CanProduce_Implementation() const {
	return HasPower();
}

void AApPortal::CheckPower(bool newHasPower) const {
	if (Factory_HasPower()) {
		((AApPortalSubsystem*)portalSubsystem)->RegisterPortal(this);
	}	else {
		((AApPortalSubsystem*)portalSubsystem)->UnRegisterPortal(this);
	}
}

void AApPortal::Factory_Tick(float dt) {
	Super::Factory_Tick(dt);

	inventory = GetStorageInventory();
	if (inventory != nullptr) {
		if (targetPlayerSlot <= -1)
			inventory->SetLocked(true);
		else {
			inventory->SetLocked(false);

			TArray<FInventoryStack> stacks;
			inventory->GetInventoryStacks(stacks);

			for (FInventoryStack stack : stacks) {
				((AApPortalSubsystem*)portalSubsystem)->Send(targetPlayerSlot, stack);
			}

			inventory->Empty();
		}
	}

	camReceiveOutput = CanProduce() && output->IsConnected();
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
