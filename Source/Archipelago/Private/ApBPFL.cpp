#include "ApBPFL.h"
#include "SessionSettings/SessionSettingsManager.h"

DEFINE_LOG_CATEGORY(LogApBPFL);

//TODO REMOVE
#pragma optimize("", off)

/*
void UApBPFL::SetSessionSetting(const FString& cvar, TScriptInterface<class IFGOptionInterface> optionsInterface) {
	UE_LOG(LogApBPFL, Display, TEXT("Setting session setting"));

	optionsInterface->SetNameOptionValue(cvar, "Testing setting save");
	auto optionName = optionsInterface->GetNameOptionValue(cvar);
	optionsInterface->ApplyChanges();
	auto optionNameAfterApply = optionsInterface->GetNameOptionValue(cvar);

	auto debug = true;
}
*/

#pragma optimize("", on)