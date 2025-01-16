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

#define ID_OFFSET 1338000

//TODO REMOVE
#pragma optimize("", off)

USTRUCT()
struct ARCHIPELAGO_API FApReplicatedItemInfo
{
	GENERATED_BODY()

public:
	// Value constructor
	FApReplicatedItemInfo(FString itemName, FString playerName, int64 itemId, int64 locationId, int flags, bool isLocalPlayer) 
		//current format <islocal:1>,<flags:7>,<locationid:12>,<itemid:12>
		//could be squahed to free 8 bits -> <free:8>,<islocal:1>,<flags:3>,<locationid:10>,<itemid:10>
		: data(((isLocalPlayer ? 1 : 0) << 31) | ((flags) << 24) | ((locationId - ID_OFFSET) << 12)),
			itemName(itemName.Left(50)), playerName()
	{
		//no need to replicated the local player's name
		//no need to replicate itemid if its not from local player
		//truncation at 16 is default for AP
		//todo might want to move player replication to its own subsystem so it can be shared between gifting and schematic patching
		if (!isLocalPlayer)
			this->playerName = playerName.Left(16);
		else
			this->data |= ((itemId - ID_OFFSET) << 0);
	}
	// Default constructor
	FApReplicatedItemInfo()
		: data(0), itemName(), playerName()
	{}
	// Copy constructor
	FApReplicatedItemInfo(const FApReplicatedItemInfo& Other) 
		: data(Other.data), itemName(Other.itemName), playerName(Other.playerName)
	{}

private:
	UPROPERTY()
	uint32 data;

public:
	UPROPERTY()
	FString itemName;

	UPROPERTY()
	FString playerName;

public:
	FORCEINLINE int64 const GetItemId() const { return ID_OFFSET + (data & 0x00000FFF); }
	FORCEINLINE int64 const GetLocationId() const { return ID_OFFSET + ((data & 0x00FFF000) >> 12); }
	FORCEINLINE int const GetFlags() const { return (data & 0x4F000000) >> 24; }
	FORCEINLINE bool GetIsLocalPlayer() const { return (data & 0x80000000) > 0; }
};

#pragma optimize("", on)

USTRUCT()
struct ARCHIPELAGO_API FApReplicatedMilestoneInfo
{
	GENERATED_BODY()

public:
	UPROPERTY()
	uint8 tier;

	UPROPERTY()
	uint8 milestone;

	UPROPERTY()
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

	bool IsCollected(int64 locationId) const { return collectedLocations.Contains(locationId); };

	void Server_SetItemInfoPerSchematicId(int currentPlayerId, const TArray<FApNetworkItem>& itemInfo);
	void Server_SetItemInfoPerMilestone(int currentPlayerId, const TMap<int, TMap<int, TArray<FApNetworkItem>>>& itemsPerMilestone);
	void Server_Collect(TSet<int64> locations);
private:
	UPROPERTY(ReplicatedUsing = OnRep_ItemInfosReplicated)
	TArray<FApReplicatedItemInfo> replicatedItemInfos;

	UPROPERTY(ReplicatedUsing = OnRep_MilestonesReplicated)
	TArray<FApReplicatedMilestoneInfo> replicatedMilestones;

	UPROPERTY(ReplicatedUsing = OnRep_CollectedLocationsReplicated)
	TArray<int16> replicatedCollectedLocations;

	UContentLibSubsystem* contentLibSubsystem;
	AApConnectionInfoSubsystem* connectionInfo;
	AApSlotDataSubsystem* slotDataSubsystem;
	AApMappingsSubsystem* mappingSubsystem;
	AFGResearchManager* RManager;

	TSet<int64> collectedLocations;
	TQueue<int64> clientCollectedLocationsToProcess;

	TMap<int64, TSubclassOf<UFGSchematic>> client_collectableSchematicPerLocation;
	TSet<int64> client_locationsWithLocalItems;

	UTexture2D* collectedIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Archipelago/Assets/SourceArt/ArchipelagoAssetPack/AP-Black.AP-Black"));
	UClass* workshopComponent = nullptr;

	bool receivedItemInfos = false;
	bool receivedMilestones = false;
	bool isInitialized = false;
	bool hasPatchedSchematics = false;

	TArray<FApReplicatedItemInfo> MakeReplicateable(int currentPlayerId, const TArray<FApNetworkItem>& itemInfo);

	void Initialize();
	void InitializeSchematicsBasedOnScoutedData();

	void InitializaHubSchematic(TSubclassOf<UFGSchematic> factorySchematic, const TArray<FApReplicatedItemInfo>& items, const TMap<int64, int>& costs);
	void InitializaSchematicForItem(TSubclassOf<UFGSchematic> factorySchematic, const FApReplicatedItemInfo& item, bool updateSchemaName);

	FContentLib_UnlockInfoOnly CreateUnlockInfoOnly(const FApReplicatedItemInfo& item);
	void UpdateInfoOnlyUnlockWithBuildingInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApBuildingItem> itemInfo);
	void UpdateInfoOnlyUnlockWithRecipeInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApRecipeItem> itemInfo);
	void UpdateInfoOnlyUnlockWithItemBundleInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApItem> itemInfo);
	void UpdateInfoOnlyUnlockWithSchematicInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApSchematicItem> itemInfo);
	void UpdateInfoOnlyUnlockWithSpecialInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item, TSharedRef<FApSpecialItem> itemInfo);
	void UpdateInfoOnlyUnlockWithGenericApInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, const FApReplicatedItemInfo& item);
	
	void Client_ProcessCollectedLocations();
	void Client_Collect(UFGUnlock* unlock, bool isLocalItem);

	UFUNCTION() //required for event hookup
	void OnRep_ItemInfosReplicated();
	UFUNCTION() //required for event hookup
	void OnRep_MilestonesReplicated();
	UFUNCTION() //required for event hookup
	void OnRep_CollectedLocationsReplicated();
	UFUNCTION() //required for event hookup
	void OnClientSubsystemsValid();
};

