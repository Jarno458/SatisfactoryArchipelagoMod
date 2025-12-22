#include "Commands/ResyncCommand.h"
#include "Subsystem/ApServerRandomizerSubsystem.h"

AResyncCommand::AResyncCommand() {
	CommandName = TEXT("resync");
	Usage = NSLOCTEXT("Archipelago", "ApResyncCommandUsage", "/resync - resets the internal item duplication counter, save and reload the save afterwards");
	MinNumberOfArguments = 0;
}

EExecutionStatus AResyncCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {

	AApServerRandomizerSubsystem::Get(GetWorld())->ResetDuplicationCounter();

	return EExecutionStatus::COMPLETED;
}
