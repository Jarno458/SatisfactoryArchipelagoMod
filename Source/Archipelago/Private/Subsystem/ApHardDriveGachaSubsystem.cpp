#include "Subsystem/ApHardDriveGachaSubsystem.h"
#include "Patching/NativeHookManager.h"
#include "Registry/ModContentRegistry.h"
#include "Subsystem/ApSchematicPatcherSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "Subsystem/ApSlotDataSubsystem.h"

DEFINE_LOG_CATEGORY(LogApHardDriveGachaSubsystem);

//TODO REMOVE
#pragma optimize("", off)

AApHardDriveGachaSubsystem::AApHardDriveGachaSubsystem() : Super() {
	PrimaryActorTick.bCanEverTick = false;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

AApHardDriveGachaSubsystem* AApHardDriveGachaSubsystem::Get(UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApHardDriveGachaSubsystem>();
}

void AApHardDriveGachaSubsystem::Initialize(const TArray<TSubclassOf<UFGSchematic>>& hardDriveSchematics) {
	UE_LOG(LogApHardDriveGachaSubsystem, Display, TEXT("AApSubsystem()::Initialize()"));

	apHardDriveSchematics = hardDriveSchematics;

	//we cannot trust the order of this array to be in order of check priority, so we re-sort it
	UFGSchematic::SortByMenuPriority(apHardDriveSchematics);
}

void AApHardDriveGachaSubsystem::BeginPlay() {
	UE_LOG(LogApHardDriveGachaSubsystem, Display, TEXT("AApHardDriveGachaSubsystem::BeginPlay()"));
	Super::BeginPlay();

	UWorld* world = GetWorld();

	AApSlotDataSubsystem* slotDataSubsystem = AApSlotDataSubsystem::Get(world);
	fgcheck(slotDataSubsystem);

	//risky to assume slot data is always ready when BeginPlay is called but i think it will work..
	if (!slotDataSubsystem->HasLoadedSlotData() || !slotDataSubsystem->EnableHardDriveGacha) {
		UE_LOG(LogApHardDriveGachaSubsystem, Display, TEXT("AApHardDriveGachaSubsystem::BeginPlay() Hard Drive Gacha disabled"));
		return;
	}

	contentRegistry = UModContentRegistry::Get(world);
	fgcheck(contentRegistry);
	RManager = AFGResearchManager::Get(world);
	fgcheck(RManager);

	AFGSchematicManager* SManager = AFGSchematicManager::Get(world);
	fgcheck(SManager);
	SManager->PurchasedSchematicDelegate.AddDynamic(this, &AApHardDriveGachaSubsystem::OnSchematicCompleted);

	if (!hooksInitialized) {
		hookHandler = SUBSCRIBE_METHOD(AFGResearchManager::GetAvailableAlternateSchematics, [this](auto& scope, const AFGResearchManager* self, TArray<TSubclassOf<UFGSchematic>> excludedSchematics, int32 numSchematics, TArray<TSubclassOf<UFGSchematic>>& out_schematics) {
			return GetAvailableAlternateSchematics(scope, self, excludedSchematics, numSchematics, out_schematics);
		});

		hooksInitialized = true;
	}
}

void AApHardDriveGachaSubsystem::EndPlay(const EEndPlayReason::Type endPlayReason) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApHardDriveGachaSubsystem::EndPlay(%i)"), endPlayReason);

	Super::EndPlay(endPlayReason);

	if (hookHandler.IsValid())
		UNSUBSCRIBE_METHOD(AFGResearchManager::GetAvailableAlternateSchematics, hookHandler);
}

bool AApHardDriveGachaSubsystem::GetAvailableAlternateSchematics(
	TCallScope<bool(*)(const AFGResearchManager* self, TArray<TSubclassOf<UFGSchematic>>, int32, TArray<TSubclassOf<UFGSchematic>>&)>& Scope,
	const AFGResearchManager* self, TArray<TSubclassOf<UFGSchematic>> excludedSchematics, int32 numSchematics, TArray<TSubclassOf<UFGSchematic>>& out_schematics) {

	UE_LOG(LogApHardDriveGachaSubsystem, Display, TEXT("AApHardDriveGachaSubsystem::GetAvailableAlternateSchematics()"));

	// run normal schematic selection to give external mod schematics a chance
	bool result = Scope(self, excludedSchematics, numSchematics, out_schematics);

	//Schematic should be inside apHardDriveSchematics (that the list of schematics to still unlock)
	//Schematic should not be in excludedSchematics (those are the previus ones on a reroll)
	//Schematic should not already be pending for selection in AFGResearchManager::GetUnclaimedHardDrives(TArray<UFGHardDrive*>& out_HardDrives)
	//Schematic should not already be already be selected to offer
	TSet<TSubclassOf<UFGSchematic>> schematicsToExclude;

	schematicsToExclude.Append(excludedSchematics);

	//i could not get AFGResearchManager::GetPendingRewards to work
	TArray<UFGHardDrive*> unclaimedHarddrives;
	RManager->GetUnclaimedHardDrives(unclaimedHarddrives);
	for (UFGHardDrive* pendingHarddrive : unclaimedHarddrives) {
		if (!IsValid(pendingHarddrive))
			continue;

		TArray<TSubclassOf<UFGSchematic>> pendingSchematics;
		pendingHarddrive->GetSchematics(pendingSchematics);

		schematicsToExclude.Append(pendingSchematics);
	}

	for (int i=0; i < out_schematics.Num(); i++) {
		if (IsExternalModSchematic(out_schematics[i])) {
			// if its a external mod schematic (aka a schematic added by an other mod), preserve it
		}
		else {
			//if its an AP harddrive schematic, replace it according to progresion logic
			TSubclassOf<UFGSchematic> randomlySelectedSchematic = GetRandomSchematic(schematicsToExclude);

			if (randomlySelectedSchematic != nullptr) {
				out_schematics[i] = randomlySelectedSchematic;

				schematicsToExclude.Add(randomlySelectedSchematic);
			}
		}
	}

	return result;
}

TSubclassOf<class UFGSchematic> AApHardDriveGachaSubsystem::GetRandomSchematic(TSet<TSubclassOf<UFGSchematic>> excludedSchematics) {
	//Schematic should be randomly selected from the first x schematics in apHardDriveSchematics that arent excluded

	TArray<TSubclassOf<class UFGSchematic>> schematicsToOffer;
	for (TSubclassOf<class UFGSchematic> schematic : apHardDriveSchematics) {
		if (excludedSchematics.Contains(schematic))
			continue;

		schematicsToOffer.Add(schematic);

		if (schematicsToOffer.Num() >= bucketSize)
			break;
	}

	if (schematicsToOffer.Num() == 0)
		return nullptr;

	int index = FMath::RandRange(0, schematicsToOffer.Num() - 1);

	return schematicsToOffer[index];
}

bool AApHardDriveGachaSubsystem::IsExternalModSchematic(TSubclassOf<class UFGSchematic> schematic) {
	FGameObjectRegistration registration;
	return !(UFGSchematic::GetMenuPriority(schematic) >= 1338600 && UFGSchematic::GetMenuPriority(schematic) <= 1338699)
		&& contentRegistry->GetSchematicRegistrationInfo(schematic, registration)
		&& !registration.HasAnyFlags(EGameObjectRegistrationFlags::BuiltIn);
}

void AApHardDriveGachaSubsystem::OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(LogApHardDriveGachaSubsystem, Display, TEXT("AApHardDriveGachaSubsystem::OnSchematicCompleted()"));

	if (UFGSchematic::GetType(schematic) != ESchematicType::EST_Alternate)
		return;

	apHardDriveSchematics.Remove(schematic);
}

#pragma optimize("", on)

