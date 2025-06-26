#include "Module/ApGameInstanceModule.h"

DEFINE_LOG_CATEGORY(LogApGameInstanceModule);

#define LOCTEXT_NAMESPACE "Archipelago"

UApGameInstanceModule::UApGameInstanceModule()
{
	UE_LOG(LogApGameInstanceModule, Display, TEXT("UApGameInstanceModule::UApGameInstanceModule()"));
}

#undef LOCTEXT_NAMESPACE
