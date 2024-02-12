#include "Commands/ApGetItemCommand.h"

//TODO REMOVE
#pragma optimize("", off)

AApGetItemCommand::AApGetItemCommand() {
	CommandName = TEXT("ap-getitem");
	Usage = TEXT("/ap-getitem <item-name> - asks the server to send you the specified AP item, the request can be rejected by the server if you don't have permission");
	MinNumberOfArguments = 1;
	Aliases.Add(TEXT("getitem"));
}

EExecutionStatus AApGetItemCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	FString message = FString::Join(Arguments, TEXT(" "));

	AApSubsystem::Get(GetWorld())->Say("!getitem " + message);

	return EExecutionStatus::COMPLETED;
}

#pragma optimize("", on)