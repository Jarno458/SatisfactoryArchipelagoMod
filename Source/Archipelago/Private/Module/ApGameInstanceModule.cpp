#include "Module/ApGameInstanceModule.h"

DEFINE_LOG_CATEGORY(LogApGameInstanceModule);

#define LOCTEXT_NAMESPACE "Archipelago"

UApGameInstanceModule::UApGameInstanceModule()
{
	UE_LOG(LogApGameInstanceModule, Display, TEXT("UApGameInstanceModule::UApGameInstanceModule()"));
}

void UApGameInstanceModule::YeetToMainMenu(APlayerController* player, FText reason) {
	UE_LOG(LogApGameInstanceModule, Display, TEXT("LogApGameInstanceModule::YeetToMainMenu(player, %s)"), *reason.ToString());

	UFGBlueprintFunctionLibrary::TravelToMainMenu(player);

	FPopupClosed closed;
	FText title = LOCTEXT("APInitFailed", "Archipelago Initialization Failed");
	UFGBlueprintFunctionLibrary::AddPopupWithCloseDelegate(player, title, reason, closed);

	// the above code does not work, so we yeet the entire game instead
	// LogGame: SendToMainMenu: Null player controller supplied
	UE_LOG(LogApGameInstanceModule, Fatal, TEXT("Archipelago Initialization Failed: %s"), *reason.ToString());
}
