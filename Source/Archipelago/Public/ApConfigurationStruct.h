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
    int Timeout{};

    UPROPERTY(BlueprintReadWrite)
    bool Debugging{};

    UPROPERTY(BlueprintReadWrite)
    bool SaveMode{};

    UPROPERTY(BlueprintReadWrite)
    bool OtherMessages{};

    UPROPERTY(BlueprintReadWrite)
    bool ItemSendMessages{};

    UPROPERTY(BlueprintReadWrite)
    bool ItemReceivedMessages{};

    UPROPERTY(BlueprintReadWrite)
    bool CountdownMessages{};

    UPROPERTY(BlueprintReadWrite)
    bool HintMessages{};

	 /* Retrieves active configuration value and returns object of this struct containing it */
    static FApConfigurationStruct GetActiveConfig(UWorld* WorldContext) {
        FApConfigurationStruct ConfigStruct{};
        FConfigId ConfigId{"Archipelago", ""};
        UConfigManager* ConfigManager = WorldContext->GetGameInstance()->GetSubsystem<UConfigManager>();
        ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{FApConfigurationStruct::StaticStruct(), &ConfigStruct});
        return ConfigStruct;
    }
};

