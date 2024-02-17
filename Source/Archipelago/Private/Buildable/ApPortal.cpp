#include "Buildable/ApPortal.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApServerGiftingSubsystem.h"
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

	UWorld* world = GetWorld();
	portalSubsystem = AApPortalSubsystem::Get(world);
	giftingSubsystem = AApServerGiftingSubsystem::Get(world);
	mostlyClientSideGiftingSubsystem = AApReplicatedGiftingSubsystem::Get(world);

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

	CheckPower(false);
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

	camReceiveOutput = CanProduce() && output->IsConnected();
}

void AApPortal::Factory_CollectInput_Implementation() {
	if (input == nullptr || !input->IsConnected() || !targetPlayer.IsValid())
		return;

	TArray<FInventoryItem> items;
	if (!input->Factory_PeekOutput(items) || items.Num() == 0)
		return;

	if (!((AApReplicatedGiftingSubsystem*)mostlyClientSideGiftingSubsystem)->CanSend(targetPlayer, items[0].GetItemClass()))
		return; //block input
	
	FInventoryItem item;
	float offset;

	if (!input->Factory_GrabOutput(item, offset))
		return;

	FInventoryStack stack;
	stack.Item = item;
	stack.NumItems = 1;

	((AApServerGiftingSubsystem*)giftingSubsystem)->EnqueueForSending(targetPlayer, stack);
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
