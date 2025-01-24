#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"
#include "Command/CommandSender.h"

#include "Subsystem/ApSubsystem.h"

#include "ResyncCommand.generated.h"

UCLASS()
class AResyncCommand : public AChatCommandInstance
{
	GENERATED_BODY()

public:
	AResyncCommand();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
