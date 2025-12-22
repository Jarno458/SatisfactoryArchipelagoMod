#include "Commands/ApCollectCommand.h"

AApCollectCommand::AApCollectCommand() {
	CommandName = TEXT("ap-collect");
	Usage = NSLOCTEXT("Archipelago", "ApCollectCommandUsage", "/ap-collect - attempts to collect all of your items from inside other players' worlds");
	Aliases.Add(TEXT("collect"));
}

EExecutionStatus AApCollectCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	AApSubsystem::Get(GetWorld())->Say(TEXT("!collect"));

	return EExecutionStatus::COMPLETED;
}
