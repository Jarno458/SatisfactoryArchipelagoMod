#include "Commands/ResyncCommand.h"
#include "Subsystem/ApServerRandomizerSubsystem.h"

//TODO REMOVE
#pragma optimize("", off)

AResyncCommand::AResyncCommand() {
	CommandName = TEXT("resync");
	Usage = FText::FromString(TEXT("/resync - resets the internal item duplication counter, save and reload the save afterwards"));
	MinNumberOfArguments = 0;
}

EExecutionStatus AResyncCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {

	AApServerRandomizerSubsystem::Get(GetWorld())->ResetDuplicationCounter();

	return EExecutionStatus::COMPLETED;
}

#pragma optimize("", on)