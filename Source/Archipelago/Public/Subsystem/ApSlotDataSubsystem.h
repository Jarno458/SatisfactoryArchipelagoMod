#pragma once

#include "CoreMinimal.h"
#include "FGSaveInterface.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Data/ApSlotData.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApSlotDataSubsystem, Log, All);

#include "ApSlotDataSubsystem.generated.h"

UCLASS()
class ARCHIPELAGO_API AApSlotDataSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	AApSlotDataSubsystem();

	// Begin IFGSaveInterface
	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
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
	FORCEINLINE FApSlotData GetSlotData() const { return slotData; };

private:
	UPROPERTY(SaveGame)
	FApSlotData slotData;

	UPROPERTY(SaveGame)
	TArray<FApSaveableHubLayout> saveSlotDataHubLayout;

	void SetSlotDataJson(FString slotDataJson);

	friend class AApServerRandomizerSubsystem;
};
