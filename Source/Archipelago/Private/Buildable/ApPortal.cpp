#include "Buildable/ApPortal.h"

#include "Net/UnrealNetwork.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApReplicatedGiftingSubsystem.h"

/*
 * Plan
 *
 * Every factory tick,
 * Try to get 1 item from vault of our output inventory is empty
 * Compare to filter and select a random availble item
 * using a round robin approach to make sure every selected itemclass gets a chance to send items if there are multiple
 */

AApPortal::AApPortal() : Super() {
	bReplicates = true;

	SetmPowerConsumption(10);

	mInputInventory = CreateDefaultSubobject<UFGInventoryComponent>(TEXT("InputInventory"));
	mInputInventory->SetDefaultSize(10);

	mOutputInventory = CreateDefaultSubobject<UFGInventoryComponent>(TEXT("OutputInventory"));
	mOutputInventory->SetDefaultSize(1);

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
	mOutputInventory->SetReplicationRelevancyOwner(this);

	UWorld* world = GetWorld();
	portalSubsystem = AApPortalSubsystem::Get(world);
	replicatedGiftingSubsystem = AApReplicatedGiftingSubsystem::Get(world);
	vaultSubsystem = AApVaultSubsystem::Get(world);

	for (UFGFactoryConnectionComponent* connection : GetConnectionComponents()) {
		if (connection->GetDirection() == EFactoryConnectionDirection::FCD_INPUT) {
			input = connection;

			connection->SetInventory(mInputInventory);
		}
		else
		{
			output = connection;

			connection->SetInventory(mOutputInventory);
			connection->SetInventoryAccessIndex(0);
		}
	}

	 //TODO Remove for testing only
	AApMappingsSubsystem* mappingSubsystem = AApMappingsSubsystem::Get(world);
	
	allowedOutput.Empty();
	
	allowedOutput.Add(StaticCastSharedRef<FApItem>(mappingSubsystem->ApItems[1338087])->Class);  //quickwire
	allowedOutput.Add(StaticCastSharedRef<FApItem>(mappingSubsystem->ApItems[1338115])->Class);  //wire
	allowedOutput.Add(StaticCastSharedRef<FApItem>(mappingSubsystem->ApItems[1338017])->Class);  //cable
	allowedOutput.Add(StaticCastSharedRef<FApItem>(mappingSubsystem->ApItems[1338025])->Class);  //concrete
}

bool AApPortal::CanProduce_Implementation() const {
	return HasPower();
}

void AApPortal::Factory_Tick(float dt) {
	Super::Factory_Tick(dt);

	if (!HasAuthority())
		return;

	camReceiveOutput = IsValid(this) && CanProduce() && IsValid(output) && output->IsConnected();

	FInventoryStack itemStack;
	bool hasItem = mOutputInventory->GetStackFromIndex(0, itemStack) && itemStack.HasItems();

	if (!camReceiveOutput) {
		if (hasItem)
		{
			vaultSubsystem->Store(FItemAmount(itemStack.Item.GetItemClass(), itemStack.NumItems), true);
			mOutputInventory->Empty();
		}
	}
	else {
		if (hasItem)
			return;

		if (allowedOutput.IsEmpty())
			return;

		const int32 startIndex = roundRobinIndex;
		for (int32 i = 0; i < allowedOutput.Num(); ++i)
		{
			const int32 currentIndex = (startIndex + i) % allowedOutput.Num();
			roundRobinIndex = (currentIndex + 1) % allowedOutput.Num();

			if (vaultSubsystem->TryGetSingleItem(allowedOutput[currentIndex]))
			{
				mOutputInventory->AddStackToIndex_Unsafe(0, FInventoryStack(1, allowedOutput[currentIndex]));
				break;
			}
		}
	}
}

void AApPortal::ServerSetTarget(const FApPlayer& player)
{
	ServerSendManually();

	targetPlayer = player;
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

	 TSubclassOf<UFGItemDescriptor> itemClass = items[0].GetItemClass();

	if (static_cast<AApVaultSubsystem*>(vaultSubsystem)->DoesPlayerAcceptVaultItems(targetPlayer))
	{
		if (!static_cast<AApVaultSubsystem*>(vaultSubsystem)->CanSend(targetPlayer, itemClass))
			return; //block input
	} else if (!static_cast<AApReplicatedGiftingSubsystem*>(replicatedGiftingSubsystem)->CanSend(targetPlayer, itemClass))
		return; //block input

	UFGInventoryComponent* targetInventory = input->GetInventory();

	if (targetInventory->HasEnoughSpaceForItem(items[0]))
	{
		FInventoryItem item;
		float offset;

		if (!input->Factory_GrabOutput(item, offset, itemClass))
			return;

		targetInventory->AddItem(item);
	}
}


void AApPortal::ServerSendManually() const
{
	if (!IsValid(this))
		return;

	TArray<FInventoryStack> stacks;
	mInputInventory->GetInventoryStacks(stacks);
	mInputInventory->Empty();

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

#pragma optimize("", on)
