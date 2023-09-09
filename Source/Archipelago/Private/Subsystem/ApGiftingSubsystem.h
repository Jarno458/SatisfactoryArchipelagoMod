#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApPortalSubsystem.h"

#include "ApGiftingSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApGiftingSubsystem, Log, All);

/**
 * 
 */
UCLASS()
class AApGiftingSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApGiftingSubsystem();

	virtual void BeginPlay() override;

	virtual void Tick(float dt) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get Archipelago Gifting Subsystem"))
	static AApGiftingSubsystem* Get();
	static AApGiftingSubsystem* Get(class UWorld* world);

private:
	bool apInitialized;

	AApSubsystem* ap;
	AApPortalSubsystem* portalSubSystem;

public:

private:
	void OpenGiftbox(const FApSlotData slotData);
	void OnGiftsUpdated(AP_SetReply setReply);

	TSubclassOf<UFGItemDescriptor> TryGetItemClassByName(FString name);
	TSubclassOf<UFGItemDescriptor> TryGetItemByTraits(TArray<TSharedPtr<FJsonValue>> traits);
};
