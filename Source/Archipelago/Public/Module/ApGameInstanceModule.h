#pragma once

#include "CoreMinimal.h"

#include "FGBlueprintFunctionLibrary.h"

#include "ApBlueprintDataBridge.h"
#include "Module/GameInstanceModule.h"
#include "Patching/NativeHookManager.h"

#if UE_SERVER
#include "Controller/FGServerStateController.h"
#include "Controller/FGServerManagementController.h"
#else
#include "FGServerObjectOptionInterface.h"
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogApGameInstanceModule, Log, All);

#include "ApGameInstanceModule.generated.h"

/**
 * Blueprint implemented Mod Module
*/
UCLASS(Abstract, Blueprintable)
class ARCHIPELAGO_API UApGameInstanceModule : public UGameInstanceModule
{
	GENERATED_BODY()

public:
	UApGameInstanceModule();

	// Data Asset populated in Blueprint that contains references to assets so Cpp can refer to them
	UPROPERTY(EditDefaultsOnly, Category = "Archipelago")
	UApBlueprintDataBridge* BlueprintData;

	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;

private:
#if UE_SERVER
	void Server_GetOptions(TCallScope<void(*)(const UFGServerStateController*, TMap<FString, FString>&, TMap<FString, FString>&)>& Scope, const UFGServerStateController* self, TMap<FString, FString>& OutServerOptions, TMap<FString, FString>& OutPendingServerOptions);
	void Server_ApplyOptions(TCallScope<void(*)(const UFGServerManagementController*, const TMap<FString, FString>&)>& Scope, const UFGServerManagementController* self, const TMap<FString, FString>& UpdatedServerOptions);
#else
	void Client_ReceiveServerSettings(TCallScope<void(*)(UFGServerObjectOptionAdapter*, const TMap<FString, FString>&, const TMap<FString, FString>&)>& Scope, UFGServerObjectOptionAdapter* self, const TMap<FString, FString>& InServerSettings, const TMap<FString, FString>& PendingServerOptions);
	void Client_WriteChangedSettings(TCallScope<void(*)(UFGServerObjectOptionAdapter*, TMap<FString, FString>&)>& Scope, UFGServerObjectOptionAdapter* self, TMap<FString, FString>& OutServerSettings);
#endif
};
