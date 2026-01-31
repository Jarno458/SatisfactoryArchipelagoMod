#include "ApPlayerSubsystem.h"

#include "EngineUtils.h"
#include "Subsystem/SubsystemActorManager.h"
#include "AI/FGCreatureController.h"

#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(LogApPlayerSubsystem);

#define LOCTEXT_NAMESPACE "Archipelago"

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

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AApPlayerSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApPlayerSubsystem, Display, TEXT("AApPlayerSubsystem::BeginPlay()"));

	UWorld* world = GetWorld();
	fgcheck(world != nullptr);

	ap = AApSubsystem::Get(world);
	fgcheck(ap);
	slotData = AApSlotDataSubsystem::Get(world);
	fgcheck(slotData);
}

void AApPlayerSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (!slotData->HasLoadedSlotData())
		return;

	if (slotData->DeathLink)
	{
		ap->SetDeathLinkReceivedCallback([this](FString source, FString cause) { OnDeathLinkReceived(source, cause); });

		canTriggerDeathlinks = true;
	}

	SetActorTickEnabled(false);

	isInitialized = true;
}

void AApPlayerSubsystem::OnPlayerDeath(AActor* deadActor, const UDamageType* damageType, AActor* instigatedBy, AActor* damageCauser) {
	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnPlayerDeath({0})", *deadActor->GetName());

	if (!isInitialized || !canTriggerDeathlinks || !HasAuthority())
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

	FString cause = GetDamageSuffix(damageType, deadActor, instigatedBy, damageCauser);

	if (!cause.IsEmpty())
		cause = source + TEXT(" ") + cause;

	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnPlayerDeath source: {0}, cause: {1}", source, cause);

	AddDeathLinkMessageToChat(source, cause);

	ap->TriggerDeathLink(source, cause);

	MassMurder();
}

FString AApPlayerSubsystem::GetDamageSuffix(const UDamageType* damageType, AActor* self, AActor* instigatedBy, AActor* damageCauser)
{
	FString cause = TEXT("");
	FString damageName = TEXT("");

	bool isTeamKill = false;
	bool selfHarm = false;
	FString causer = TEXT("");

	if (IsValid(instigatedBy))
	{
		if (instigatedBy->IsA<AFGPlayerController>())
		{
			AFGPlayerController* causeeController = Cast<AFGPlayerController>(instigatedBy);

			if (IsValid(causeeController))
			{
				AFGCharacterPlayer* causeePlayer = Cast<AFGCharacterPlayer>(causeeController->GetCharacter());
				if (IsValid(causeePlayer))
				{
					if (self != causeePlayer)
						isTeamKill = true;
					else
						selfHarm = true;

					AFGPlayerState* causeePlayerState = causeeController->GetPlayerState<AFGPlayerState>();
					if (IsValid(causeePlayerState))
						causer = causeePlayerState->GetPlayerName();
				}
			}
		}
		else if (instigatedBy->IsA<AFGCreatureController>())
		{
			FString causeeName = instigatedBy->GetName();

			if (causeeName.Contains("Hog") || causeeName.Contains("Johnny"))
				causer = "A Hog";
			if (causeeName.Contains("Spitter"))
				causer = "A Spitter";
			if (causeeName.Contains("Stinger"))
				causer = "A Spider";
			if (causeeName.Contains("Crab") || causeeName.Contains("Hatcher"))
				causer = "A Bee";
		}
	} else if (IsValid(damageCauser)) {
		FString causeeName = damageCauser->GetName();

		if (causeeName.Contains("Hog") || causeeName.Contains("Johnny"))
			causer = "A Hog";
		if (causeeName.Contains("Spitter"))
			causer = "A Spitter";
		if (causeeName.Contains("Stinger"))
			causer = "A Spider";
		if (causeeName.Contains("Crab") || causeeName.Contains("Hatcher"))
			causer = "A Bee";
	}

	if (IsValid(damageType))
		damageName = damageType->GetName();

	if (damageName.EndsWith(TEXT("WorldBounds_C")))
		cause = TEXT("flew out of the map");
	else if (damageName.EndsWith(TEXT("Radiation_C")))
		cause = TEXT("died of radiation poisoning");
	else if (damageName.EndsWith(TEXT("Gas_C")))
		cause = TEXT("suffocated in toxic gas");
	else if (damageName.EndsWith(TEXT("_Fall_C")))
		cause = TEXT("fell to their death");
	else if (damageName.EndsWith(TEXT("Explosive_C")))
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
	else
	{
		if (isTeamKill)
		{
			if (causer.IsEmpty())
				cause = TEXT("was teamkilled");
			else
				cause = FString::Format(TEXT("was teamkilled by {0}"), { causer });
		}
		else if (selfHarm)
			cause = TEXT("killed themself");
		else if (!causer.IsEmpty())
			cause = FString::Format(TEXT("was killed by {0}"), { causer });
		else
			cause = TEXT("was killed");
	}

	return cause;
}

void AApPlayerSubsystem::OnDeathLinkReceived(FString source, FString cause) {
	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnDeathLinkReceived({0}, {1})", source, cause);

	if (!isInitialized || !canTriggerDeathlinks || !HasAuthority())
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

#undef LOCTEXT_NAMESPACE
