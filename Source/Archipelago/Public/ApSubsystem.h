#pragma once

#include <functional>
#include <map>

#include "CoreMinimal.h"

#include "FGSchematicManager.h"
#include "FGResearchManager.h"

#include "Patching/BlueprintHookHelper.h"
#include "Patching/BlueprintHookManager.h"
#include "Registry/ModContentRegistry.h"
#include "Util/BlueprintLoggingLibrary.h"
#include "Subsystem/ModSubsystem.h"

#include "Archipelago.h"

DECLARE_LOG_CATEGORY_EXTERN(ApSubsystem, Log, All);

#include "ApSubsystem.generated.h"

UCLASS(Blueprintable)
class ARCHIPELAGO_API AApSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AApSubsystem();

	UPROPERTY()
		AFGSchematicManager* SManager;
	UPROPERTY()
		AFGResearchManager* RManager;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void MonitorDataStoreValue(std::string key, AP_DataType dataType, std::function<void(AP_SetReply)> callback);

private:
	static std::map<std::string, std::function<void(AP_SetReply)>> callbacks;

	static void SetReplyCallback(AP_SetReply setReply);
};
