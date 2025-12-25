#include "ApPlayerSubsystem.h"
#include "EngineUtils.h"
#include "Subsystem/SubsystemActorManager.h"
#include "FGDriveablePawn.h"

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

	ap = AApSubsystem::Get(GetWorld());
	fgcheck(ap);

	ap->SetDeathLinkReceivedCallback([this](FString source, FString cause) { OnDeathLinkReceived(source, cause); });

	canTriggerDeathlinks = true;

	UE_LOG(LogApPlayerSubsystem, Display, TEXT("AApPlayerSubsystem::BeginPlay()"));
}

void AApPlayerSubsystem::OnPlayerDeath(AActor* deadActor, AActor* causee, const UDamageType* damageType) {
	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnPlayerDeath({0})", *deadActor->GetName());

	if (!HasAuthority() || !canTriggerDeathlinks)
		return;

	FString source(TEXT("Unknown"));

	if (!IsValid(deadActor))
		return;

	AFGCharacterPlayer* player = Cast<AFGCharacterPlayer>(deadActor);
	if (IsValid(player))
	{
		APlayerState* state = player->GetPlayerState();
		if (IsValid(state))
			source = state->GetPlayerName();
	}

	FString cause = GetDamageSuffix(damageType, deadActor, causee);

	if (!cause.IsEmpty())
		cause = source + TEXT(" ") + cause;

	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnPlayerDeath source: {0}, cause: {1}", source, cause);

	AddDeathLinkMessageToChat(source, cause);

	ap->TriggerDeathLink(source, cause);

	MassMurder();
}

FString AApPlayerSubsystem::GetDamageSuffix(const UDamageType* damageType, AActor* self, AActor* causee)
{
	FString cause = TEXT("");
	FString damageName = damageType->GetName();

	if (damageName.EndsWith(TEXT("WorldBounds_C")))
		cause = TEXT("flew out of the map");
	if (damageName.EndsWith(TEXT("Radiation_C")))
		cause = TEXT("died of radiation poisoning");
	if (damageName.EndsWith(TEXT("Gas_C")))
		cause = TEXT("suffocated in toxic gas");
	if (damageName.EndsWith(TEXT("_Fall_C")))
		cause = TEXT("fell to their death");
	else
	{
		bool isTeamKill = false;
		bool selfHarm = false;
		FString causer = TEXT("");

		if (causee->IsA<AFGCharacterPlayer>())
		{
			AFGCharacterPlayer* causeePlayer = Cast<AFGCharacterPlayer>(causee);

			if (IsValid(causeePlayer))
			{
				if (self != causeePlayer)
					isTeamKill = true;
				else
					selfHarm = true;

				APlayerState* causeePlayerState = causeePlayer->GetPlayerState();
				if (!IsValid(causeePlayerState))
					causer = causeePlayerState->GetPlayerName();
			}
		}
		else if (causee->IsA<AFGCreature>())
		{
			FString causeeName = causee->GetName();

			if (causeeName.Contains("Hog") || causeeName.Contains("Johnny"))
				causer = "A Hog";
			if (causeeName.Contains("Spitter"))
				causer = "A Spitter";
			if (causeeName.Contains("Stinger"))
				causer = "A Spider";
			if (causeeName.Contains("Crab") || causeeName.Contains("Hatcher"))
				causer = "A Bee";
		}

		if (damageName.EndsWith(TEXT("Explosive_C")))
		{
			if (isTeamKill)
				cause = FString::Format(TEXT("was blown up by teammate {0}"), { causer });
			else if (selfHarm)
				cause = TEXT("blew themself up");
			else if (!causer.IsEmpty())
				cause = FString::Format(TEXT("was blown up by {0}"), { causer });
			else
				cause = TEXT("blew up");
		}
		if (damageName.EndsWith(TEXT("Physical_C")))
		{
			if (isTeamKill)
				cause = FString::Format(TEXT("was teamkilled by {0}"), { causer });
			else if (selfHarm)
				cause = TEXT("killed themself");
			else if (!causer.IsEmpty())
				cause = FString::Format(TEXT("was killed by {0}"), { causer });
			else
				cause = TEXT("was killed");
		}
	}

	return cause;
}

void AApPlayerSubsystem::OnDeathLinkReceived(FString source, FString cause) {
	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnDeathLinkReceived({0}, {1})", source, cause);

	if (!HasAuthority())
		return;

	AddDeathLinkMessageToChat(source, cause);

	MassMurder();
}

void AApPlayerSubsystem::AddDeathLinkMessageToChat(const FString& source, const FString& cause) const
{
	FText sourceText = FText::FromString(source);
	FText causeText = FText::FromString(cause);
	FText message;
	if (cause.IsEmpty())
		message = FText::Format(LOCTEXT("DeathLinkReceived", "{0} has died, and so have you!"), sourceText);
	else
		message = FText::Format(LOCTEXT("DeathLinkReceivedWithCause", "{0} has died because {1}"), sourceText, causeText);

	ap->AddChatMessage(message, FLinearColor::Red);
}

void AApPlayerSubsystem::MassMurder()
{
	canTriggerDeathlinks = false;

	for (TActorIterator<AFGCharacterPlayer> actorItterator(GetWorld()); actorItterator; ++actorItterator) {
		AFGCharacterPlayer* player = *actorItterator;
		if (!IsValid(player))
			continue;

		bool isOnline = IsOnlinePlayer(player);
		if (!isOnline)
			continue;

		UFGHealthComponent* health = player->GetHealthComponent();
		if (!IsValid(health))
			continue;

		health->Kill();
	}

	canTriggerDeathlinks = true;
}

bool AApPlayerSubsystem::IsOnlinePlayer(AFGCharacterPlayer* player)
{
	if (!IsValid(player))
		return false;

	return player->IsPlayerOnline();
}

#pragma optimize("", on)

#undef LOCTEXT_NAMESPACE
