#pragma once

#include "FGCheatManager.h"
#include "GameFramework/CheatManager.h"

#include "ApCheatManager.generated.h"

UCLASS(NotBlueprintable)
class UApCheatManager final : public UCheatManagerExtension, public IFGCheatBoardFunctionCategoryProvider
{
	GENERATED_BODY()

public:
	UApCheatManager();

	// Begin IFGCheatBoardFunctionCategoryProvider interface
	virtual TMap<FString, FString> GetFunctionCategories() const override;
	// End IFGCheatBoardFunctionCategoryProvider interface

	UFUNCTION(Exec, CheatBoard)
	void AllowSelfGifting(bool enabled) const;
};