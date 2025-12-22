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

void AApPlayerSubsystem::OnPlayerBeginPlay(AFGCharacterPlayer* player) {
	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnPlayerBeginPlay() player begin play");

	if (!HasAuthority())
		return;

	UFGHealthComponent* health = player->GetHealthComponent();

	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnPlayerEndPlay Health {0}", health->GetFullName());

	health->DeathDelegate.AddDynamic(this, &AApPlayerSubsystem::OnPlayerDeath);
	health->OnTakeAnyDamageDelegate.AddDynamic(this, &AApPlayerSubsystem::OnPlayerTakeDamage);

}

void AApPlayerSubsystem::OnPlayerEndPlay(AFGCharacterPlayer* player) {
	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnPlayerEndPlay player left play");

	if (!HasAuthority())
		return;

	UFGHealthComponent* health = player->GetHealthComponent();

	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnPlayerEndPlay Health {0}", health->GetFullName());

	health->DeathDelegate.RemoveDynamic(this, &AApPlayerSubsystem::OnPlayerDeath);
	health->OnTakeAnyDamageDelegate.RemoveDynamic(this, &AApPlayerSubsystem::OnPlayerTakeDamage);
}

void AApPlayerSubsystem::OnPlayerDeath(AActor* deadActor) {
	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnPlayerDeath({0})", *deadActor->GetName());

	if (!HasAuthority() || !canTriggerDeathlinks)
		return;

	ap->TriggerDeathLink();

	MassMurder();
}

void AApPlayerSubsystem::OnPlayerTakeDamage(AActor* damagedActor, float damageAmount, const UDamageType* damageType, AController* instigatedBy, AActor* damageCauser) {
	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnPlayerTakeDamage({0}, {1})", *damagedActor->GetName(), damageAmount);

	if (!HasAuthority() || !canTriggerDeathlinks)
		return;
}

void AApPlayerSubsystem::OnDeathLinkReceived(FText message) {
	UE_LOGFMT(LogApPlayerSubsystem, Display, "AApPlayerSubsystem::OnDeathLinkReceived({0})", message.ToString());

	if (!HasAuthority())
		return;

	MassMurder();
	
	ap->AddChatMessage(message, FLinearColor::Red);
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
