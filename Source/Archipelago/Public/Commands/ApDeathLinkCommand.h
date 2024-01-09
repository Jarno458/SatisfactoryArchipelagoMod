#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"
#include "Command/CommandSender.h"

#include "Subsystem/ApSubsystem.h"

#include "ApDeathLinkCommand.generated.h"

UCLASS()
class AApDeathLinkCommand: public AChatCommandInstance
{
	GENERATED_BODY()

public:
	AApDeathLinkCommand();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
