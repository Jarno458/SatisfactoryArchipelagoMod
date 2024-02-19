#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"
#include "Command/CommandSender.h"

#include "Subsystem/ApSubsystem.h"

#include "ApPlayersCommand.generated.h"

UCLASS()
class AApPlayersCommand : public AChatCommandInstance {
	GENERATED_BODY()

public:
	AApPlayersCommand();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
