#pragma once

#include "CoreMinimal.h"

#include "FGBlueprintFunctionLibrary.h"

#include "ApBlueprintDataBridge.h"
#include "Module/GameInstanceModule.h"
#include "Patching/NativeHookManager.h"
#include "SessionSettings/SessionSettingsManager.h"

#include "Misc/Variant.h"
#include "Settings/FGUserSettingApplyType.h"

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
	void DediServer_GetOptions(TMap<FString, FString>& OutServerOptions, TMap<FString, FString>& OutPendingServerOptions);
	void DediServer_ApplyOptions(const TMap<FString, FString>& UpdatedServerOptions);

	void DediServer_CopySettingFromSessionSettings(const USessionSettingsManager* sessionSettings, const FString& cvar, TMap<FString, FString>& OutServerOptions, TMap<FString, FString>& OutPendingServerOptions);
	void DediServer_CopySettingToSessionSettings(const USessionSettingsManager* sessionSettings, const FString& cvar, const TMap<FString, FString>& UpdatedServerOptions);

	static void VariantAsString(TCallScope<FString(*)(const FVariant&)>& Scope, const FVariant& variant);
	static void StringAsVariant(TCallScope<bool(*)(const FString&, EVariantTypes, FVariant&)>& Scope, const FString& string, EVariantTypes variantType, FVariant& outVariant);
};
