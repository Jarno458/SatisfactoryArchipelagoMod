#include "Buildable/ApPortal.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApReplicatedGiftingSubsystem.h"

//TODO REMOVE
#pragma optimize("", off)

AApPortal::AApPortal() : Super() {
	mPowerInfoClass = UFGPowerInfoComponent::StaticClass();
	//mSignificanceRange = 18000;
	//MaxRenderDistance = -1;

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

	if (Factory_HasPower()) {
		((AApPortalSubsystem*)portalSubsystem)->RegisterPortal(this);
	}	else {
		((AApPortalSubsystem*)portalSubsystem)->UnRegisterPortal(this);
	}
}

void AApPortal::Factory_Tick(float dt) {
	Super::Factory_Tick(dt);

	if (!HasAuthority())
		return;

	camReceiveOutput = CanProduce() && output->IsConnected();
}

bool AApPortal::OutputIsEmpty() const {
	FScopeLock lock(&outputLock);
	return !nextItemToOutput.IsValid();
}

void AApPortal::SetOutput(FInventoryItem item) {
	FScopeLock lock(&outputLock);
	nextItemToOutput = item;
}

FInventoryItem AApPortal::StealOutput() {
	FScopeLock lock(&outputLock);
	FInventoryItem returnItem = nextItemToOutput;
	nextItemToOutput = FInventoryItem::NullInventoryItem;
	return returnItem;
}

void AApPortal::Factory_CollectInput_Implementation() {
	if (((AApReplicatedGiftingSubsystem*)replicatedGiftingSubsystem)->GetState() != EApGiftingSeriveState::Ready 
		|| input == nullptr 
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
	
	FScopeLock lock(&outputLock);
	if (nextItemToOutput.IsValid()) {
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

	FScopeLock lock(&outputLock);
	if (nextItemToOutput.IsValid()) {
		out_item = nextItemToOutput;
		nextItemToOutput = FInventoryItem::NullInventoryItem;
		return true;
	} else {
		return false;
	}
}

#pragma optimize("", on)
