#include "Module/ApGameInstanceModule.h"
#include "SessionSettings/SessionSettingsManager.h"
#include "Settings/SMLOptionsLibrary.h"

DEFINE_LOG_CATEGORY(LogApGameInstanceModule);

#define LOCTEXT_NAMESPACE "Archipelago"

//TODO REMOVE
#pragma optimize("", off)

UApGameInstanceModule::UApGameInstanceModule()
{
	UE_LOG(LogApGameInstanceModule, Display, TEXT("UApGameInstanceModule::UApGameInstanceModule()"));
}

void UApGameInstanceModule::DispatchLifecycleEvent(ELifecyclePhase phase) {
	Super::DispatchLifecycleEvent(phase);

	if (phase == ELifecyclePhase::CONSTRUCTION && !WITH_EDITOR) {
#if UE_SERVER
		SUBSCRIBE_METHOD(UFGServerStateController::Handler_GetServerOptions, [this](auto& scope, const UFGServerStateController* self, TMap<FString, FString>& OutServerOptions, TMap<FString, FString>& OutPendingServerOptions) {
			return Server_GetOptions(scope, self, OutServerOptions, OutPendingServerOptions);
		});
		SUBSCRIBE_METHOD(UFGServerManagementController::Handler_ApplyServerOptions, [this](auto& scope, const UFGServerManagementController* self, const TMap<FString, FString>& UpdatedServerOptions) {
			return Server_ApplyOptions(scope, self, UpdatedServerOptions);
		});
#else
		SUBSCRIBE_METHOD(UFGServerObjectOptionAdapter::ReceiveServerSettings, [this](auto& scope, UFGServerObjectOptionAdapter* self, const TMap<FString, FString>& InServerSettings, const TMap<FString, FString>& PendingServerOptions) {
			return Client_ReceiveServerSettings(scope, self, InServerSettings, PendingServerOptions);
		});
		SUBSCRIBE_METHOD(UFGServerObjectOptionAdapter::WriteChangedSettings, [this](auto& scope, UFGServerObjectOptionAdapter* self, TMap<FString, FString>& OutServerSettings) {
			return Client_WriteChangedSettings(scope, self, OutServerSettings);
		});
#endif
	}
}

#if UE_SERVER
void UApGameInstanceModule::Server_GetOptions(
	TCallScope<void(*)(const UFGServerStateController*, TMap<FString, FString>&, TMap<FString, FString>&)>& Scope,
	const UFGServerStateController* self, TMap<FString, FString>& OutServerOptions, TMap<FString, FString>& OutPendingServerOptions) {
	UE_LOG(LogApGameInstanceModule, Display, TEXT("UApGameInstanceModule::Server::GetServerOptions()"));

	Scope(self, OutServerOptions, OutPendingServerOptions);

	UWorld* world = GetWorld();
	fgcheck(world);

	USessionSettingsManager* sessionSettings = GetWorld()->GetSubsystem<USessionSettingsManager>();
	fgcheck(sessionSettings);

	FString uri = USMLOptionsLibrary::GetStringOptionValue(sessionSettings, "Archipelago.Connection.ServerURI").TrimStartAndEnd();
	FString user = USMLOptionsLibrary::GetStringOptionValue(sessionSettings, "Archipelago.Connection.UserName").TrimStartAndEnd();
	FString password = USMLOptionsLibrary::GetStringOptionValue(sessionSettings, "Archipelago.Connection.Password");

	OutServerOptions["Archipelago.Connection.ServerURI"] = uri;
	OutServerOptions["Archipelago.Connection.UserName"] = user;
	OutServerOptions["Archipelago.Connection.Password"] = password;
}

void UApGameInstanceModule::Server_ApplyOptions(
	TCallScope<void(*)(const UFGServerManagementController*, const TMap<FString, FString>&)>& Scope,
	const UFGServerManagementController* self, const TMap<FString, FString>& UpdatedServerOptions) {
	UE_LOG(LogApGameInstanceModule, Display, TEXT("UApGameInstanceModule::Server::ApplyServerOptions()"));

	UWorld* world = GetWorld();
	fgcheck(world);

	USessionSettingsManager* sessionSettings = GetWorld()->GetSubsystem<USessionSettingsManager>();
	fgcheck(sessionSettings);

	if (UpdatedServerOptions.Contains("Archipelago.Connection.ServerURI"))
		USMLOptionsLibrary::SetStringOptionValue(sessionSettings, "Archipelago.Connection.ServerURI", UpdatedServerOptions["Archipelago.Connection.ServerURI"]);
	if (UpdatedServerOptions.Contains("Archipelago.Connection.UserName"))
		USMLOptionsLibrary::SetStringOptionValue(sessionSettings, "Archipelago.Connection.UserName", UpdatedServerOptions["Archipelago.Connection.UserName"]);
	if (UpdatedServerOptions.Contains("Archipelago.Connection.Password"))
		USMLOptionsLibrary::SetStringOptionValue(sessionSettings, "Archipelago.Connection.Password", UpdatedServerOptions["Archipelago.Connection.Password"]);

	Scope(self, UpdatedServerOptions);
}

#else

void UApGameInstanceModule::Client_ReceiveServerSettings(
	TCallScope<void(*)(UFGServerObjectOptionAdapter*, const TMap<FString, FString>&, const TMap<FString, FString>&)>& Scope,
	UFGServerObjectOptionAdapter* self, const TMap<FString, FString>& InServerSettings, const TMap<FString, FString>& PendingServerOptions) {
	UE_LOG(LogApGameInstanceModule, Display, TEXT("UApGameInstanceModule::Client::ReceiveServerSettings()"));

	auto debug = 5;

	Scope(self, InServerSettings, PendingServerOptions);

	if (InServerSettings.Contains("Archipelago.Connection.ServerURI"))
		USMLOptionsLibrary::SetStringOptionValue(self, "Archipelago.Connection.ServerURI", InServerSettings["Archipelago.Connection.ServerURI"]);
	if (InServerSettings.Contains("Archipelago.Connection.UserName"))
		USMLOptionsLibrary::SetStringOptionValue(self, "Archipelago.Connection.UserName", InServerSettings["Archipelago.Connection.UserName"]);
	if (InServerSettings.Contains("Archipelago.Connection.Password"))
		USMLOptionsLibrary::SetStringOptionValue(self, "Archipelago.Connection.Password", InServerSettings["Archipelago.Connection.Password"]);

	auto debug2 = 6;
}

void UApGameInstanceModule::Client_WriteChangedSettings(
	TCallScope<void(*)(UFGServerObjectOptionAdapter*, TMap<FString, FString>&)>& Scope,
	UFGServerObjectOptionAdapter* self, TMap<FString, FString>& OutServerSettings) {

	UE_LOG(LogApGameInstanceModule, Display, TEXT("UApGameInstanceModule::Client::WriteChangedSettings()"));

	Scope(self, OutServerSettings);

	if (OutServerSettings.Contains("Archipelago.Connection.ServerURI"))
		OutServerSettings["Archipelago.Connection.ServerURI"] = self->GetOptionDisplayValueTyped<FString>("Archipelago.Connection.ServerURI");
	if (OutServerSettings.Contains("Archipelago.Connection.UserName"))
		OutServerSettings["Archipelago.Connection.UserName"] = self->GetOptionDisplayValueTyped<FString>("Archipelago.Connection.UserName");
	if (OutServerSettings.Contains("Archipelago.Connection.Password"))
		OutServerSettings["Archipelago.Connection.Password"] = self->GetOptionDisplayValueTyped<FString>("Archipelago.Connection.Password");
}
#endif

#pragma optimize("", on)

#undef LOCTEXT_NAMESPACE
