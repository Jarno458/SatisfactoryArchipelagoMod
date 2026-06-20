#include "ApCheatManager.h"

#include "Subsystem/ApVaultSubsystem.h"

UApCheatManager::UApCheatManager() : Super() {
}

TMap<FString, FString> UApCheatManager::GetFunctionCategories() const
{
	return { { TEXT("AllowSelfGifting"), TEXT("Archipelago") } };
}

void UApCheatManager::AllowSelfGifting(bool enabled) const
{
	AApVaultSubsystem* vaultSubsystem = AApVaultSubsystem::Get(GetWorld());
	if (vaultSubsystem)
	{
		vaultSubsystem->AllowSelfGifting(enabled);
	}
}
