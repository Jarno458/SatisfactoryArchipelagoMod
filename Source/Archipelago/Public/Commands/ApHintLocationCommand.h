#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"
#include "Command/CommandSender.h"

#include "Subsystem/ApSubsystem.h"

#include "ApHintLocationCommand.generated.h"

UCLASS()
class AApHintLocationCommand: public AChatCommandInstance
{
	GENERATED_BODY()

public:
	AApHintLocationCommand();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
