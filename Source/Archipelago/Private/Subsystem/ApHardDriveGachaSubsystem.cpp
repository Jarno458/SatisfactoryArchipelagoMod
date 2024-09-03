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
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 1.0f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

AApHardDriveGachaSubsystem* AApHardDriveGachaSubsystem::Get(UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApHardDriveGachaSubsystem>();
}

void AApHardDriveGachaSubsystem::Initialize(const TArray<TSubclassOf<UFGSchematic>>& apSchematics) {
	UE_LOG(LogApHardDriveGachaSubsystem, Display, TEXT("AApSubsystem()::Initialize()"));

	apHardDriveSchematics = apSchematics;

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

	ap = AApSubsystem::Get(world);
	fgcheck(ap);
	contentRegistry = UModContentRegistry::Get(world);
	fgcheck(contentRegistry);
	SManager = AFGSchematicManager::Get(world);
	fgcheck(SManager);

	SManager->PurchasedSchematicDelegate.AddDynamic(this, &AApHardDriveGachaSubsystem::OnSchematicCompleted);

	if (!hooksInitialized) {
		SUBSCRIBE_METHOD(UFGHardDriveSettings::GetValidSchematicRewardDrops, [this](auto& scope, const UFGHardDriveSettings* self, AFGSchematicManager* schematicManager, TArray<TSubclassOf<class UFGSchematic>>& out_validSchematics) {
			GetValidSchematicRewardDrops(scope, self, schematicManager, out_validSchematics);
		});

		//TOOD maybe not needed
		//SUBSCRIBE_METHOD(UFGHardDriveSettings::GetFinalSchematicRewards, [this](auto& scope, const UFGHardDriveSettings* self, const TArray<TSubclassOf<class UFGSchematic>>& allValidSchematicDrops) {
		//	return GetFinalSchematicRewards(scope, self, allValidSchematicDrops);
		//});

		hooksInitialized = true;
	}
}

void AApHardDriveGachaSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void AApHardDriveGachaSubsystem::GetValidSchematicRewardDrops(
		TCallScope<void(*)(const UFGHardDriveSettings*, class AFGSchematicManager*, TArray<TSubclassOf<class UFGSchematic>>&)>& Scope, 
		const UFGHardDriveSettings* self, class AFGSchematicManager* schematicManager, TArray<TSubclassOf<class UFGSchematic>>& out_validSchematics) {
	UE_LOG(LogApHardDriveGachaSubsystem, Display, TEXT("AApHardDriveGachaSubsystem::GetValidSchematicRewardDrops()"));

	TArray<TSubclassOf<class UFGSchematic>> resultSchematics;

	Scope(self, schematicManager, resultSchematics);

	TArray<TSubclassOf<class UFGSchematic>> modSchematics;
	for (TSubclassOf<class UFGSchematic> schematic : resultSchematics) {
		FGameObjectRegistration registration;
		if (contentRegistry->GetSchematicRegistrationInfo(schematic, registration) 
					&& !registration.HasAnyFlags(EGameObjectRegistrationFlags::BuiltIn)
					&& !(UFGSchematic::GetMenuPriority(schematic) >= 1338600 && UFGSchematic::GetMenuPriority(schematic) <= 1338699))
			modSchematics.Add(schematic);
	}

	schematicsToOffer.Empty();

	int numSchematicsLeft = apHardDriveSchematics.Num();

	if (numSchematicsLeft >= 1) schematicsToOffer.Add(GetRandomSchematic(0, bucketSize));
	if (numSchematicsLeft >= 2) schematicsToOffer.Add(GetRandomSchematic(bucketSize, bucketSize * 2));
	if (numSchematicsLeft >= 3) schematicsToOffer.Add(GetRandomSchematic(bucketSize * 2, bucketSize * 3));

	if (modSchematics.Num() > 0)	{
		UFGGlobalSettings::GetHardDriveSettingsCDO()->mUniqueItemCount = 4;

		int index = FMath::RandRange(0, modSchematics.Num() - 1);
		schematicsToOffer.Add(modSchematics[index]);
	} else {
		UFGGlobalSettings::GetHardDriveSettingsCDO()->mUniqueItemCount = 3;
	}

	out_validSchematics = schematicsToOffer;
}

TSubclassOf<class UFGSchematic> AApHardDriveGachaSubsystem::GetRandomSchematic(int lowerBound, int upperBound) {
	int size = upperBound - lowerBound;
	int numSchematicsLeft = apHardDriveSchematics.Num();

	if (numSchematicsLeft - schematicsToOffer.Num() <= 0)
		return nullptr;

	if (lowerBound > numSchematicsLeft - size)
		lowerBound = numSchematicsLeft - size;
	if (lowerBound	< 0)
		lowerBound = 0;
	if (upperBound > numSchematicsLeft)
		upperBound = numSchematicsLeft;

	TSubclassOf<class UFGSchematic> schematic;
	do {
		int index = FMath::RandRange(lowerBound, upperBound - 1);
		schematic = apHardDriveSchematics[index];
	} while (schematicsToOffer.Contains(schematic));

	return schematic;
}

TArray<TSubclassOf<class UFGSchematic>> AApHardDriveGachaSubsystem::GetFinalSchematicRewards(
		TCallScope<TArray<TSubclassOf<class UFGSchematic>>(*)(const UFGHardDriveSettings*, const TArray<TSubclassOf<class UFGSchematic>>& allValidSchematicDrops)>& Scope,
	const UFGHardDriveSettings* self, const TArray<TSubclassOf<class UFGSchematic>>& allValidSchematicDrops) {
	UE_LOG(LogApHardDriveGachaSubsystem, Display, TEXT("AApHardDriveGachaSubsystem::GetFinalSchematicRewards()"));

	TArray<TSubclassOf<class UFGSchematic>> resultSchematics = Scope(self, schematicsToOffer);

	return resultSchematics;
}

void AApHardDriveGachaSubsystem::OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(LogApHardDriveGachaSubsystem, Display, TEXT("AApHardDriveGachaSubsystem::OnSchematicCompleted()"));

	if (UFGSchematic::GetType(schematic) != ESchematicType::EST_Alternate)
		return;

	apHardDriveSchematics.Remove(schematic);
}


#pragma optimize("", on)

