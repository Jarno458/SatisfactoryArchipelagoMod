#pragma once

#include "CoreMinimal.h"

#include "FGSchematicManager.h"
#include "FGResearchManager.h"

#include "Registry/ModContentRegistry.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Templates/SubclassOf.h"
#include "Reflection/ClassGenerator.h"
#include "Buildables/FGBuildable.h"
#include "Buildables/FGBuildableAutomatedWorkBench.h"
#include "Unlocks/FGUnlockInfoOnly.h"
#include "ApConfigurationStruct.h"
#include "Data/ApSlotData.h"
#include "Data/ApTypes.h"
#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"
#include "Subsystem/ApSlotDataSubsystem.h"
#include "Subsystem/ApMappingsSubsystem.h"
#include "ApUtils.h"

#include "CLSchematicBPFLib.h"
#include "BPFContentLib.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApSchematicPatcherSubsystem, Log, All);

#include "ApSchematicPatcherSubsystem.generated.h"

#define IS_LOCAL_PLAYER 0b10000000
#define ID_OFFSET 1338000

USTRUCT()
struct ARCHIPELAGO_API FApReplicatedItemInfo
{
	GENERATED_BODY()

public:
	UPROPERTY()
	uint8 flags;

	UPROPERTY()
	int16 item;

	UPROPERTY()
	int16 location;

	UPROPERTY()
	FString itemName;

	UPROPERTY()
	FString playerName;
};

USTRUCT()
struct ARCHIPELAGO_API FApReplicatedMilestoneInfo
{
	GENERATED_BODY()

public:
	UPROPERTY()
	uint8 tier;

	UPROPERTY()
	uint8 milestone;

	UPROPERTY(SaveGame)
	TArray<FApReplicatedItemInfo> items;
};

UCLASS()
class ARCHIPELAGO_API AApSchematicPatcherSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AApSchematicPatcherSubsystem ();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	static AApSchematicPatcherSubsystem* Get(class UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Ap Schematic Patcher Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApSchematicPatcherSubsystem* Get(UObject* worldContext);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool IsInitialized() const { return isInitialized; };

	//quite slow now its not a set
	FORCEINLINE bool IsCollected(int64 locationId) const { return collectedLocations.Contains(locationId); };

	bool IsCollected(UFGUnlock* unlock); //TODO remove
	void Collect(UFGSchematic* schematic, int unlockIndex, FApNetworkItem& networkItem);

	void Server_SetItemInfoPerSchematicId(int currentPlayerId, const TArray<FApNetworkItem>& itemInfo);
	void Server_SetItemInfoPerMilestone(int currentPlayerId, const TMap<int, TMap<int, TArray<FApNetworkItem>>>& itemsPerMilestone);

private:
	UPROPERTY(ReplicatedUsing = OnRep_ItemInfosReplicated)
	TArray<FApReplicatedItemInfo> replicatedItemInfos;

	UPROPERTY(ReplicatedUsing = OnRep_MilestonesReplicated)
	TArray<FApReplicatedMilestoneInfo> replicatedMilestones;

	UPROPERTY(ReplicatedUsing = OnRep_CollectedLocationsReplicated)
	TArray<int> collectedLocations;

	UContentLibSubsystem* contentLibSubsystem;
	AApConnectionInfoSubsystem* connectionInfo;
	AApSlotDataSubsystem* slotDataSubsystem;
	AApMappingsSubsystem* mappingSubsystem;

	TQueue<int64> collectedLocationsToProcess;

	UTexture2D* collectedIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Archipelago/Assets/SourceArt/ArchipelagoAssetPack/AP-Black.AP-Black"));
	UClass* workshopComponent = LoadClass<UObject>(nullptr, TEXT("/Game/FactoryGame/Buildable/-Shared/WorkBench/BP_WorkshopComponent.BP_WorkshopComponent_C"));

	bool receivedItemInfos = false;
	bool receivedMilestones = false;
	bool isInitialized = false;
	bool hasPatchedSchematics = false;

	void Collect(UFGUnlock* unlock, FApNetworkItem& networkItem);

	TArray<FApReplicatedItemInfo> MakeReplicateable(int currentPlayerId, const TArray<FApNetworkItem>& itemInfo);

	void Initialize();
	void InitializeSchematicsBasedOnScoutedData();

	//void InitializaHubSchematic(FString name, TSubclassOf<UFGSchematic> factorySchematic, const TArray<FApReplicatedItemInfo>& apItems);
	void InitializaHubSchematic(TSubclassOf<UFGSchematic> factorySchematic, const TArray<FApReplicatedItemInfo>& items, const TMap<FString, int>& costs);
	void InitializaSchematicForItem(TSubclassOf<UFGSchematic> factorySchematic, const FApReplicatedItemInfo& item, bool updateSchemaName);

	FContentLib_UnlockInfoOnly CreateUnlockInfoOnly(const FApReplicatedItemInfo& item);
	void UpdateInfoOnlyUnlockWithBuildingInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApBuildingItem> itemInfo);
	void UpdateInfoOnlyUnlockWithRecipeInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApRecipeItem> itemInfo);
	void UpdateInfoOnlyUnlockWithItemBundleInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApItem> itemInfo);
	void UpdateInfoOnlyUnlockWithSchematicInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApSchematicItem> itemInfo);
	void UpdateInfoOnlyUnlockWithSpecialInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApSpecialItem> itemInfo);
	void UpdateInfoOnlyUnlockWithGenericApInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item);
	
	UFUNCTION() //required for event hookup
	void OnRep_ItemInfosReplicated();
	UFUNCTION() //required for event hookup
	void OnRep_MilestonesReplicated();
	UFUNCTION() //required for event hookup
	void OnRep_CollectedLocationsReplicated();
	UFUNCTION() //required for event hookup
	void OnClientSubsystemsValid();
};

