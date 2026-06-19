#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"

#include "Subsystem/ApMappingsSubsystem.h"
#include "Subsystem/ApConnectionInfoSubsystem.h"

#include "Data/ApTypes.h"

#include "ApPortalSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApPortalSubsystem, Log, All);

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
	class AApServerGiftingSubsystem* giftingSubsystem;
	class AApReplicatedGiftingSubsystem* replicatedGiftingSubsystem;
	class AApVaultSubsystem* vaultSubsystem;

	bool isInitialized;

	FDateTime lastAutoSendTime;

public:
	FORCEINLINE bool IsInitialized() const { return isInitialized; }

	void SendBuffer(FApPlayer targetPlayer, TArray<FItemAmount> items) const;

	bool DoesPlayerAccept(const FApPlayer& targetPlayer, TSubclassOf<UFGItemDescriptor> itemClass) const;

private:
	void ProcessAutoVaultStoring() const;
};
