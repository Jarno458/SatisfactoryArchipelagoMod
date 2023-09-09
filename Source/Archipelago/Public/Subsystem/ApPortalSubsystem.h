#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"

#include "Buildable/ApPortal.h"

#include "ApPortalSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApPortalSubsystem, Log, All);

/**
 * 
 */
UCLASS()
class ARCHIPELAGO_API AApPortalSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApPortalSubsystem();

	virtual void BeginPlay() override;

	virtual void Tick(float dt) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get Portal ApSubsystem"))
	static AApPortalSubsystem* Get();
	static AApPortalSubsystem* Get(class UWorld* world);

private:
	TQueue<FInventoryItem> OutputQueue = TQueue<FInventoryItem>();

	int lastUsedPortalIndex;

public:
	UPROPERTY(BlueprintReadOnly) //blueprint likely dont need this
	TSet<const AApPortal*> BuiltPortals;

	void Enqueue(TSubclassOf<UFGItemDescriptor> cls);

	void RegisterPortal(const AApPortal* portal);
	void UnRegisterPortal(const AApPortal* portal);
};
