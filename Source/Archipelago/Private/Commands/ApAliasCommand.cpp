#include "Commands/ApAliasCommand.h"

//TODO REMOVE
#pragma optimize("", off)

AApAliasCommand::AApAliasCommand() {
	CommandName = TEXT("ap-alias");
	Usage = TEXT("/ap-alias <new alias> - send chat message to AP server");
	Aliases.Add(TEXT("alias"));
	MinNumberOfArguments = 1;
}

EExecutionStatus AApAliasCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	FString message = FString::Join(Arguments, TEXT(" "));

	AApSubsystem::Get(GetWorld())->Say("!alias " + message);

	return EExecutionStatus::COMPLETED;
}

#pragma optimize("", on)