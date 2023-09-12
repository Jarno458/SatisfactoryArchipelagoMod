#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "JsonObjectConverter.h"

#include "Subsystem/ModSubsystem.h"
#include "Subsystem/ApSubsystem.h"
#include "Subsystem/ApPortalSubsystem.h"

#include "Data/ApGiftJson.h"

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

	TMap<FString, TSubclassOf<UFGItemDescriptor>> NameToItemMapping;
	TMap<TSubclassOf<UFGItemDescriptor>, FString> ItemToNameMapping;

	AApSubsystem* ap;
	AApPortalSubsystem* portalSubSystem;

public:
	void Send(TMap<int, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend);

private:
	void LoadItemNameMapping();

	void OpenGiftbox();
	void OnGiftsUpdated(AP_SetReply setReply);
	void HandleProcessedGifts(TArray<FString> procesedGifts, TArray<FApGiftJson> giftsToReject);

	TSubclassOf<UFGItemDescriptor> TryGetItemClassByTraits(TArray<FApGiftTraitJson> traits);
	TArray<FApGiftTraitJson> GetTraitsForItem(int64_t itemId);
};
