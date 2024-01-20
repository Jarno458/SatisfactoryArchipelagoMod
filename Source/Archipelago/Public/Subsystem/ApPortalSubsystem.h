#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"

#include "Buildable/ApPortal.h"
#include "Subsystem/ApMappingsSubsystem.h"

#include "Data/ApTypes.h"

#include "ApPortalSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApPortalSubsystem, Log, All);

/**
 * 
 */
UCLASS()
class ARCHIPELAGO_API AApPortalSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	AApPortalSubsystem();

	virtual void BeginPlay() override;

	virtual void Tick(float dt) override;

	static AApPortalSubsystem* Get(class UWorld* world);

protected:
	// Begin IFGSaveInterface
	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void GatherDependencies_Implementation(TArray<UObject*>& out_dependentObjects) override {};
	virtual bool NeedTransform_Implementation() override { return false; };
	virtual bool ShouldSave_Implementation() const override { return true; };
	// End IFSaveInterface

private:
	AApMappingsSubsystem* mappings;
	AModSubsystem* giftingSubsystem;
	
	UPROPERTY(SaveGame)
	TArray<int64> OutputQueueSave;

	TQueue<FInventoryItem, EQueueMode::Mpsc> OutputQueue;
	TQueue<FInventoryItem, EQueueMode::Mpsc> StartupQueue;

	int lastUsedPortalIndex;
	bool isInitialized;

public:
	TSet<const AApPortal*> BuiltPortals;

	FORCEINLINE bool IsInitialized() const { return isInitialized; };

	void Enqueue(TSubclassOf<UFGItemDescriptor> cls, int amount);

	void Send(FApPlayer targetPlayer, FInventoryStack itemStack);

	void RegisterPortal(const AApPortal* portal);
	void UnRegisterPortal(const AApPortal* portal);

private:
	void ProcessInputQueue();
	void ProcessOutputQueue();

	void StoreQueueForSave();
	void RebuildQueueFromSave();
};
