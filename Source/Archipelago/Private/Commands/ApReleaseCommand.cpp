#include "Commands/ApReleaseCommand.h"

//TODO REMOVE
#pragma optimize("", off)

AApReleaseCommand::AApReleaseCommand() {
	CommandName = TEXT("ap-release");
	Usage = TEXT("/ap-release - attempts to send out all your locations to other players' worlds");
	Aliases.Add(TEXT("release"));
	Aliases.Add(TEXT("ap-forfeit"));
	Aliases.Add(TEXT("forfeit"));
}

EExecutionStatus AApReleaseCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	AApSubsystem::Get(GetWorld())->Say(TEXT("!release"));

	return EExecutionStatus::COMPLETED;
}

#pragma optimize("", on)