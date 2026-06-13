#include "Buildable/ApPortal.h"

#include "Misc/ScopeTryLock.h"
#include "Net/UnrealNetwork.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApReplicatedGiftingSubsystem.h"

AApPortal::AApPortal() : Super() {
	bReplicates = true;

	SetmPowerConsumption(10);

	mInputInventory = CreateDefaultSubobject<UFGInventoryComponent>(TEXT("InputInventory"));
	mInputInventory->SetDefaultSize(10);

	targetPlayer = FApPlayer();

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	//needs to thick atleast 1200 times per minute to keep up with a 1200 belt, 1200 / 60sec = 20x per second
	//0.048 is ~20.8 times per second
	PrimaryActorTick.TickInterval = 0.048f;
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
	vaultSubsystem = AApVaultSubsystem::Get(world);

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

/*
 * Plan
 * 
 * Every tick, as time critical get items vault
 * Compare to filter and select a random availble item
 * Assign it as nextItemToOutput
 * 
 * TODO Find a way to not make everyone slap a portal on each miner
 * // Maybe simply disallow sending directly to your own personal vault
 *
 * TODO default item filter for new building should be set to block all output None
 */


void AApPortal::Factory_Tick(float dt) {
	Super::Factory_Tick(dt);

	if (!HasAuthority())
		return;

	FScopeTryLock lock(&outputLock);
	if (!lock.IsLocked())
		return;

	camReceiveOutput = IsValid(this) && CanProduce() && IsValid(output) && output->IsConnected();

	if (!camReceiveOutput) {
		if (nextItemToOutput.IsValid())
			vaultSubsystem->Store(FItemAmount(nextItemToOutput.GetItemClass(), 1), true);

		nextItemToOutput = FInventoryItem::NullInventoryItem;

	} else {
		if (nextItemToOutput.IsValid())
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
				nextItemToOutput = FInventoryItem(allowedOutput[currentIndex]);
				break;
			}
		}
	}
}

void AApPortal::SetTarget(const FApPlayer& player)
{
	 //TODO handle cleaning of inventory
	 // send current inventory

	targetPlayer = player;
}

/*
bool AApPortal::TrySetOutput(const FInventoryItem& item) {
	if (!IsValid(this) || !camReceiveOutput)
		return false;

	FScopeTryLock lock(&outputLock);

	if (!lock.IsLocked() || nextItemToOutput.IsValid())
		return false;

	nextItemToOutput = item;
	return true;
}
*/

void AApPortal::Factory_CollectInput_Implementation() {
	if (static_cast<AApReplicatedGiftingSubsystem*>(replicatedGiftingSubsystem)->GetState() != EApGiftingServiceState::Ready
		|| !IsValid(input)
		|| !input->IsConnected()
		|| !targetPlayer.IsValid())
		return;

	TArray<FInventoryItem> items;
	if (!input->Factory_PeekOutput(items) || items.Num() == 0)
		return;

	if (static_cast<AApVaultSubsystem*>(vaultSubsystem)->DoesPlayerAcceptVaultItems(targetPlayer))
	{
		if (!static_cast<AApVaultSubsystem*>(vaultSubsystem)->CanSend(targetPlayer, items[0].GetItemClass()))
			return; //block input
	} else if (!static_cast<AApReplicatedGiftingSubsystem*>(replicatedGiftingSubsystem)->CanSend(targetPlayer, items[0].GetItemClass()))
		return; //block input

	UFGInventoryComponent* targetInventory = input->GetInventory();

	if (targetInventory->HasEnoughSpaceForItem(items[0]))
	{
		FInventoryItem item;
		float offset;

		if (!input->Factory_GrabOutput(item, offset))
			return;

		targetInventory->AddItem(item);
	}
}


void AApPortal::ServerSendManually() {
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
