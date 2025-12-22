#include "Commands/ApSayCommand.h"

AApSayCommand::AApSayCommand() {
	CommandName = TEXT("ap-say");
	Usage = NSLOCTEXT("Archipelago", "ApSayCommandUsage", "/ap-say <message> - send chat message to AP server");
	MinNumberOfArguments = 1;
	Aliases.Add(TEXT("say"));
}

EExecutionStatus AApSayCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	FString message = FString::Join(Arguments, TEXT(" "));

	AApSubsystem::Get(GetWorld())->Say(message);

	return EExecutionStatus::COMPLETED;
}
