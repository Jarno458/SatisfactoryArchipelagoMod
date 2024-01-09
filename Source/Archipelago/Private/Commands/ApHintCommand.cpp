#include "Commands/ApHintCommand.h"

//TODO REMOVE
#pragma optimize("", off)

AApHintCommand::AApHintCommand() {
	CommandName = TEXT("ap-hint");
	Usage = TEXT("/ap-hint <item-name> - attempt to hint where the specified item is located");
	MinNumberOfArguments = 1;
	Aliases.Add(TEXT("hint"));
}

EExecutionStatus AApHintCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	FString message = FString::Join(Arguments, TEXT(" "));

	AApSubsystem::Get(GetWorld())->Say("!hint " + message);

	return EExecutionStatus::COMPLETED;
}

#pragma optimize("", on)