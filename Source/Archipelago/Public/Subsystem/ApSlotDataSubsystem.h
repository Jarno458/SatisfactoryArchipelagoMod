#pragma once

#include "CoreMinimal.h"
#include "FGSaveInterface.h"
#include "Subsystem/ModSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApSlotDataSubsystem, Log, All);

#include "ApSlotDataSubsystem.generated.h"

#define ID_OFFSET 1338000

UENUM(BlueprintType)
enum class EApSlotDataState : uint8 {
	NotLoaded UMETA(DisplayName = "Not Avaiable"),
	Ready UMETA(DisplayName = "Avaiable"),
	Failed UMETA(DisplayName = "Loading Failed"),
};

USTRUCT()
struct ARCHIPELAGO_API FApReplicatedHubLayoutEntry
{
	GENERATED_BODY()
public:
	// Value constructor
	FApReplicatedHubLayoutEntry(int tier, int milestone, int64 itemId, uint32 amount)
		//current format <tier:4>,<milestone:4>,<itemId:24>,<amount:20>
		: data(
			  (((uint64)tier) << 48) 
			| (((uint64)milestone) << 44) 
			| (((uint64)itemId) << 20) 
			| (((uint64)amount) << 0)
		)
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
	int64 data; //Sadly a single hub milestone can take up 50_000 costs so it wont fit in 12bit 4096 max

public:
	FORCEINLINE int const GetTier() const { return (data & 0x000F000000000000) >> 48; }
	FORCEINLINE int const GetMilestone() const { return (data & 0x0000F00000000000) >> 44; }
	FORCEINLINE int64 const GetItemId() const { return ((data & 0x00000FFFFFF00000) >> 20); }
	FORCEINLINE int const GetAmount() const { return (data & 0x00000000000FFFFF) >> 0; }
};

USTRUCT()
struct ARCHIPELAGO_API FApReplicatedCostAmount
{
	GENERATED_BODY()

public:
	// Value constructor
	FApReplicatedCostAmount(int64 itemId, uint32 amount)
		//current format <itemId:12>,<amount:20>
		: data(
			  ((itemId - ID_OFFSET) << 20) 
			| (amount << 0)
		)
	{}
	// Default constructor
	FApReplicatedCostAmount()
		: data(0)
	{}
	// Copy constructor
	FApReplicatedCostAmount(const FApReplicatedCostAmount& Other)
		: data(Other.data)
	{}

private:
	UPROPERTY(SaveGame)
	int32 data;

public:
	FORCEINLINE int64 const GetItemId() const { return ID_OFFSET + ((data & 0xFFF00000) >> 20); }
	FORCEINLINE uint32 const GetAmount() const { return (data & 0x000FFFFF) >> 0; }
};

USTRUCT()
struct ARCHIPELAGO_API FApGoals
{
	GENERATED_BODY()

public:
	// Value constructor
	FApGoals(bool anyGoal, 
		bool resourceSinkGoal, bool spaceElevatorGoal, bool resourceSinkPerMinuteGoal, bool explorationGoal, bool ficsmasGoal,
		uint8 finalSpaceElevatorTier, uint64 finalResourceSinkPoints, uint64 perMinuteResourceSinkPoints)
		//current format <anyGoal:1>,<final_space_elevator_tier:3><undefined:23>,<ficsmas_goal:1>,<exploration_goal:1>,<resource_sink_per_minute_goal:1>,<resource_sink_goal:1>,<space_elevator_tier:1>
		: data(
				  ((anyGoal ? 1 : 0) << 31) 
				| ((finalSpaceElevatorTier & 0x7) << 28) 
				| ((ficsmasGoal ? 1 : 0) << 4)
				| ((explorationGoal ? 1 : 0) << 3) 
				| ((resourceSinkPerMinuteGoal ? 1 : 0) << 2) 
				| ((resourceSinkGoal ? 1 : 0) << 1) 
				| ((spaceElevatorGoal ? 1 : 0) << 0)
			),
			finalResourceSinkPoints(finalResourceSinkPoints),
			perMinuteResourceSinkPoints(perMinuteResourceSinkPoints)
	{
	}
	// Default constructor
	FApGoals()
		: data(0), finalResourceSinkPoints(0), perMinuteResourceSinkPoints(0)
	{}
	// Copy constructor
	FApGoals(const FApGoals& Other)
		: data(Other.data), 
			finalResourceSinkPoints(Other.finalResourceSinkPoints), 
			perMinuteResourceSinkPoints(Other.perMinuteResourceSinkPoints)
	{}

private:
	UPROPERTY(SaveGame)
	uint32 data;

	UPROPERTY(SaveGame)
	uint64 finalResourceSinkPoints;

	UPROPERTY(SaveGame)
	uint32 perMinuteResourceSinkPoints;

public:
	FORCEINLINE bool const RequireAllGoals() const { return (data & 0x80000000) == 0; }
	FORCEINLINE uint8 const GetFinalSpaceElevatorTier() const { return (data & 0x70000000) >> 28; }
	FORCEINLINE uint64 const GetFinalResourceSinkPoints() const { return finalResourceSinkPoints; }
	FORCEINLINE uint32 const GePerMinuteResourceSinkPoints() const { return perMinuteResourceSinkPoints; }
	FORCEINLINE bool const IsSpaceElevatorGoalEnabled() const { return (data & 0x00000001) > 0; }
	FORCEINLINE bool const IsResourceSinkGoalEnabled() const { return (data & 0x00000002) > 0; }
	FORCEINLINE bool const IsResourceSinkPerMinuteGoalEnabled() const { return (data & 0x00000004) > 0; }
	FORCEINLINE bool const IsExplorationGoalEnabled() const { return (data & 0x00000008) > 0; }
	FORCEINLINE bool const IsFicsmasGoalEnabled() const { return (data & 0x00000010) > 0; }
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

	UFUNCTION(BlueprintCallable)
	bool HasLoadedSlotData();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE EApSlotDataState GetSlotDataState() const { return slotDataState; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE FText GetLastError() const { return lastError; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	const TMap<int64, int> GetCostsForMilestone(int tier, int milestone);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	const TMap<int64, int> GetExplorationGoalCosts();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetNumberOfHubTiers();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetNumberOfMilestonesForTier(int tier);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<int64> GetStarterRecipeIds();

public:
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
	bool DeathLink;

	UPROPERTY(BlueprintReadOnly, SaveGame, Replicated)
	TArray<int64> starterRecipeIds;

private:
	UPROPERTY(SaveGame, ReplicatedUsing = ReconstructHubLayout)
	TArray<FApReplicatedHubLayoutEntry> hubCostEntriesReplicated;
	TArray<TArray<TMap<int64, int>>> hubLayout; //build based on hubCostEntries

	UPROPERTY(SaveGame, ReplicatedUsing = ReconstructExplorationCost)
	TArray<FApReplicatedCostAmount> replicatedExplorationCost;
	TMap<int64, int> explorationCosts; //build based on replicatedExplorationCost

	bool hasLoadedSlotData;
	bool hasLoadedExplorationData;
	EApSlotDataState slotDataState = EApSlotDataState::NotLoaded;
	FText lastError;

	UPROPERTY(SaveGame, Replicated)
	FApGoals Goals;

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool const RequireAllGoals() const { return Goals.RequireAllGoals(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE uint8 const GetFinalSpaceElevatorTier() const { return Goals.GetFinalSpaceElevatorTier(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE int64 const GetFinalResourceSinkPoints() const { return Goals.GetFinalResourceSinkPoints(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE int32 const GePerMinuteResourceSinkPoints() const { return Goals.GePerMinuteResourceSinkPoints(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool const IsSpaceElevatorGoalEnabled() const { return Goals.IsSpaceElevatorGoalEnabled(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool const IsResourceSinkGoalEnabled() const { return Goals.IsResourceSinkGoalEnabled(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool const IsResourceSinkPerMinuteGoalEnabled() const { return Goals.IsResourceSinkPerMinuteGoalEnabled(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool const IsExplorationGoalEnabled() const { return Goals.IsExplorationGoalEnabled(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool const IsFicsmasGoalEnabled() const { return Goals.IsFicsmasGoalEnabled(); }

private:
	EApSlotDataState TryLoadSlotDataFromServer(FString json);

	UFUNCTION() //required for event hookup
	void ReconstructHubLayout();

	UFUNCTION() //required for event hookup
	void ReconstructExplorationCost();
};
