#pragma once

#include "CoreMinimal.h"
#include "FGSaveInterface.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApSlotDataSubsystem, Log, All);

#include "ApSlotDataSubsystem.generated.h"

#define ID_OFFSET 1338000

USTRUCT()
struct ARCHIPELAGO_API FApReplicatedHubLayoutEntry
{
	GENERATED_BODY()

private:
	UPROPERTY(SaveGame)
	int32 data;

public:
	// replication compression
	void Pack(int tier, int milestone, int64 itemId, int amount) {
		//current format <tier:4>,<milestone:4>,<itemId:12>,<amount:12>
		data = (tier << 28) | (milestone << 24) | ((itemId - ID_OFFSET) << 12) | (amount << 0);
	}
	FORCEINLINE int const GetTier() const { return (data & 0xF0000000) >> 28; }
	FORCEINLINE int const GetMilestone() const { return (data & 0x0F000000) >> 24; }
	FORCEINLINE int64 const GetItemId() const { return ID_OFFSET + ((data & 0x00FFF000) >> 12); }
	FORCEINLINE int const GetAmount() const { return (data & 0x00000FFF) >> 0; }
	//
};

UCLASS()
class ARCHIPELAGO_API AApSlotDataSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	AApSlotDataSubsystem();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Begin IFGSaveInterface
	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void GatherDependencies_Implementation(TArray<UObject*>& out_dependentObjects) override {};
	virtual bool NeedTransform_Implementation() override { return false; };
	virtual bool ShouldSave_Implementation() const override { return true; };
	// End IFSaveInterface

public:
	// Get subsystem. Server-side only, null on clients
	static AApSlotDataSubsystem* Get(class UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Archipelago Slot Data Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApSlotDataSubsystem* Get(UObject* worldContext);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool HasLoadedSlotData() const { return hasLoadedSlotData; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	const TMap<int64, int> GetCostsForMilestone(int tier, int milestone);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetNumberOfHubTiers();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetNumberOfMilestonesForTier(int tier);

	void SetSlotDataJson(FString slotDataJson);

public:
	UPROPERTY(BlueprintReadOnly, SaveGame)
	int NumberOfChecksPerMilestone;
	UPROPERTY(BlueprintReadOnly, SaveGame)
	int FreeSampleEquipment;
	UPROPERTY(BlueprintReadOnly, SaveGame)
	int FreeSampleBuildings;
	UPROPERTY(BlueprintReadOnly, SaveGame)
	int FreeSampleParts;
	UPROPERTY(BlueprintReadOnly, SaveGame)
	bool FreeSampleRadioactive;
	UPROPERTY(BlueprintReadOnly, SaveGame)
	bool EnergyLink;
	UPROPERTY(BlueprintReadOnly, SaveGame)
	bool EnableHardDriveGacha;
	UPROPERTY(BlueprintReadOnly, SaveGame, Replicated)
	uint8 FinalSpaceElevatorTier;
	UPROPERTY(BlueprintReadOnly, SaveGame, Replicated)
	int64 FinalResourceSinkPoints;

private:
	UPROPERTY(SaveGame, ReplicatedUsing = ReconstructHubLayout)
	TArray<FApReplicatedHubLayoutEntry> hubCostEntries;
	TArray<TArray<TMap<int64, int>>> hubLayout; //build based on hubCostEntries

	bool hasLoadedSlotData;

private:
	UFUNCTION() //required for event hookup
	void ReconstructHubLayout();
};
