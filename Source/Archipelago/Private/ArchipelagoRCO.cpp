#include "ArchipelagoRCO.h"
#include "Logging/StructuredLog.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogApReplication);

void UArchipelagoRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UArchipelagoRCO, Dummy);
}

void UArchipelagoRCO::ServerSetPortalTargetPlayer_Implementation(AApPortal* Building, FApPlayer Player) {
	if (!Building)
		return;

	UE_LOGFMT(LogApReplication, Display, "RCO Server Set PortalTarget {0}", Player.Name);

	Building->targetPlayer = Player;
}
