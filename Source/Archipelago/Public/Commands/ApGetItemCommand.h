#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"
#include "Command/CommandSender.h"

#include "Subsystem/ApSubsystem.h"

#include "ApGetItemCommand.generated.h"

UCLASS()
class AApGetItemCommand: public AChatCommandInstance
{
	GENERATED_BODY()

public:
	AApGetItemCommand();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
