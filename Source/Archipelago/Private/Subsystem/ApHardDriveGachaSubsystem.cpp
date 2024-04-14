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

void AApHardDriveGachaSubsystem::Initialize(TArray<TSubclassOf<UFGSchematic>> apSchematics) {
	UE_LOG(LogApHardDriveGachaSubsystem, Display, TEXT("AApSubsystem()::Initialize()"));

	apHardDriveSchematics = apSchematics;
}

void AApHardDriveGachaSubsystem::BeginPlay() {
	UE_LOG(LogApHardDriveGachaSubsystem, Display, TEXT("AApHardDriveGachaSubsystem::BeginPlay()"));
	Super::BeginPlay();

	UWorld* world = GetWorld();

	AApSlotDataSubsystem* slotDataSubsystem = AApSlotDataSubsystem::Get(world);
	fgcheck(slotDataSubsystem);

	if (slotDataSubsystem->GetSlotData().hasLoadedSlotData && !slotDataSubsystem->GetSlotData().enableHardDriveGacha) {
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
			&& !apHardDriveSchematics.Contains(schematic))
			modSchematics.Add(schematic);
	}

	schematicsToOffer.Empty();

	//TODO devide in buckets
	schematicsToOffer.Add(apHardDriveSchematics[0]);
	schematicsToOffer.Add(apHardDriveSchematics[1]);
	schematicsToOffer.Add(apHardDriveSchematics[2]);

	if (modSchematics.Num() > 0)	{
		UFGGlobalSettings::GetHardDriveSettingsCDO()->mUniqueItemCount = 4;

		int index = FMath::RandRange(0, modSchematics.Num() - 1);
		schematicsToOffer.Add(modSchematics[index]);
	} else {
		UFGGlobalSettings::GetHardDriveSettingsCDO()->mUniqueItemCount = 3;
	}

	out_validSchematics = schematicsToOffer;
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

