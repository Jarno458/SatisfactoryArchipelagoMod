#include "Commands/ApDeathLinkCommand.h"

//TODO REMOVE
#pragma optimize("", off)

AApDeathLinkCommand::AApDeathLinkCommand() {
	CommandName = TEXT("ap-deathlink");
	Usage = NSLOCTEXT("Archipelago", "ApDeathlinkCommandUsage", "/ap-deathlink - list all clients that have deathlink enabled");
	Aliases.Add(TEXT("deathlink"));	
	Aliases.Add(TEXT("status-deathlink"));
}

EExecutionStatus AApDeathLinkCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	AApSubsystem::Get(GetWorld())->Say(TEXT("!status DeathLink"));

	return EExecutionStatus::COMPLETED;
}

#pragma optimize("", on)