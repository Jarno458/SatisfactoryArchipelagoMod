#include "Buildable/ApPortal.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApReplicatedGiftingSubsystem.h"

//TODO REMOVE
#pragma optimize("", off)

AApPortal::AApPortal() : Super() {
	mPowerInfoClass = UFGPowerInfoComponent::StaticClass();

	bReplicates = true;

	mPowerConsumption = 10;

	targetPlayer = FApPlayer();
}

void AApPortal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AApPortal, targetPlayer);
}

void AApPortal::BeginPlay() {
	Super::BeginPlay();

	if (!HasAuthority())
		return;

	UWorld* world = GetWorld();
	portalSubsystem = AApPortalSubsystem::Get(world);
	replicatedGiftingSubsystem = AApReplicatedGiftingSubsystem::Get(world);

	mPowerInfo->OnHasPowerChanged.BindUFunction(this, "CheckPower");

	for (UFGFactoryConnectionComponent* connection : GetConnectionComponents()) {
		if (connection->GetDirection() == EFactoryConnectionDirection::FCD_INPUT)
			input = connection;
		else
			output = connection;
	}

	CheckPower(false);
}

bool AApPortal::CanProduce_Implementation() const {
	return HasPower();
}

void AApPortal::CheckPower(bool newHasPower) {
	if (!HasAuthority())
		return;

	if (IsValid(this) && Factory_HasPower()) {
		((AApPortalSubsystem*)portalSubsystem)->RegisterPortal(this);
	} else {
		if (!IsValid(this)) {
			((AApPortalSubsystem*)portalSubsystem)->UnRegisterPortal(this, FInventoryItem::NullInventoryItem);
			return;
		}
		
		FScopeTryLock lock(&outputLock);
		if (lock.IsLocked()) {
			((AApPortalSubsystem*)portalSubsystem)->UnRegisterPortal(this, nextItemToOutput);
		} else {
			((AApPortalSubsystem*)portalSubsystem)->UnRegisterPortal(this, FInventoryItem::NullInventoryItem);
		}
	}
}

void AApPortal::Factory_Tick(float dt) {
	Super::Factory_Tick(dt);

	if (!HasAuthority())
		return;

	camReceiveOutput = IsValid(this) && CanProduce() && IsValid(output) && output->IsConnected();
}

bool AApPortal::TrySetOutput(FInventoryItem item) {
	if (!IsValid(this) || !camReceiveOutput)
		return false;

	FScopeTryLock lock(&outputLock);

	if (!lock.IsLocked() || nextItemToOutput.IsValid())
		return false;

	nextItemToOutput = item;
	return true;
}

void AApPortal::Factory_CollectInput_Implementation() {
	if (((AApReplicatedGiftingSubsystem*)replicatedGiftingSubsystem)->GetState() != EApGiftingServiceState::Ready 
		|| !IsValid(input)
		|| !input->IsConnected() 
		|| !targetPlayer.IsValid())
			return;

	TArray<FInventoryItem> items;
	if (!input->Factory_PeekOutput(items) || items.Num() == 0)
		return;

	if (!((AApReplicatedGiftingSubsystem*)replicatedGiftingSubsystem)->CanSend(targetPlayer, items[0].GetItemClass()))
		return; //block input
	
	FInventoryItem item;
	float offset;

	if (!input->Factory_GrabOutput(item, offset))
		return;

	FInventoryStack stack;
	stack.Item = item;
	stack.NumItems = 1;

	((AApPortalSubsystem*)portalSubsystem)->Send(targetPlayer, stack);
}

bool AApPortal::Factory_PeekOutput_Implementation(const class UFGFactoryConnectionComponent* connection, TArray<FInventoryItem>& out_items, TSubclassOf<UFGItemDescriptor> type) const {
	if (!Factory_HasPower())
		return false;
	
	FScopeTryLock lock(&outputLock);
	if (lock.IsLocked() && nextItemToOutput.IsValid()) {
		out_items.Emplace(nextItemToOutput);
		return true;
	} else {
		return false;
	}
}

bool AApPortal::Factory_GrabOutput_Implementation(class UFGFactoryConnectionComponent* connection, FInventoryItem& out_item, float& out_OffsetBeyond, TSubclassOf<UFGItemDescriptor> type) {
	out_OffsetBeyond = 0;
	
	if (!Factory_HasPower())
		return false;

	//hardlock we need to yield some output here
	FScopeTryLock lock(&outputLock);
	if (lock.IsLocked() && nextItemToOutput.IsValid()) {
		out_item = nextItemToOutput;
		nextItemToOutput = FInventoryItem::NullInventoryItem;
		return true;
	} else {
		out_item = FInventoryItem::NullInventoryItem;
		return false;
	}
}

#pragma optimize("", on)
