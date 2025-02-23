#include "Commands/ApPlayersCommand.h"

//TODO REMOVE
#pragma optimize("", off)

AApPlayersCommand::AApPlayersCommand() {
	CommandName = TEXT("ap-players");
	Usage = FText::FromString(TEXT("/ap-players - Get information about connected Archipelago players")).ToString();
	Aliases.Add(TEXT("players"));
}

EExecutionStatus AApPlayersCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	AApSubsystem::Get(GetWorld())->Say("!players");

	return EExecutionStatus::COMPLETED;
}

#pragma optimize("", on)