#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"
#include "Command/CommandSender.h"

#include "Subsystem/ApSubsystem.h"

#include "ApAliasCommand.generated.h"

UCLASS()
class AApAliasCommand: public AChatCommandInstance
{
	GENERATED_BODY()

public:
	AApAliasCommand();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
