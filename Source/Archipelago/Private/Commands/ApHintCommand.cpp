#include "Commands/ApHintCommand.h"

//TODO REMOVE
#pragma optimize("", off)

AApHintCommand::AApHintCommand() {
	CommandName = TEXT("ap-hint");
	Usage = NSLOCTEXT("Archipelago", "ApHintCommandUsage", "/ap-hint <item-name> - attempt to hint where the specified item is located");
	Aliases.Add(TEXT("hint"));
}

EExecutionStatus AApHintCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	if (Arguments.Num() == 0) {
		AApSubsystem::Get(GetWorld())->Say("!hint");
	} else {
		FString message = FString::Join(Arguments, TEXT(" "));

		AApSubsystem::Get(GetWorld())->Say("!hint " + message);
	}

	return EExecutionStatus::COMPLETED;
}

#pragma optimize("", on)