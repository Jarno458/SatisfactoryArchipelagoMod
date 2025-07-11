#include "Module/ApGameInstanceModule.h"
#include "Settings/SMLOptionsLibrary.h"
#include "Logging/StructuredLog.h"

#if UE_SERVER
#include "Controller/FGServerStateController.h"
#include "Controller/FGServerManagementController.h"
#endif

DEFINE_LOG_CATEGORY(LogApGameInstanceModule);

#define LOCTEXT_NAMESPACE "Archipelago"

UApGameInstanceModule::UApGameInstanceModule()
{
	UE_LOGFMT(LogApGameInstanceModule, Display, "UApGameInstanceModule::UApGameInstanceModule()");
}

void UApGameInstanceModule::DispatchLifecycleEvent(ELifecyclePhase phase) {
	Super::DispatchLifecycleEvent(phase);

	if (phase == ELifecyclePhase::CONSTRUCTION && !WITH_EDITOR) {
#if UE_SERVER
		SUBSCRIBE_METHOD(UFGServerStateController::Handler_GetServerOptions, [this](auto& func, const UFGServerStateController* self, TMap<FString, FString>& OutServerOptions, TMap<FString, FString>& OutPendingServerOptions) {
			func(self, OutServerOptions, OutPendingServerOptions);
			DediServer_GetOptions(OutServerOptions, OutPendingServerOptions);
		});
		SUBSCRIBE_METHOD(UFGServerManagementController::Handler_ApplyServerOptions, [this](auto& func, const UFGServerManagementController* self, const TMap<FString, FString>& UpdatedServerOptions) {
			func(self, UpdatedServerOptions);
			DediServer_ApplyOptions(UpdatedServerOptions);
		});
#endif

		SUBSCRIBE_METHOD(UFGUserSettingApplyType::VariantAsString, UApGameInstanceModule::VariantAsString);
		SUBSCRIBE_METHOD(UFGUserSettingApplyType::StringAsVariant, UApGameInstanceModule::StringAsVariant);
	}
}

void UApGameInstanceModule::DediServer_GetOptions(TMap<FString, FString>& OutServerOptions, TMap<FString, FString>& OutPendingServerOptions) {
	if (!IsRunningDedicatedServer())
		UE_LOGFMT(LogApGameInstanceModule, Fatal, "UApGameInstanceModule::DediServer_GetOptions() called outside of dedicated server");

	//handle conversion from session settings to sever options

	UWorld* world = GetWorld();
	fgcheck(world);

	USessionSettingsManager* sessionSettings = world->GetSubsystem<USessionSettingsManager>();
	fgcheck(sessionSettings);

	DediServer_CopySettingFromSessionSettings(sessionSettings, "Archipelago.Connection.ServerURI", OutServerOptions, OutPendingServerOptions);
	DediServer_CopySettingFromSessionSettings(sessionSettings, "Archipelago.Connection.UserName", OutServerOptions, OutPendingServerOptions);
	DediServer_CopySettingFromSessionSettings(sessionSettings, "Archipelago.Connection.Password", OutServerOptions, OutPendingServerOptions);
}

void UApGameInstanceModule::DediServer_ApplyOptions(const TMap<FString, FString>& UpdatedServerOptions) {
	if (!IsRunningDedicatedServer())
		UE_LOGFMT(LogApGameInstanceModule, Fatal, "UApGameInstanceModule::DediServer_ApplyOptions() called outside of dedicated server");

	UWorld* world = GetWorld();
	fgcheck(world);

	USessionSettingsManager* sessionSettings = world->GetSubsystem<USessionSettingsManager>();
	fgcheck(sessionSettings);

	DediServer_CopySettingToSessionSettings(sessionSettings, "Archipelago.Connection.ServerURI", UpdatedServerOptions);
	DediServer_CopySettingToSessionSettings(sessionSettings, "Archipelago.Connection.UserName", UpdatedServerOptions);
	DediServer_CopySettingToSessionSettings(sessionSettings, "Archipelago.Connection.Password", UpdatedServerOptions);
}

void UApGameInstanceModule::DediServer_CopySettingFromSessionSettings(const USessionSettingsManager* sessionSettings, const FString& cvar, TMap<FString, FString>& OutServerOptions, TMap<FString, FString>& OutPendingServerOptions) {
	if(!IsRunningDedicatedServer())
		UE_LOGFMT(LogApGameInstanceModule, Fatal, "UApGameInstanceModule::DediServer_CopySettingFromSessionSettings() called outside of dedicated server");
	
	UFGUserSettingApplyType* setting = sessionSettings->FindSessionSetting(cvar);
	UFGUserSettingApplyType_RequireSessionRestart* applyType = Cast<UFGUserSettingApplyType_RequireSessionRestart>(setting);

	FString current = applyType->GetAppliedValue().GetValue<FString>();

	OutServerOptions[cvar] = current;

	FVariant perndingVariant = applyType->GetPendingAppliedValue();
	if (!perndingVariant.IsEmpty()) {
		FString pendingValue = perndingVariant.GetValue<FString>();

		if (current != pendingValue)
		{
			if (OutPendingServerOptions.Contains(cvar))
				OutPendingServerOptions[cvar] = pendingValue;
			else
				OutPendingServerOptions.Add(cvar, pendingValue);
		}
	}

	TArray<FString> out_debugData;
	applyType->GetDebugData(out_debugData);
	UE_LOGFMT(LogApGameInstanceModule, Display, "UApGameInstanceModule::Server::GetServerOptions() applyType: {0}", out_debugData[0]);
}

void UApGameInstanceModule::DediServer_CopySettingToSessionSettings(const USessionSettingsManager* sessionSettings, const FString& cvar, const TMap<FString, FString>& UpdatedServerOptions) {
	if (!IsRunningDedicatedServer())
		UE_LOGFMT(LogApGameInstanceModule, Fatal, "UApGameInstanceModule::DediServer_CopySettingToSessionSettings() called outside of dedicated server");

	if (UpdatedServerOptions.Contains(cvar)) {
		UFGUserSettingApplyType* setting = sessionSettings->FindSessionSetting(cvar);
		UFGUserSettingApplyType_RequireSessionRestart* applyType = Cast<UFGUserSettingApplyType_RequireSessionRestart>(setting);

		applyType->ForceSetPendingAppliedValue(UpdatedServerOptions[cvar]);
		//the above line does not correctly apply the value of session restart so for now we just hard set the current value
		applyType->ForceSetValue(UpdatedServerOptions[cvar], false);

		if (!cvar.Contains("Password", ESearchCase::CaseSensitive)) {
			TArray<FString> out_debugData;
			applyType->GetDebugData(out_debugData);
			UE_LOGFMT(LogApGameInstanceModule, Display, "UApGameInstanceModule::Server::ApplyServerOptions() applyType: {0}", out_debugData[0]);
		}
	}
}

void UApGameInstanceModule::VariantAsString(TCallScope<FString(*)(const FVariant&)>& Scope, const FVariant& variant) {
	if (variant.GetType() == EVariantTypes::String)
		Scope.Override(variant.GetValue<FString>());
}

void UApGameInstanceModule::StringAsVariant(TCallScope<bool(*)(const FString&, EVariantTypes, FVariant&)>& Scope, const FString& string, EVariantTypes variantType, FVariant& outVariant) {
	if (variantType == EVariantTypes::String) {
		outVariant = FVariant(string);
		Scope.Override(true);
	}
}

#undef LOCTEXT_NAMESPACE
