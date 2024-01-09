#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"
#include "Command/CommandSender.h"

#include "Subsystem/ApSubsystem.h"

#include "ApReleaseCommand.generated.h"

UCLASS()
class AApReleaseCommand : public AChatCommandInstance
{
	GENERATED_BODY()

public:
	AApReleaseCommand();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
