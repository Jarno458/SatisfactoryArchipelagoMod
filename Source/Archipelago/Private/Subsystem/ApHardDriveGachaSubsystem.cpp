#include "Subsystem/ApHardDriveGachaSubsystem.h"
#include "Patching/NativeHookManager.h"
#include "Registry/ModContentRegistry.h"

DEFINE_LOG_CATEGORY(LogApHardDriveGachaSubsystem);

//TODO REMOVE
#pragma optimize("", off)

AApHardDriveGachaSubsystem::AApHardDriveGachaSubsystem() : Super() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 1.0f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

AApHardDriveGachaSubsystem* AApHardDriveGachaSubsystem::Get(UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApHardDriveGachaSubsystem>();
}

void AApHardDriveGachaSubsystem::DispatchLifecycleEvent(ELifecyclePhase phase, TArray<TSubclassOf<UFGSchematic>> apHardcodedSchematics) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem()::DispatchLifecycleEvent(%s)"), *UEnum::GetValueAsString(phase));


	apUnlocks.Add(apHardcodedSchematics[apHardcodedSchematics.Num() - 1]);
}

void AApHardDriveGachaSubsystem::BeginPlay() {
	Super::BeginPlay();

	UWorld* world = GetWorld();
	ap = AApSubsystem::Get(world);
	fgcheck(ap);
	contentRegistry = UModContentRegistry::Get(world);
	fgcheck(contentRegistry);

	//void GetValidSchematicRewardDrops( class AFGSchematicManager* schematicManager, TArray<TSubclassOf<class UFGSchematic>>& out_validSchematics) const;
	//TArray<TSubclassOf<class UFGSchematic>> GetFinalSchematicRewards( const TArray<TSubclassOf<class UFGSchematic>>& allValidSchematicDrops ) const;


	if (!hooksInitialized) {
		UE_LOG(LogApHardDriveGachaSubsystem, Display, TEXT("AApHardDriveGachaSubsystem::BeginPlay() Initializing hooks"));
		//UFGHardDriveSettings* bpscdo = GetMutableDefault<UFGHardDriveSettings>();
		SUBSCRIBE_METHOD(UFGHardDriveSettings::GetValidSchematicRewardDrops, [this](auto& scope, const UFGHardDriveSettings* self, AFGSchematicManager* schematicManager, TArray<TSubclassOf<class UFGSchematic>>& out_validSchematics) {
			GetValidSchematicRewardDrops(scope, self, schematicManager, out_validSchematics);
		});

		SUBSCRIBE_METHOD(UFGHardDriveSettings::GetFinalSchematicRewards, [this](auto& scope, const UFGHardDriveSettings* self, const TArray<TSubclassOf<class UFGSchematic>>& allValidSchematicDrops) {
			return GetFinalSchematicRewards(scope, self, allValidSchematicDrops);
		});

		hooksInitialized = true;
	}

	UFGGlobalSettings::GetHardDriveSettingsCDO()->mUniqueItemCount = 4;
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

	schematicsToOffer.Empty();

	TArray<TSubclassOf<class UFGSchematic>> modSchematics;

	for (TSubclassOf<class UFGSchematic> schematic : resultSchematics) {

		FGameObjectRegistration registration;

		if (contentRegistry->GetSchematicRegistrationInfo(schematic, registration) && registration.HasAnyFlags(EGameObjectRegistrationFlags::BuiltIn))
			modSchematics.Add(schematic);
	}

	schematicsToOffer.Add(apUnlocks[0]);
	schematicsToOffer.Add(apUnlocks[0]);
	schematicsToOffer.Add(apUnlocks[0]);

	if (modSchematics.Num() > 0)	{
		int index = FMath::RandRange(0, modSchematics.Num() - 1);
		schematicsToOffer.Add(modSchematics[index]);
	}
}

TArray<TSubclassOf<class UFGSchematic>> AApHardDriveGachaSubsystem::GetFinalSchematicRewards(
		TCallScope<TArray<TSubclassOf<class UFGSchematic>>(*)(const UFGHardDriveSettings*, const TArray<TSubclassOf<class UFGSchematic>>& allValidSchematicDrops)>& Scope,
	const UFGHardDriveSettings* self, const TArray<TSubclassOf<class UFGSchematic>>& allValidSchematicDrops) {
	UE_LOG(LogApHardDriveGachaSubsystem, Display, TEXT("AApHardDriveGachaSubsystem::GetFinalSchematicRewards()"));

	TArray<TSubclassOf<class UFGSchematic>> resultSchematics = Scope(self, schematicsToOffer);

	return resultSchematics;
}


#pragma optimize("", on)

