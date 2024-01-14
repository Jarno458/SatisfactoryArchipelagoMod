#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"
#include "Command/CommandSender.h"

#include "Subsystem/ApSubsystem.h"

#include "ApCountdownCommand.generated.h"

UCLASS()
class AApCountdownCommand : public AChatCommandInstance
{
	GENERATED_BODY()

public:
	AApCountdownCommand();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
