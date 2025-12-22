#include "Commands/ApHintLocationCommand.h"

AApHintLocationCommand::AApHintLocationCommand() {
	CommandName = TEXT("ap-hintlocation");
	Usage = NSLOCTEXT("Archipelago", "ApHintLocationCommandUsage", "/ap-hintlocation <location-name> - attempt to hint what is located at the specified location");
	MinNumberOfArguments = 1;
	Aliases.Add(TEXT("hintlocation"));
	Aliases.Add(TEXT("ap-hint_location"));
	Aliases.Add(TEXT("hint_location"));
}

EExecutionStatus AApHintLocationCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	FString message = FString::Join(Arguments, TEXT(" "));

	AApSubsystem::Get(GetWorld())->Say("!hint_location " + message);

	return EExecutionStatus::COMPLETED;
}
