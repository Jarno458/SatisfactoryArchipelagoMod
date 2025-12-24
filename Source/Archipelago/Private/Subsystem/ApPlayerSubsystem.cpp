#include "ApPlayerSubsystem.h"
#include "EngineUtils.h"
#include "Subsystem/SubsystemActorManager.h"

#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(LogApPlayerSubsystem);

#define LOCTEXT_NAMESPACE "Archipelago"

#pragma optimize("", off)

AApPlayerSubsystem* AApPlayerSubsystem::Get(UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApPlayerSubsystem>();
}

AApPlayerSubsystem* AApPlayerSubsystem::Get(UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AApPlayerSubsystem::AApPlayerSubsystem() : Super() {
	UE_LOG(LogApPlayerSubsystem, Display, TEXT("AApPlayerSubsystem::AApPlayerSubsystem()"));

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AApPlayerSubsystem::BeginPlay() {
	Super::BeginPlay();

	AApSubsystem* ap = AApSubsystem::Get(GetWorld());
	ap->SetDeathLinkReceivedCallback([this](FText message) { OnDeathLinkReceived(message); });

	canTriggerDeathlinks = true;

	UE_LOG(LogApPlayerSubsystem, Display, TEXT("AApPlayerSubsystem::BeginPlay()"));
}

void AApPlayerSubsystem::OnPlayerDeath(AActor* deadActor, AActor* causee, const UDamageType* damageType) {
	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnPlayerDeath({0})", *deadActor->GetName());

	if (!HasAuthority() || !canTriggerDeathlinks)
		return;

	FString source(TEXT("Unknown"));
	FString cause(TEXT(""));

	if (!IsValid(deadActor))
		return;

	AFGCharacterPlayer* player = Cast<AFGCharacterPlayer>(deadActor);
	if (IsValid(player))
	{
		APlayerState* state = player->GetPlayerState();
		if (IsValid(state))
			source = state->GetPlayerName();
	}

	bool isTeamKill = false;
	if (causee->IsA<AFGCharacterPlayer>())
	{
		AFGCharacterPlayer* player2 = Cast<AFGCharacterPlayer>(causee);
		if (IsValid(player2))
		{
			if (player != player2)
				isTeamKill = true;
		}
	}

	FString damageName = damageType->GetName();

	if (damageName.EndsWith(TEXT("WorldBounds")))
		cause = TEXT("Flew out of the map");
	if (damageName.EndsWith(TEXT("Radiation")))
		cause = TEXT("Radiation poisoning");
	if (damageName.EndsWith(TEXT("Gas")))
		cause = TEXT("Suffocated in toxic gas");
	if (damageName.EndsWith(TEXT("_Fall")))
		cause = TEXT("Fell to their death");
	if (damageName.EndsWith(TEXT("Explosive")))
	{
		if (isTeamKill)
			cause = TEXT("Blown up by a teammate");
		else
			cause = TEXT("Blown up");
	}
	if (damageName.EndsWith(TEXT("Physical")))
	{
		if (isTeamKill)
			cause = TEXT("Teamkill");
		else
			cause = TEXT("Physical damage");
	}

	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnPlayerDeath source: {0}, cause: {1}", source, cause);

	ap->TriggerDeathLink(source, cause);

	MassMurder();
}

void AApPlayerSubsystem::OnDeathLinkReceived(FText message) {
	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnDeathLinkReceived({0})", message.ToString());

	if (!HasAuthority())
		return;

	MassMurder();
	
	//ap->AddChatMessage(message, FLinearColor::Red);
}

void AApPlayerSubsystem::MassMurder()
{
	canTriggerDeathlinks = false;

	for (TActorIterator<AFGCharacterPlayer> actorItterator(GetWorld()); actorItterator; ++actorItterator) {
		AFGCharacterPlayer* player = *actorItterator;

		if (!IsValid(player))
			continue;

		UFGHealthComponent* health = player->GetHealthComponent();

		if (!IsValid(health))
			continue;

		health->Kill();
	}

	canTriggerDeathlinks = true;
}

/*
void AApServerRandomizerSubsystem::HandleDeathLink() {
	if (IsRunningDedicatedServer())
		return; // TODO make deathlink work for dedicated servers

	const AFGPlayerController* playerController = UFGBlueprintFunctionLibrary::GetLocalPlayerController(GetWorld());
	if (playerController == nullptr)
		return;

	AFGCharacterPlayer* player = Cast<AFGCharacterPlayer>(playerController->GetControlledCharacter());
	if (player == nullptr)
		return;

	HandleInstagib(player);

	if (player->IsAliveAndWell()) {
		awaitingHealty = false;
	}
	else {
		if (!awaitingHealty) {
			if (!instagib) {
				ap->TriggerDeathLink();
			}
			awaitingHealty = true;
		}
	}
	if (instagib) {
		instagib = false;
	}
}

void AApServerRandomizerSubsystem::HandleInstagib(AFGCharacterPlayer* player) {
	if (instagib) {
		const TSubclassOf<UDamageType> damageType = TSubclassOf<UDamageType>(UDamageType::StaticClass());
		FDamageEvent instagibDamageEvent = FDamageEvent(damageType);
		player->TakeDamage(1333337, instagibDamageEvent, player->GetFGPlayerController(), player);
	}
}
*/

#pragma optimize("", on)

#undef LOCTEXT_NAMESPACE
