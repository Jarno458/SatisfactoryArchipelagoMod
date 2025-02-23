#include "Commands/ApAliasCommand.h"

//TODO REMOVE
#pragma optimize("", off)

AApAliasCommand::AApAliasCommand() {
	CommandName = TEXT("ap-alias");
	Usage = FText::FromString(TEXT("/ap-alias <new alias> - Sets your alias, which allows you to use commands with the alias rather than your provided slot name.")).ToString();
	Aliases.Add(TEXT("alias"));
	MinNumberOfArguments = 1;
}

EExecutionStatus AApAliasCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	FString message = FString::Join(Arguments, TEXT(" "));

	AApSubsystem::Get(GetWorld())->Say("!alias " + message);

	return EExecutionStatus::COMPLETED;
}

#pragma optimize("", on)