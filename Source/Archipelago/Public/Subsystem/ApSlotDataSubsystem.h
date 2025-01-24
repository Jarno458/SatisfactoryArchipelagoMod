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

public:
	// Value constructor
	FApReplicatedHubLayoutEntry(int tier, int milestone, int64 itemId, int amount)
		//current format <tier:4>,<milestone:4>,<itemId:12>,<amount:12>
		: data((tier << 28) | (milestone << 24) | ((itemId - ID_OFFSET) << 12) | (amount << 0))
	{}	
	// Default constructor
	FApReplicatedHubLayoutEntry()
		: data(0)
	{}
	// Copy constructor
	FApReplicatedHubLayoutEntry(const FApReplicatedHubLayoutEntry& Other)
		: data(Other.data)
	{}

private:
	UPROPERTY(SaveGame)
	int32 data;

public:
	FORCEINLINE int const GetTier() const { return (data & 0xF0000000) >> 28; }
	FORCEINLINE int const GetMilestone() const { return (data & 0x0F000000) >> 24; }
	FORCEINLINE int64 const GetItemId() const { return ID_OFFSET + ((data & 0x00FFF000) >> 12); }
	FORCEINLINE int const GetAmount() const { return (data & 0x00000FFF) >> 0; }
};

USTRUCT()
struct ARCHIPELAGO_API FApGoals
{
	GENERATED_BODY()

public:
	// Value constructor
	FApGoals(bool anyGoal, bool resourceSinkGoal, bool spaceElevatorGoal, uint8 finalSpaceElevatorTier, uint64 finalResourceSinkPoints)
		//current format <anyGoal:1>,<final_space_elevator_tier:3><undefined:26>,<resource_sink_goal:1>,<space_elevator_tier:1>
		: data(((anyGoal ? 1 : 0) << 31) | ((finalSpaceElevatorTier) << 28) | ((resourceSinkGoal ? 1 : 0) << 1) | ((spaceElevatorGoal ? 1 : 0) << 0)),
			finalResourceSinkPoints(finalResourceSinkPoints)
	{
	}
	// Default constructor
	FApGoals()
		: data(0), finalResourceSinkPoints(0)
	{}
	// Copy constructor
	FApGoals(const FApGoals& Other)
		: data(Other.data), finalResourceSinkPoints(Other.finalResourceSinkPoints)
	{}

private:
	UPROPERTY(SaveGame)
	uint32 data;

	UPROPERTY(SaveGame)
	uint64 finalResourceSinkPoints;

public:
	FORCEINLINE int64 const RequireAllGoals() const { return (data & 0x80000000) == 0; }
	FORCEINLINE uint8 const GetFinalSpaceElevatorTier() const { return (data & 0x70000000) >> 28; }
	FORCEINLINE int const GetFinalResourceSinkPoints() const { return finalResourceSinkPoints; }
	FORCEINLINE bool const IsSpaceElevatorGoalEnabled() const { return (data & 0x00000001) > 0; }
	FORCEINLINE bool const IsResourceSinkGoalEnabled() const { return (data & 0x00000002) > 0; }
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

private:
	UPROPERTY(SaveGame, ReplicatedUsing = ReconstructHubLayout)
	TArray<FApReplicatedHubLayoutEntry> hubCostEntries;
	TArray<TArray<TMap<int64, int>>> hubLayout; //build based on hubCostEntries

	bool hasLoadedSlotData;

	UPROPERTY(SaveGame, Replicated)
	FApGoals Goals;

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE int64 const RequireAllGoals() const { return Goals.RequireAllGoals(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE uint8 const GetFinalSpaceElevatorTier() const { return Goals.GetFinalSpaceElevatorTier(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE int const GetFinalResourceSinkPoints() const { return Goals.GetFinalResourceSinkPoints(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool const IsSpaceElevatorGoalEnabled() const { return Goals.IsSpaceElevatorGoalEnabled(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool const IsResourceSinkGoalEnabled() const { return Goals.IsResourceSinkGoalEnabled(); }

private:
	UFUNCTION() //required for event hookup
	void ReconstructHubLayout();
};
