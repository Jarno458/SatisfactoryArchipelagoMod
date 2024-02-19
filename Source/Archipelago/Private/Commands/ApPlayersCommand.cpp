#include "Commands/ApPlayersCommand.h"

//TODO REMOVE
#pragma optimize("", off)

AApPlayersCommand::AApPlayersCommand() {
	CommandName = TEXT("ap-players");
	Usage = TEXT("/ap-players - Get information about connected Archipelago players");
	Aliases.Add(TEXT("players"));
}

EExecutionStatus AApPlayersCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	AApSubsystem::Get(GetWorld())->Say("!players");

	return EExecutionStatus::COMPLETED;
}

#pragma optimize("", on)