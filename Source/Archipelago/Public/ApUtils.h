#pragma once

#include "CoreMinimal.h"
#include "Resources/FGItemDescriptor.h"
#include "ApBlueprintDataBridge.h"
#include "Archipelago.h"
#include "BigInt.h"
#include "Module/ApGameInstanceModule.h"
#include "Data/ApTypes.h"
#include "Subsystem/SubsystemActorManager.h"

#include "ApUtils.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApUtils, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogArchipelagoCpp, Log, All);

/**
 * Archipelago Utility library
 */
UCLASS()
class ARCHIPELAGO_API UApUtils : public UObject
{
	GENERATED_BODY()

public:
	// Shorthand convert from std::string to FText
	static FText FText(std::string inString);

	// Shorthand convert from std::string to FString
	// Name is not exactly 'FString' because that causes naming problems
	static FString FStr(std::string inString);

	// Shorthand convert from int64 to FString
	// Name is not exactly 'FString' because that causes naming problems
	static FString FStr(int64 inInt);

	// Wrapper for FClassGenerator::GenerateSimpleClass that returns existing classes instead of crashing by creating new ones
	static TTuple<bool, UClass*> FindOrCreateClass(const TCHAR* packageName, const TCHAR* className, UClass* parentClass);

	static FString GetImagePathForItem(UFGItemDescriptor* item);

	static UApGameInstanceModule* GetGameInstanceModule(UObject* worldContext);

	static FString YankParseValueString(AP_SetReply& setReply);
	static int256 Int256FromBigIntString(FString bigInt);

	UFUNCTION(BlueprintCallable)
	static UApBlueprintDataBridge* GetBlueprintDataBridge(UObject* worldContext);

	static void WriteStringToFile(FString Path, FString text, bool relative);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool IsApPlayerValid(FApPlayer player);

	 template<typename T>
	static T* GetSubsystemActorIncludingParentClases(class UWorld* world) {
		USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
		fgcheck(SubsystemActorManager);

		for (const TPair<TSubclassOf<AModSubsystem>, AModSubsystem*>& subsystemEntry : SubsystemActorManager->GetSubsystemActors()) {
			if (subsystemEntry.Key->IsChildOf(T::StaticClass()))
				return Cast<T>(subsystemEntry.Value);
		}

		return nullptr;
	}
};
