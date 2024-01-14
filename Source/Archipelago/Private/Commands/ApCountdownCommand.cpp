#include "Commands/ApCountdownCommand.h"

//TODO REMOVE
#pragma optimize("", off)

AApCountdownCommand::AApCountdownCommand() {
	CommandName = TEXT("ap-countdown");
	Usage = TEXT("/ap-countdown <seconds> - starts a countdown on the server thats broadcasted to all clients");
	MinNumberOfArguments = 1;
	Aliases.Add(TEXT("countdown"));
}

EExecutionStatus AApCountdownCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	if (Arguments.Num() != 1) {
		return EExecutionStatus::BAD_ARGUMENTS;
	}

	AApSubsystem::Get(GetWorld())->Say("!countdown " + Arguments[0]);

	return EExecutionStatus::COMPLETED;
}

#pragma optimize("", on)