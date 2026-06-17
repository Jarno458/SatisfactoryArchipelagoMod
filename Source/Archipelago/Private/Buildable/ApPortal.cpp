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
	DOREPLIFETIME(AApPortal, configuredOutputFilter);
}

void AApPortal::BeginPlay() {
	Super::BeginPlay();

	if (!HasAuthority())
		return;

	mInputInventory->SetReplicationRelevancyOwner(this);
	mOutputInventory->SetReplicationRelevancyOwner(this);

	mInputInventory->mItemFilter.BindUObject(this, &AApPortal::FilterInventoryClasses);

	UWorld* world = GetWorld();
	portalSubsystem = AApPortalSubsystem::Get(world);
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
}

bool AApPortal::CanProduce_Implementation() const {
	return HasPower();
}

void AApPortal::ServerSetAllowedOutput(const TArray<TSubclassOf<UFGItemDescriptor>>& newAllowedOutput)
{
	if (!HasAuthority())
		return;

	configuredOutputFilter = newAllowedOutput;

	if (newAllowedOutput.Contains(WildcardDescriptor))
	{
		usedOutputFilter = vaultSubsystem->GetAllAcceptedItems();
	}
	else
	{
		usedOutputFilter = configuredOutputFilter;
	}
}

void AApPortal::ServerSetTarget(const FApPlayer& player)
{
	if (!HasAuthority())
		return;

	ServerSendManually();

	targetPlayer = player;
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

		if (usedOutputFilter.IsEmpty())
			return;

		const int32 startIndex = roundRobinIndex;
		for (int32 i = 0; i < usedOutputFilter.Num(); ++i)
		{
			const int32 currentIndex = (startIndex + i) % usedOutputFilter.Num();
			roundRobinIndex = (currentIndex + 1) % usedOutputFilter.Num();

			if (vaultSubsystem->TryGetSingleItem(usedOutputFilter[currentIndex]))
			{
				mOutputInventory->AddStackToIndex_Unsafe(0, FInventoryStack(1, usedOutputFilter[currentIndex]));
				break;
			}
		}
	}
}

void AApPortal::Factory_CollectInput_Implementation() {
	PullItemFromConnectionToInventory(input, mInputInventory);
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

	portalSubsystem->SendBuffer(targetPlayer, itemsToSend);
}

bool AApPortal::FilterInventoryClasses(TSubclassOf<UObject> object, int32 idx) const
{
	TSubclassOf<UFGItemDescriptor> itemClass = TSubclassOf<UFGItemDescriptor>(object);

	return IsValid(portalSubsystem) 
		&& targetPlayer.IsValid() 
		&& IsValid(itemClass) 
		&& portalSubsystem->DoesPlayerAccept(targetPlayer, TSubclassOf<UFGItemDescriptor>(object));
}
