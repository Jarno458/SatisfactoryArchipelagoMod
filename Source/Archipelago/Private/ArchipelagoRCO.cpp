#include "ArchipelagoRCO.h"
#include "Logging/StructuredLog.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogApReplication);

void UArchipelagoRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UArchipelagoRCO, Dummy);
}

void UArchipelagoRCO::ServerSetPortalOutputFilter_Implementation(AApPortal* Building, const TArray<TSubclassOf<UFGItemDescriptor>>& outputFilter)
{
	if (!Building)
		return;

	UE_LOGFMT(LogApReplication, Display, "RCO Server Set PortalOutputFilter outputFilter[{0}]", outputFilter.Num());

	Building->ServerSetAllowedOutput(outputFilter);
}

void UArchipelagoRCO::ServerSetPortalTargetPlayer_Implementation(AApPortal* Building, FApPlayer Player) {
	if (!Building)
		return;

	UE_LOGFMT(LogApReplication, Display, "RCO Server Set PortalTarget team: {0}, slot: {1}", Player.Team, Player.Slot);

	Building->ServerSetTarget(Player);
}

void UArchipelagoRCO::ServerSendPortalGifts_Implementation(AApPortal* Building) {
	if (!Building)
		return;

	UE_LOGFMT(LogApReplication, Display, "RCO Server Send Portal Gifts");

	Building->ServerSendManually();
}