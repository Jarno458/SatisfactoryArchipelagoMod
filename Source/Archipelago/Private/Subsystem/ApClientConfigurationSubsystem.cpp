#include "ApClientConfigurationSubsystem.h"

#include "Subsystem/SubsystemActorManager.h"
#include "Configuration/ModConfiguration.h"
#include "Configuration/Properties/ConfigPropertyInteger.h"
#include "Configuration/Properties/ConfigPropertyBool.h"
#include "Configuration/ConfigManager.h"
#include "Configuration/Properties/ConfigPropertySection.h"

DEFINE_LOG_CATEGORY(LogApClientConfigurationSubsystem);

//TODO REMOVE
#pragma optimize("", off)

AApClientConfigurationSubsystem* AApClientConfigurationSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApClientConfigurationSubsystem>();
}

AApClientConfigurationSubsystem::AApClientConfigurationSubsystem() : Super() {
	UE_LOG(LogApClientConfigurationSubsystem, Display, TEXT("AApClientConfigurationSubsystem::AApClientConfigurationSubsystem()"));

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnLocal;

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AApClientConfigurationSubsystem::BeginPlay() {
	Super::BeginPlay();

	SetMamEnhancerConfigurationHooks();

	UE_LOG(LogApClientConfigurationSubsystem, Display, TEXT("AApClientConfigurationSubsystem::BeginPlay()"));
}

void AApClientConfigurationSubsystem::SetMamEnhancerConfigurationHooks() {
	FConfigId MamEnhancerConfigId{ "MAMTips", "" };

	UConfigManager* ConfigManager = GetWorld()->GetGameInstance()->GetSubsystem<UConfigManager>();
	if (ConfigManager == nullptr) {
		return;
	}

	UConfigPropertySection* configRoot = ConfigManager->GetConfigurationRootSection(MamEnhancerConfigId);
	if (configRoot == nullptr) {
		return;
	}

	if (configRoot->SectionProperties.Contains("ShowHiddenNodesDetails")) {
		UConfigPropertyBool* showHidenNodeDetails = Cast<UConfigPropertyBool>(configRoot->SectionProperties["ShowHiddenNodesDetails"]);
		if (showHidenNodeDetails != nullptr)
			showHidenNodeDetails->OnPropertyValueChanged.AddDynamic(this, &AApClientConfigurationSubsystem::LockMamEnhancerSpoilerConfiguration);
	}

	if (configRoot->SectionProperties.Contains("MakeHiddenPrettyMode")) {
		UConfigPropertyInteger* hidenDisplayMode = Cast<UConfigPropertyInteger>(configRoot->SectionProperties["MakeHiddenPrettyMode"]);
		if (hidenDisplayMode != nullptr)
			hidenDisplayMode->OnPropertyValueChanged.AddDynamic(this, &AApClientConfigurationSubsystem::LockMamEnhancerSpoilerConfiguration);
	}

	LockMamEnhancerSpoilerConfiguration();
}

void AApClientConfigurationSubsystem::LockMamEnhancerSpoilerConfiguration() {
	FConfigId MamEnhancerConfigId{ "MAMTips", "" };

	UConfigManager* ConfigManager = GetWorld()->GetGameInstance()->GetSubsystem<UConfigManager>();
	if (ConfigManager == nullptr) {
		return;
	}

	UConfigPropertySection* configRoot = ConfigManager->GetConfigurationRootSection(MamEnhancerConfigId);
	if (configRoot == nullptr) {
		return;
	}

	bool dirty = false;

	if (configRoot->SectionProperties.Contains("ShowHiddenNodesDetails")) {
		UConfigPropertyBool* showHidenNodeDetails = Cast<UConfigPropertyBool>(configRoot->SectionProperties["ShowHiddenNodesDetails"]);
		if (showHidenNodeDetails != nullptr) {
			if (showHidenNodeDetails->Value) {
				showHidenNodeDetails->Value = false;

				dirty = true;
			}
		}
	}

	// 1 = Empty Gray Boxes (Base Game)
	// 2 = Show Question Mark Icons
	// 5 = Show "Who's That Jace?" Icons (Silly)
	if (configRoot->SectionProperties.Contains("MakeHiddenPrettyMode")) {
		UConfigPropertyInteger* hidenDisplayMode = Cast<UConfigPropertyInteger>(configRoot->SectionProperties["MakeHiddenPrettyMode"]);
		if (hidenDisplayMode != nullptr) {
			if (hidenDisplayMode->Value != 1 && hidenDisplayMode->Value != 2 && hidenDisplayMode->Value != 5) {
				hidenDisplayMode->Value = 1;

				dirty = true;
			}
		}
	}

	if (dirty) {
		ConfigManager->MarkConfigurationDirty(MamEnhancerConfigId);
		ConfigManager->FlushPendingSaves();
		ConfigManager->ReloadModConfigurations();
	}
}

#pragma optimize("", on)