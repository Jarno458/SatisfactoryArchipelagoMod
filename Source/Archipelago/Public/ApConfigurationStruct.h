#pragma once
#include "CoreMinimal.h"
#include "Configuration/ConfigManager.h"
#include "Engine/Engine.h"
#include "ApConfigurationStruct.generated.h"

/* Struct generated from Mod Configuration Asset '/Archipelago/ApConfiguration' */
USTRUCT(BlueprintType)
struct FApConfigurationStruct {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    FString Url{};

    UPROPERTY(BlueprintReadWrite)
    FString Login{};

    UPROPERTY(BlueprintReadWrite)
    FString Password{};

    UPROPERTY(BlueprintReadWrite)
    FString Game{};

    UPROPERTY(BlueprintReadWrite)
    bool Enabled{};

    UPROPERTY(BlueprintReadWrite)
    bool Debugging{};

	/* Retrieves active configuration value and returns object of this struct containing it */
    static FApConfigurationStruct GetActiveConfig(UObject* WorldContext) {
        FApConfigurationStruct ConfigStruct{};
        FConfigId ConfigId{"Archipelago", ""};
        if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull)) {
            UConfigManager* ConfigManager = World->GetGameInstance()->GetSubsystem<UConfigManager>();
            ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{FApConfigurationStruct::StaticStruct(), &ConfigStruct});
        }
        return ConfigStruct;
    }
};

