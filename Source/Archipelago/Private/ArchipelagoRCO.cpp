#include "ArchipelagoRCO.h"

#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogApReplication);

void UArchipelagoRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UArchipelagoRCO, Dummy);

}

void UArchipelagoRCO::ServerSetPortalTargetPlayer_Implementation(AApPortal* Building, FApPlayer Player) {
	if (!Building)
		return;
	UE_LOG(LogApReplication, Display, TEXT("RCO Server Set PortalTarget %s to %s"), *UKismetSystemLibrary::GetDisplayName(Building), *Player.Name);
	Building->targetPlayer = Player;
}
