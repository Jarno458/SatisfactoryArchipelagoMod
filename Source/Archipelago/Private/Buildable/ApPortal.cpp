#include "Buildable/ApPortal.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApReplicatedGiftingSubsystem.h"

#pragma optimize("", off)

AApPortal::AApPortal() : Super() {
	bReplicates = true;

	mPowerConsumption = 10;

	mInputInventory = CreateDefaultSubobject<UFGInventoryComponent>(TEXT("InputInventory"));
	mInputInventory->SetDefaultSize(10);

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

	mInputInventory->SetReplicationRelevancyOwner(this);
	mInputInventory->mItemFilter.BindUObject(this, &AApPortal::FilterInventoryClasses);

	UWorld* world = GetWorld();
	portalSubsystem = AApPortalSubsystem::Get(world);
	replicatedGiftingSubsystem = AApReplicatedGiftingSubsystem::Get(world);

	for (UFGFactoryConnectionComponent* connection : GetConnectionComponents()) {
		if (connection->GetDirection() == EFactoryConnectionDirection::FCD_INPUT) {
			input = connection;

			connection->SetInventory(mInputInventory);
		}
		else
		{
			output = connection;
		}
	}
}

bool AApPortal::CanProduce_Implementation() const {
	return HasPower();
}

void AApPortal::Factory_Tick(float dt) {
	Super::Factory_Tick(dt);

	if (!HasAuthority())
		return;

	camReceiveOutput = IsValid(this) && CanProduce() && IsValid(output) && output->IsConnected();

	if (!camReceiveOutput) {
		FScopeTryLock lock(&outputLock);
		if (lock.IsLocked()) {
			if (nextItemToOutput.IsValid())
				static_cast<AApPortalSubsystem*>(portalSubsystem)->ReQueue(nextItemToOutput);

			nextItemToOutput = FInventoryItem::NullInventoryItem;
		}
	}
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
	if (static_cast<AApReplicatedGiftingSubsystem*>(replicatedGiftingSubsystem)->GetState() != EApGiftingServiceState::Ready
		|| !IsValid(input)
		|| !input->IsConnected()
		|| !targetPlayer.IsValid())
		return;

	TArray<FInventoryItem> items;
	if (!input->Factory_PeekOutput(items) || items.Num() == 0)
		return;

	//TODO input filtering based on target player accepted traits

	UFGInventoryComponent* targetInventory = input->GetInventory();

	if (targetInventory->HasEnoughSpaceForItem(items[0]))
	{
		FInventoryItem item;
		float offset;

		if (!input->Factory_GrabOutput(item, offset))
			return;

		targetInventory->AddItem(item);
	}

	//if (!input->Factory_GrabOutput(item, offset))
	//	return;

	/*
	if (!static_cast<AApReplicatedGiftingSubsystem*>(replicatedGiftingSubsystem)->CanSend(targetPlayer, items[0].GetItemClass()))
		return; //block input

	FInventoryItem item;
	float offset;

	if (!input->Factory_GrabOutput(item, offset))
		return;

	FItemAmount stack;
	stack.ItemClass = item.GetItemClass();
	stack.Amount = 1;

	static_cast<AApPortalSubsystem*>(portalSubsystem)->Send(targetPlayer, stack);
	*/
}


void AApPortal::ServerSendManually() {
	if (!IsValid(this))
		return;

	TArray<FInventoryStack> stacks;
	mInputInventory->GetInventoryStacks(stacks);

	TArray<FItemAmount> itemsToSend;

	for (FInventoryStack& Stack : stacks)
	{
		if (Stack.NumItems <= 0)
			continue;

		bool found = false;

		for (FItemAmount& ItemsToSend : itemsToSend)
		{
			if (ItemsToSend.ItemClass == Stack.Item.GetItemClass())
			{
				ItemsToSend.Amount += Stack.NumItems;
				found = true;
				break;
			}
		}

		if (!found)
		{
			FItemAmount newItem;
			newItem.ItemClass = Stack.Item.GetItemClass();
			newItem.Amount = Stack.NumItems;
			itemsToSend.Add(newItem);
		}
	}

	static_cast<AApPortalSubsystem*>(portalSubsystem)->SendBuffer(targetPlayer, itemsToSend);
}

bool AApPortal::Factory_PeekOutput_Implementation(const class UFGFactoryConnectionComponent* connection, TArray<FInventoryItem>& out_items, TSubclassOf<UFGItemDescriptor> type) const {
	if (!Factory_HasPower())
		return false;

	FScopeTryLock lock(&outputLock);
	if (lock.IsLocked() && nextItemToOutput.IsValid()) {
		out_items.Emplace(nextItemToOutput);
		return true;
	}
	else {
		return false;
	}
}


bool AApPortal::Factory_GrabOutput_Implementation(class UFGFactoryConnectionComponent* connection, FInventoryItem& out_item, float& out_OffsetBeyond, TSubclassOf<UFGItemDescriptor> type) {
	out_OffsetBeyond = 0;

	if (!Factory_HasPower()) {
		return false;
	}

	//hardlock we need to yield some output here
	FScopeTryLock lock(&outputLock);
	if (lock.IsLocked() && nextItemToOutput.IsValid()) {
		out_item = nextItemToOutput;
		nextItemToOutput = FInventoryItem::NullInventoryItem;
		return true;
	}
	else {
		return false;
	}
}


bool AApPortal::FilterInventoryClasses(TSubclassOf<UObject> object, int32 idx) const
{
	//TODO filter on player accepted traits
	return true;
}

#pragma optimize("", on)
