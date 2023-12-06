#include "Buildable/ApPortal.h"
#include "Subsystem/ApPortalSubsystem.h"
#include "Subsystem/ApGiftingSubsystem.h"

//TODO REMOVE
#pragma optimize("", off)

AApPortal::AApPortal() : Super() {
	mPowerInfoClass = UFGPowerInfoComponent::StaticClass();
	mSignificanceRange = 18000;
	MaxRenderDistance = -1;

	bReplicates = true;

	mPowerConsumption = 10;

	//TODO implelemt through ui
	targetPlayer.Name = TEXT("JarnoSF");
	targetPlayer.Team = 0;
}

void AApPortal::BeginPlay() {
	Super::BeginPlay();

	UWorld* world = GetWorld();
	portalSubsystem = AApPortalSubsystem::Get(world);
	giftingSubsystem = AApGiftingSubsystem::Get(world);

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
	if (input == nullptr || !input->IsConnected())
		return;

	TArray<FInventoryItem> items;
	TSubclassOf<UFGItemDescriptor> itemClass;

	if (!input->Factory_PeekOutput(items, itemClass) || items.Num() == 0)
		return;

	if (!((AApGiftingSubsystem*)giftingSubsystem)->CanSend(targetPlayer, items[0]))
		return; //block input
	
	FInventoryItem item;
	float offset;

	if (!input->Factory_GrabOutput(item, offset, itemClass))
		return;

	FInventoryStack stack;
	stack.Item = item;
	stack.NumItems = 1;

	((AApGiftingSubsystem*)giftingSubsystem)->EnqueueForSending(targetPlayer, stack);
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
