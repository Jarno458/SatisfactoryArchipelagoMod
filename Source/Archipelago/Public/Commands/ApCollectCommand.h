#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"
#include "Command/CommandSender.h"

#include "Subsystem/ApSubsystem.h"

#include "ApCollectCommand.generated.h"

UCLASS()
class AApCollectCommand: public AChatCommandInstance
{
	GENERATED_BODY()

public:
	AApCollectCommand();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
