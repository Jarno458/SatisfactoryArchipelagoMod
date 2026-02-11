#include "Subsystem/ApPlayerInfoSubsystem.h"

#include "EngineUtils.h"
#include "NameAsStringProxyArchive.h"
#include "ReliableMessagingPlayerComponent.h"
#include "SubsystemActorManager.h"

DEFINE_LOG_CATEGORY(LogApPlayerInfoSubsystem);

#pragma optimize("", off)

FArchive& operator<<(FArchive& Ar, FReplicatedFApPlayerInfo& Info)
{
	Ar << Info.Team;
	Ar << Info.Slot;
	Ar << Info.Name;
	Ar << Info.Game;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FPlayerInfoSubsystemInitialReplicationMessage& Message)
{
	Ar << Message.PlayerInfos;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FPlayerInfoSubsystemUpdateReplicationMessage& Message)
{
	Ar << Message.PlayerInfos;
	return Ar;
}

AApPlayerInfoSubsystem* AApPlayerInfoSubsystem::Get(UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApPlayerInfoSubsystem>();
}

AApPlayerInfoSubsystem* AApPlayerInfoSubsystem::Get(UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApPlayerInfoSubsystem::AApPlayerInfoSubsystem() : Super() {
	UE_LOG(LogApPlayerInfoSubsystem, Display, TEXT("AApPlayerInfoSubsystem::AApPlayerInfoSubsystem()"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.1f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnLocal;
}

void AApPlayerInfoSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApPlayerInfoSubsystem, Display, TEXT("AApPlayerInfoSubsystem::BeginPlay()"));

	UWorld* world = GetWorld();
	const ENetMode ActiveNetMode = world->GetNetMode();

	//cant use Authority checks here because its locally spawned
	if (ActiveNetMode != ENetMode::NM_Client) {
		ap = AApSubsystem::Get(world);
		connectionInfoSubsystem = AApConnectionInfoSubsystem::Get(world);
	}
}

void AApPlayerInfoSubsystem::Tick(float dt) {
	Super::Tick(dt);

	//cant use Authority checks here because its locally spawned
	const ENetMode ActiveNetMode = GetWorld()->GetNetMode();
	if (ActiveNetMode == ENetMode::NM_Client)
	{
		SetActorTickEnabled(false);
		return;
	}

	if (connectionInfoSubsystem->GetConnectionState() == EApConnectionState::Connected) {
		const TMap<FApPlayer, TTuple<FString, FString>>& players = ap->GetAllApPlayers();

		if (!isInitialized)
		{
			TArray<FReplicatedFApPlayerInfo> playerInfos;

			for (const TPair<FApPlayer, TTuple<FString, FString>>& playerInfo : players)
			{
				FString name = playerInfo.Value.Get<0>();
				FString game = playerInfo.Value.Get<1>();

				playerInfos.Add(FReplicatedFApPlayerInfo(playerInfo.Key.Team, playerInfo.Key.Slot, name, game));
			}

			InitializeData(playerInfos);

			SendInitialReplicationDataForAllClients();

			PrimaryActorTick.TickInterval = 30.0f;

			return;
		}

		TArray<FReplicatedFApPlayerInfo> playerInfosToUpdate;

		for (const TPair<FApPlayer, TTuple<FString, FString>>& playerInfo : players)
		{
			FString name = playerInfo.Value.Get<0>();

			if (PlayerNamesMap.Contains(playerInfo.Key) && PlayerNamesMap[playerInfo.Key] != name)
			{
				playerInfosToUpdate.Add(FReplicatedFApPlayerInfo(playerInfo.Key, name, TEXT("")));

				PlayerNamesMap[playerInfo.Key] = name;
			}
		}

		if (!playerInfosToUpdate.IsEmpty()) {
			UpdateReplicationDataForAllClients(playerInfosToUpdate);
		}
	}
}

FString AApPlayerInfoSubsystem::GetPlayerName(FApPlayer player) const {
	if (PlayerNamesMap.Contains(player)) {
		if (hasMultipleTeams) {
			return FString::Format(TEXT("{0} (Team {1})"), { PlayerNamesMap[player], player.Team });
		}
		else {
			return PlayerNamesMap[player];
		}
	}

	return FString::Format(TEXT("Team: {0}. Slot: {1}"), { player.Team, player.Slot });
}

FString AApPlayerInfoSubsystem::GetPlayerGame(FApPlayer player) const {
	if (PlayerGamesMap.Contains(player)) {
		return PlayerGamesMap[player];
	}

	return TEXT("");
}

int AApPlayerInfoSubsystem::GetPlayerCount() const {
	return PlayerNamesMap.Num();
}

TSet<int> AApPlayerInfoSubsystem::GetTeams() const {
	TSet<int> teams;

	for (TPair<FApPlayer, FString> NamesMap : PlayerNamesMap)
	{
		if (!teams.Contains(NamesMap.Key.Team))
		{
			teams.Add(NamesMap.Key.Team);
		}
	}

	return teams;
}

TArray<FApPlayer> AApPlayerInfoSubsystem::GetAllPlayers() const
{
	TArray<FApPlayer> players;

	for (const TPair<FApPlayer, FString>& player : PlayerNamesMap)
	{
		if (player.Key.Slot != 0) {
			players.Add(player.Key);;
		}
	}	

	return players;
}

TArray<FString> AApPlayerInfoSubsystem::GetAllGames() const
{
	TArray<FString> games;

	for (TPair<FApPlayer, FString> NamesMap : PlayerNamesMap)
	{
		FString game = GetPlayerGame(NamesMap.Key);

		if (!game.IsEmpty()) {
			games.Add(game);
		}
	}

	return games;
}

void AApPlayerInfoSubsystem::SendInitialReplicationDataForAllClients() {
	for (TActorIterator<APlayerController> actorItterator(GetWorld()); actorItterator; ++actorItterator) {
		APlayerController* PlayerController = *actorItterator;
		if (!IsValid(PlayerController) || PlayerController->IsLocalController())
			continue;

		SendInitialReplicationData(PlayerController);
	}
}

void AApPlayerInfoSubsystem::SendInitialReplicationData(const APlayerController* PlayerController)
{
	TArray<FReplicatedFApPlayerInfo> PlayerInfos;

	for (const TPair<FApPlayer, FString>& namesMap : PlayerNamesMap)
	{
		PlayerInfos.Add(FReplicatedFApPlayerInfo(namesMap.Key, namesMap.Value, PlayerGamesMap[namesMap.Key]));
	}

	FPlayerInfoSubsystemInitialReplicationMessage Message;
	Message.PlayerInfos = PlayerInfos;

	UE_LOG(LogApPlayerInfoSubsystem, Log, TEXT("Sending initial replication message with %d items in the lookup array to player %s"), Message.PlayerInfos.Num(), *GetPathName(GetOwner()));

	SendRawMessage(PlayerController, Message.MessageId, [&](FArchive& Ar) { Ar << Message; });
}

void AApPlayerInfoSubsystem::UpdateReplicationDataForAllClients(const TArray<FReplicatedFApPlayerInfo>& playerInfosToUpdate) const {
	for (TActorIterator<APlayerController> actorItterator(GetWorld()); actorItterator; ++actorItterator) {
		APlayerController* PlayerController = *actorItterator;

		if (!IsValid(PlayerController) || PlayerController->IsLocalController())
			continue;

		SendUpdatedReplicationData(PlayerController, playerInfosToUpdate);
	}
}

void AApPlayerInfoSubsystem::SendUpdatedReplicationData(APlayerController* PlayerController, const TArray<FReplicatedFApPlayerInfo> playerInfos) const {
	FPlayerInfoSubsystemUpdateReplicationMessage Message;
	Message.PlayerInfos = playerInfos;

	UE_LOG(LogApPlayerInfoSubsystem, Log, TEXT("Sending partial update replication message with %d items in the lookup array to player %s"), Message.PlayerInfos.Num(), *GetPathName(GetOwner()));

	SendRawMessage(PlayerController, Message.MessageId, [&](FArchive& Ar) { Ar << Message; });
}

void AApPlayerInfoSubsystem::InitializeData(TArray<FReplicatedFApPlayerInfo> playerInfos)
{
	for (const FReplicatedFApPlayerInfo& playerInfo : playerInfos)
	{
		PlayerNamesMap.Add(playerInfo, playerInfo.Name);
		PlayerGamesMap.Add(playerInfo, playerInfo.Game);

		if (playerInfo.Team >= 0) {
			hasMultipleTeams = true;
		}
	}

	isInitialized = true;
}

void AApPlayerInfoSubsystem::OnPlayerControllerBeginPlay(const APlayerController* PlayerController)
{
	if (!IsValid(PlayerController))
		return;

	// We are Server, and this is a remote player. Send descriptor lookup array to the client
	if (PlayerController->HasAuthority() && !PlayerController->IsLocalController())
	{
		if (isInitialized) //if the server has already initialized, send the data right away, (likely this player is a late joiner)
			SendInitialReplicationData(PlayerController);
	}
	// We are local player connected to a server, register the message handler
	else if (!PlayerController->HasAuthority() && PlayerController->IsLocalController())
	{
		//register data received handler
		if (UReliableMessagingPlayerComponent* PlayerComponent = UReliableMessagingPlayerComponent::GetFromPlayer(PlayerController))
		{
			UE_LOG(LogApPlayerInfoSubsystem, Log, TEXT("Registered message handler for local player"));
			PlayerComponent->RegisterMessageHandler(RELIABLE_MESSAGING_CHANNEL_ID_PLAYER_INFO,
				UReliableMessagingPlayerComponent::FOnBulkDataReplicationPayloadReceived::CreateUObject(this, &AApPlayerInfoSubsystem::OnRawDataReceived));
		}
	}
}

void AApPlayerInfoSubsystem::SendRawMessage(const APlayerController* PlayerController, EPlayerInfoSubsystemMessageId MessageId, const TFunctionRef<void(FArchive&)>& MessageSerializer) const
{
	TArray<uint8> RawMessageData;
	FMemoryWriter RawMessageMemoryWriter(RawMessageData);
	FNameAsStringProxyArchive NameAsStringProxyArchive(RawMessageMemoryWriter);

	NameAsStringProxyArchive << MessageId;
	MessageSerializer(NameAsStringProxyArchive);

	UReliableMessagingPlayerComponent* PlayerComponent = UReliableMessagingPlayerComponent::GetFromPlayer(PlayerController);
	if (ensure(PlayerComponent))
	{
		UE_LOG(LogApPlayerInfoSubsystem, Log, TEXT("Sending Message ID %d with %d bytes of payload to player %s"), MessageId, RawMessageData.Num(), *GetPathName(GetOwner()));
		PlayerComponent->SendMessage(RELIABLE_MESSAGING_CHANNEL_ID_PLAYER_INFO, MoveTemp(RawMessageData));
	}
}

void AApPlayerInfoSubsystem::OnRawDataReceived(TArray<uint8>&& InMessageData)
{
	FMemoryReader RawMessageMemoryReader(InMessageData);
	FNameAsStringProxyArchive NameAsStringProxyArchive(RawMessageMemoryReader);

	EPlayerInfoSubsystemMessageId MessageId{};
	NameAsStringProxyArchive << MessageId;

	UE_LOG(LogApPlayerInfoSubsystem, Log, TEXT("Received Message ID %d with %d bytes of payload"), MessageId, InMessageData.Num());
	if (NameAsStringProxyArchive.IsError()) return;

	switch (MessageId)
	{
	case EPlayerInfoSubsystemMessageId::InitialReplication:
	{
		FPlayerInfoSubsystemInitialReplicationMessage InitialReplicationMessage;
		NameAsStringProxyArchive << InitialReplicationMessage;

		if (NameAsStringProxyArchive.IsError())
			return;

		ReceiveInitialReplicationData(InitialReplicationMessage);
		break;
	}
	case EPlayerInfoSubsystemMessageId::PartialUpdate:
	{
		FPlayerInfoSubsystemUpdateReplicationMessage partialReplicationMessage;
		NameAsStringProxyArchive << partialReplicationMessage;

		if (NameAsStringProxyArchive.IsError())
			return;

		ReceiveInitialReplicationData(partialReplicationMessage);
		break;
	}
	}
}

void AApPlayerInfoSubsystem::ReceiveInitialReplicationData(const FPlayerInfoSubsystemInitialReplicationMessage& Message)
{
	UE_LOG(LogApPlayerInfoSubsystem, Log, TEXT("Received %d player infos from the server"), Message.PlayerInfos.Num());

	InitializeData(Message.PlayerInfos);
}

void AApPlayerInfoSubsystem::ReceiveInitialReplicationData(const FPlayerInfoSubsystemUpdateReplicationMessage& Message)
{
	UE_LOG(LogApPlayerInfoSubsystem, Log, TEXT("Received %d player infos as a partial update from the server"), Message.PlayerInfos.Num());

	for (const FReplicatedFApPlayerInfo& playerInfo : Message.PlayerInfos)
	{
		if (PlayerNamesMap.Contains(playerInfo))
		{
			PlayerNamesMap[playerInfo] = playerInfo.Name;
		}
	}
}

#pragma optimize("", on)
