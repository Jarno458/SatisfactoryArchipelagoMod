#include "Subsystem/ApSubsystem.h"
#include "ApUtils.h"

DEFINE_LOG_CATEGORY(LogApSubsystem);

DEFINE_LOG_CATEGORY(LogApChat);

//TODO REMOVE
#pragma optimize("", off)

#define LOCTEXT_NAMESPACE "Archipelago"

std::map<std::string, std::function<void(AP_SetReply)>> AApSubsystem::callbacks;

TMap<int64_t, FString> AApSubsystem::ItemIdToGameItemDescriptor = {
	//Parts
	{1338000, TEXT("Desc_SpaceElevatorPart_5_C")}, //Adaptive Control Unit
	{1338001, TEXT("Desc_CircuitBoardHighSpeed_C")}, //AI Limiter
	{1338002, TEXT("Desc_AluminumPlate_C")},
	{1338003, TEXT("Desc_Crystal_C")}, //Blue Power Slug
	{1338004, TEXT("Desc_Crystal_mk2_C")}, //Yellow Power Slug
	{1338005, TEXT("Desc_AlienProtein_C")},
	{1338006, TEXT("Desc_Crystal_mk3_C")}, //Purple Power Slug
	{1338007, TEXT("Desc_AluminumCasing_C")},
	{1338008, TEXT("Desc_AluminumIngot_C")},
	{1338009, TEXT("Desc_AluminumScrap_C")},
	{1338010, TEXT("Desc_SpaceElevatorPart_7_C")}, //Assembly Director System
	{1338011, TEXT("Desc_SpaceElevatorPart_3_C")}, //Automated Wiring
	{1338012, TEXT("Desc_Battery_C")},
	{1338013, TEXT("Desc_OreBauxite_C")},
	{1338014, TEXT("BP_EquipmentDescriptorBeacon_C")}, //Beacon
	{1338015, TEXT("Desc_GenericBiomass_C")},
	{1338016, TEXT("Desc_Gunpowder_C")}, //Black Powder
	{1338017, TEXT("Desc_Cable_C")},
	{1338018, TEXT("Desc_GoldIngot_C")}, //Caterium Ingot
	{1338019, TEXT("Desc_OreGold_C")}, //Caterium Ore
	{1338020, TEXT("Desc_CircuitBoard_C")},
	{1338021, TEXT("Desc_Coal_C")},
	{1338022, TEXT("Desc_ColorCartridge_C")},
	{1338023, TEXT("Desc_CompactedCoal_C")},
	{1338024, TEXT("Desc_Computer_C")},
	{1338025, TEXT("Desc_Cement_C")}, //Concrete
	{1338026, TEXT("Desc_CoolingSystem_C")},
	{1338027, TEXT("Desc_CopperIngot_C")},
	{1338028, TEXT("Desc_OreCopper_C")}, //Copper Ore
	{1338029, TEXT("Desc_CopperDust_C")},
	{1338030, TEXT("Desc_CopperSheet_C")},
	{1338031, TEXT("Desc_CharacterRunStatue_C")},
	{1338032, TEXT("Desc_CrystalOscillator_C")},
	{1338033, TEXT("Desc_ElectromagneticControlRod_C")},
	{1338034, TEXT("Desc_FluidCanister_C")},
	{1338035, TEXT("Desc_GasTank_C")}, //Empty Fluid Tank
	{1338036, TEXT("Desc_SteelPlateReinforced_C")}, //Encased Industrial Beam
	{1338037, TEXT("Desc_PlutoniumCell_C")},
	{1338038, TEXT("Desc_UraniumCell_C")},
	{1338039, TEXT("Desc_Fabric_C")},
	{1338040, TEXT("Desc_ResourceSinkCoupon_C")},
	{1338041, TEXT("Desc_FlowerPetals_C")},
	{1338042, TEXT("Desc_ModularFrameFused_C")},
	{1338043, TEXT("Desc_HardDrive_C")},
	{1338044, TEXT("Desc_AluminumPlateReinforced_C")}, //Heatsink
	{1338045, TEXT("Desc_ModularFrameHeavy_C")},
	{1338046, TEXT("Desc_HighSpeedConnector_C")},
	{1338047, TEXT("Desc_CharacterSpin_Statue_C")},
	{1338048, TEXT("Desc_CharacterClap_Statue_C")},
	{1338049, TEXT("Desc_IronIngot_C")},
	{1338050, TEXT("Desc_OreIron_C")},
	{1338051, TEXT("Desc_IronPlate_C")},
	{1338052, TEXT("Desc_IronRod_C")},
	{1338053, TEXT("Desc_GoldenNut_Statue_C")},
	{1338054, TEXT("Desc_Leaves_C")},
	{1338055, TEXT("Desc_Stone_C")}, //Limestone
	{1338056, TEXT("Desc_SpaceElevatorPart_6_C")}, //Magnetic Field Generator
	{1338057, TEXT("Desc_WAT2_C")}, //Mercer Sphere
	{1338058, TEXT("Desc_SpaceElevatorPart_4_C")}, //Modular Engine
	{1338059, TEXT("Desc_ModularFrame_C")},
	{1338060, TEXT("Desc_Motor_C")},
	{1338061, TEXT("Desc_Mycelia_C")},
	{1338062, TEXT("Desc_NonFissibleUranium_C")},
	{1338063, TEXT("Desc_SpaceElevatorPart_9_C")}, //Nuclear Pasta
	{1338064, TEXT("Desc_DoggoStatue_C")},
	{1338065, TEXT("Desc_AlienDNACapsule_C")},
	{1338066, TEXT("Desc_PackagedAlumina_C")},
	{1338067, TEXT("Desc_Fuel_C")},
	{1338068, TEXT("Desc_PackagedOilResidue_C")},
	{1338069, TEXT("Desc_PackagedBiofuel_C")},
	{1338070, TEXT("Desc_PackagedNitricAcid_C")},
	{1338071, TEXT("Desc_PackagedNitrogenGas_C")},
	{1338072, TEXT("Desc_PackagedOil_C")},
	{1338073, TEXT("Desc_PackagedSulfuricAcid_C")},
	{1338074, TEXT("Desc_TurboFuel_C")}, //Packaged Turno Fuel
	{1338075, TEXT("Desc_PackagedWater_C")},
	{1338076, TEXT("Desc_PetroleumCoke_C")},
	{1338077, TEXT("Desc_Plastic_C")},
	{1338078, TEXT("Desc_PlutoniumFuelRod_C")},
	{1338079, TEXT("Desc_PlutoniumPellet_C")},
	{1338080, TEXT("Desc_PlutoniumWaste_C")},
	{1338081, TEXT("Desc_PolymerResin_C")},
	{1338082, TEXT("Desc_CrystalShard_C")}, //Power Shard
	{1338083, TEXT("Desc_SpaceGiraffeStatue_C")},
	{1338084, TEXT("Desc_PressureConversionCube_C")},
	{1338085, TEXT("Desc_ComputerQuantum_C")},
	{1338086, TEXT("Desc_QuartzCrystal_C")},
	{1338087, TEXT("Desc_HighSpeedWire_C")},
	{1338088, TEXT("Desc_ModularFrameLightweight_C")},
	{1338089, TEXT("Desc_RawQuartz_C")},
	{1338090, TEXT("Desc_IronPlateReinforced_C")},
	{1338091, TEXT("Desc_Rotor_C")},
	{1338092, TEXT("Desc_Rubber_C")},
	{1338093, TEXT("Desc_SAM_C")},
	{1338094, TEXT("Desc_IronScrew_C")},
	{1338095, TEXT("Desc_Silica_C")},
	{1338096, TEXT("Desc_SpaceElevatorPart_1_C")}, //Smart Plating
	{1338097, TEXT("Desc_GunpowderMK2_C")}, //Smokeless Powder
	{1338098, TEXT("Desc_Biofuel_C")}, //Solid Biofuel
	{1338099, TEXT("Desc_WAT1_C")}, //Somersloop
	{1338100, TEXT("Desc_Stator_C")},
	{1338101, TEXT("Desc_Hog_Statue_C")},
	{1338102, TEXT("Desc_SteelPlate_C")},
	{1338103, TEXT("Desc_SteelIngot_C")},
	{1338104, TEXT("Desc_SteelPipe_C")},
	{1338105, TEXT("Desc_Sulfur_C")},
	{1338106, TEXT("Desc_ComputerSuper_C")},
	{1338107, TEXT("Desc_QuantumOscillator_C")},
	{1338108, TEXT("Desc_SpaceElevatorPart_8_C")}, //Thermal Propulsion Rocket
	{1338109, TEXT("Desc_MotorLightweight_C")},
	{1338110, TEXT("Desc_HogParts_C'")},
	{1338111, TEXT("Desc_OreUranium_C")},
	{1338112, TEXT("Desc_NuclearFuelRod_C")},
	{1338113, TEXT("Desc_NuclearWaste_C")},
	{1338114, TEXT("Desc_SpaceElevatorPart_2_C")}, //Versatile Framework
	{1338115, TEXT("Desc_Wire_C")},
	{1338116, TEXT("Desc_Wood_C")},
	{1338117, TEXT("Desc_SpitterParts_C")},
	{1338118, TEXT("Desc_StingerParts_C")},
	{1338119, TEXT("Desc_HatcherParts_C")},

	//Enquipment/Ammo
	{1338150, TEXT("Desc_Shroom_C")},
	{1338151, TEXT("Desc_Nut_C")},
	{1338152, TEXT("BP_EquipmentDescriptorJumpingStilts_C")},
	//{1338153, TEXT("")}, //BoomBox
	{1338154, TEXT("Desc_Chainsaw_C")},
	{1338155, TEXT("Desc_NobeliskCluster_C")},
	//{1338156, TEXT("Unused")},
	{1338157, TEXT("BP_EquipmentDescriptorCup")},
	{1338158, TEXT("BP_EquipmentDescriptorCupGold")},
	{1338159, TEXT("Desc_Rebar_Explosive_C")},
	{1338160, TEXT("Desc_GolfCart_C")},
	{1338161, TEXT("Desc_GolfCartGold")},
	{1338162, TEXT("BP_EquipmentDescriptorGasmask_C")},
	{1338163, TEXT("Desc_NobeliskGas_C")},
	{1338164, TEXT("BP_EquipmentDescriptorHazmatSuit_C")},
	{1338165, TEXT("Desc_CartridgeSmartProjectile_C")},
	{1338166, TEXT("BP_EquipmentDescriptorHoverPack_C")},
	{1338167, TEXT("Desc_SpikedRebar_C")},
	{1338168, TEXT("BP_EquipmentDescriptorJetPack_C")},
	{1338169, TEXT("Desc_Medkit_C")},
	{1338170, TEXT("Desc_NobeliskExplosive_C")},
	{1338171, TEXT("BP_EquipmentDescriptorNobeliskDetonator_C")},
	{1338172, TEXT("Desc_NobeliskNuke_C")},
	{1338173, TEXT("BP_EquipmentDescriptorObjectScanner_C")},
	{1338174, TEXT("Desc_Berry_C")},
	{1338175, TEXT("Desc_Parachute_C")},
	{1338176, TEXT("Desc_NobeliskShockwave_C")},
	{1338177, TEXT("Desc_RebarGunProjectile_C")},
	{1338178, TEXT("BP_EquipmentDescriptorRifle_C")},
	{1338179, TEXT("Desc_CartridgeStandard_C")},
	{1338180, TEXT("Desc_Rebar_Spreadshot_C")},
	{1338181, TEXT("Desc_Rebar_Stunshot_C")},
	{1338182, TEXT("Desc_CartridgeChaos_C")},
	{1338183, TEXT("BP_EquipmentDescriptorStunSpear_C")},
	{1338184, TEXT("BP_EquipmentDescriptorShockShank_C")},
	{1338185, TEXT("BP_EqDescZipLine_C")},
	{1338186, TEXT("BP_ItemDescriptorPortableMiner_C")}
};


TMap<int64_t, FString> AApSubsystem::ItemIdToGameRecipe = {
	{1338200, TEXT("Recipe_IronPlateReinforced")},
	{1338201, TEXT("Recipe_Alternate_AdheredIronPlate")},
	{1338202, TEXT("Recipe_Alternate_ReinforcedIronPlate_1")},
	{1338203, TEXT("Recipe_Alternate_ReinforcedIronPlate_2")},
	{1338204, TEXT("Recipe_Rotor")},
	{1338205, TEXT("Recipe_Alternate_CopperRotor")},
	{1338206, TEXT("Recipe_Alternate_Rotor")},
	{1338207, TEXT("Recipe_Stator")},
	{1338208, TEXT("Recipe_Alternate_Stator")},
	{1338209, TEXT("Recipe_Plastic")},
	{1338210, TEXT("Recipe_ResidualPlastic")},
	{1338211, TEXT("Recipe_Alternate_Plastic_1")},
	{1338212, TEXT("Recipe_Rubber")},
	{1338213, TEXT("Recipe_ResidualRubber")},
	{1338214, TEXT("Recipe_Alternate_RecycledRubber")},
	{1338215, TEXT("Recipe_IronPlate")},
	{1338216, TEXT("Recipe_Alternate_CoatedIronPlate")},
	{1338217, TEXT("Recipe_Alternate_SteelCoatedPlate")},
	{1338218, TEXT("Recipe_IronRod")},
	{1338219, TEXT("Recipe_Alternate_SteelRod")},
	{1338220, TEXT("Recipe_Screw")},
	{1338221, TEXT("Recipe_Alternate_Screw")},
	{1338222, TEXT("Recipe_Alternate_Screw_2")},
	{1338223, TEXT("Recipe_Wire")},
	{1338224, TEXT("Recipe_Alternate_FusedWire")},
	{1338225, TEXT("Recipe_Alternate_Wire_1")}, 
	{1338226, TEXT("Recipe_Alternate_Wire_2")},
	{1338227, TEXT("Recipe_Cable")},
	{1338228, TEXT("Recipe_Alternate_CoatedCable")},
	{1338229, TEXT("Recipe_Alternate_Cable_1")},
	{1338230, TEXT("Recipe_Alternate_Cable_2")},
	{1338231, TEXT("Recipe_Quickwire")},
	{1338232, TEXT("Recipe_Alternate_Quickwire")},
	{1338233, TEXT("Recipe_CopperSheet")},
	{1338234, TEXT("Recipe_Alternate_SteamedCopperSheet")},
	{1338235, TEXT("Recipe_SteelPipe")},
	{1338236, TEXT("Recipe_SteelBeam")},
	//{1338237, TEXT("")}, //Crude Oil
	{1338238, TEXT("Recipe_Alternate_HeavyOilResidue")},
	{1338239, TEXT("Recipe_Alternate_PolymerResin")},
	{1338240, TEXT("Recipe_LiquidFuel")},
	{1338241, TEXT("Recipe_ResidualFuel")},
	{1338242, TEXT("Recipe_Alternate_DilutedPackagedFuel")},
	//{1338243, TEXT("")}, //Water
	{1338244, TEXT("Recipe_Concrete")},
	{1338245, TEXT("Recipe_Alternate_RubberConcrete")},
	{1338246, TEXT("Recipe_Alternate_WetConcrete")},
	{1338247, TEXT("Recipe_Alternate_Concrete")},
	{1338248, TEXT("Recipe_Silica")},
	{1338249, TEXT("Recipe_Alternate_Silica")},
	{1338250, TEXT("Recipe_QuartzCrystal")},
	{1338251, TEXT("Recipe_Alternate_PureQuartzCrystal")},
	{1338252, TEXT("Recipe_IngotIron")},
	{1338253, TEXT("Recipe_Alternate_PureIronIngot")},
	{1338254, TEXT("Recipe_Alternate_IngotIron")},
	{1338255, TEXT("Recipe_IngotSteel")},
	{1338256, TEXT("Recipe_Alternate_CokeSteelIngot")},
	{1338257, TEXT("Recipe_Alternate_IngotSteel_2")},
	{1338258, TEXT("Recipe_Alternate_IngotSteel_1")},
	{1338259, TEXT("Recipe_IngotCopper")},
	{1338260, TEXT("Recipe_Alternate_CopperAlloyIngot")},
	{1338261, TEXT("Recipe_Alternate_PureCopperIngot")},
	{1338262, TEXT("Recipe_IngotCaterium")},
	{1338263, TEXT("Recipe_Alternate_PureCateriumIngot")},
	//{1338264, TEXT("")}, //Limestone
	//{1338265, TEXT("")}, //Raw Quartz
	//{1338266, TEXT("")}, //Iron Ore
	//{1338267, TEXT("")}, //Copper Ore
	//{1338268, TEXT("")}, //Coal
	//{1338269, TEXT("")}, //Sulfur
	//{1338270, TEXT("")}, //Caterium Ore
	{1338271, TEXT("Recipe_PetroleumCoke")},
	{1338272, TEXT("Recipe_Alternate_EnrichedCoal")},
	{1338273, TEXT("Recipe_Motor")},
	{1338274, TEXT("Recipe_Alternate_Motor_1")},
	{1338275, TEXT("Recipe_Alternate_ElectricMotor")},
	{1338276, TEXT("Recipe_ModularFrame")},
	{1338277, TEXT("Recipe_Alternate_BoltedFrame")},
	{1338278, TEXT("Recipe_Alternate_ModularFrame")},
	{1338279, TEXT("Recipe_ModularFrameHeavy")},
	{1338280, TEXT("Recipe_Alternate_FlexibleFramework")},
	{1338281, TEXT("Recipe_Alternate_ModularFrameHeavy")},
	{1338282, TEXT("Recipe_EncasedIndustrialBeam")},
	{1338283, TEXT("Recipe_Alternate_EncasedIndustrialBeam")},
	{1338284, TEXT("Recipe_Computer")},
	{1338285, TEXT("Recipe_Alternate_Computer_2")},
	{1338286, TEXT("Recipe_Alternate_Computer_1")},
	{1338287, TEXT("Recipe_CircuitBoard")},
	{1338288, TEXT("Recipe_Alternate_ElectrodeCircuitBoard")},
	{1338289, TEXT("Recipe_Alternate_CircuitBoard_1")},
	{1338290, TEXT("Recipe_Alternate_CircuitBoard_2")},
	{1338291, TEXT("Recipe_CrystalOscillator")},
	{1338292, TEXT("Recipe_Alternate_CrystalOscillator")},
	{1338293, TEXT("Recipe_AILimiter")},
	{1338294, TEXT("Recipe_ElectromagneticControlRod")},
	{1338295, TEXT("Recipe_Alternate_ElectromagneticControlRod_1")},
	{1338296, TEXT("Recipe_HighSpeedConnector")},
	{1338297, TEXT("Recipe_Alternate_HighSpeedConnector")},
	{1338298, TEXT("Recipe_SpaceElevatorPart_1")},
	{1338299, TEXT("Recipe_Alternate_PlasticSmartPlating")},
	{1338300, TEXT("Recipe_SpaceElevatorPart_2")},
	{1338301, TEXT("Recipe_Alternate_FlexibleFramework")},
	{1338302, TEXT("Recipe_SpaceElevatorPart_3")},
	{1338303, TEXT("Recipe_Alternate_HighSpeedWiring")},
	{1338304, TEXT("Recipe_SpaceElevatorPart_4")},
	{1338305, TEXT("Recipe_SpaceElevatorPart_5")},
	{1338306, TEXT("Recipe_Alternate_DilutedFuel")},
	{1338307, TEXT("Recipe_AluminaSolution")},
	{1338308, TEXT("Recipe_Alternate_AutomatedMiner")},
};

TMap<int64_t, FString> AApSubsystem::ItemIdToGameBuilding = {
	{1338400, TEXT("Recipe_ConstructorMk1")},
	{1338401, TEXT("Recipe_AssemblerMk1")},
	{1338402, TEXT("Recipe_ManufacturerMk1")},
	{1338403, TEXT("Recipe_Packager")},
	{1338404, TEXT("Recipe_OilRefinery")},
	{1338405, TEXT("Recipe_Blender")},
	{1338406, TEXT("Recipe_HadronCollider")},
	{1338407, TEXT("Recipe_GeneratorBiomass")},
	{1338408, TEXT("Recipe_GeneratorCoal")},
	{1338409, TEXT("Recipe_GeneratorGeoThermal")},
	{1338410, TEXT("Recipe_GeneratorNuclear")},
	{1338411, TEXT("Recipe_MinerMk1")},
	{1338412, TEXT("Recipe_MinerMk2")},
	{1338413, TEXT("Recipe_MinerMk3")},
	{1338414, TEXT("Recipe_OilPump")},
	{1338415, TEXT("Recipe_WaterPump")},
	{1338416, TEXT("Recipe_SmelterBasicMk1")},
	{1338417, TEXT("Recipe_SmelterMk1")},
	{1338499, TEXT("Recipe_SpaceElevator")},
};

TMap<int64_t, FString> AApSubsystem::ItemIdToGameName2 = {
};

AApSubsystem::AApSubsystem() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.5f;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer; // TODO_MULTIPLAYER is this what we want long term?

	ConnectionState = NotYetAttempted;
	ConnectionStateDescription = LOCTEXT("NotYetAttempted", "A connection has not yet been attempted. Load a save file to attempt to connect.");
}

AApSubsystem* AApSubsystem::Get() {
	return Get(GEngine->GameViewport->GetWorld());
}

AApSubsystem* AApSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	check(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApSubsystem>();
}

void AApSubsystem::BeginPlay() {
	Super::BeginPlay();

	UWorld* world = GetWorld();
	RManager = AFGResearchManager::Get(world);
	SManager = AFGSchematicManager::Get(world);
	PManager  = AFGGamePhaseManager::Get(world);
	resourceSinkSubsystem = AFGResourceSinkSubsystem::Get(world);
	
	RManager->ResearchCompletedDelegate.AddDynamic(this, &AApSubsystem::OnMamResearchCompleted);
	SManager->PurchasedSchematicDelegate.AddDynamic(this, &AApSubsystem::OnSchematicCompleted);
}

void AApSubsystem::DispatchLifecycleEvent(ELifecyclePhase phase) {
	FApConfigurationStruct config = GetActiveConfig();
	if (!config.Enabled) {
		return;
	}
	if (phase == ELifecyclePhase::INITIALIZATION) {
		// TODO_MULTIPLAYER calling HasAuthority crashes multiplayer client? too early?
		// but we're using SpawnOnServer so why/how is client running this anyways
		if (HasAuthority()) {
			// Calling SetActorTickEnabled on client crashes regardless of true or false? related to above issue?
			SetActorTickEnabled(true);
		} else {
			UE_LOG(LogApSubsystem, Warning, TEXT("Archipelago Subsystem spawned/replicated on client, this is untested behavior. Keeping tick disabled."));
		}

		contentLibSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UContentLibSubsystem>();
		check(contentLibSubsystem)
		contentRegistry = AModContentRegistry::Get(GetWorld());
		check(contentRegistry)

		UE_LOG(LogApSubsystem, Display, TEXT("Initiating Archipelago server connection in background..."));
		ConnectToArchipelago(config);

		UE_LOG(LogApSubsystem, Display, TEXT("Generating schematics from AP Item IDs..."));
		for (auto item : ItemIdToGameRecipe)
			CreateSchematicBoundToItemId(item.Key);
		for (auto item : ItemIdToGameBuilding) 
			CreateSchematicBoundToItemId(item.Key);
			
		FDateTime connectingStartedTime = FDateTime::Now();
		FGenericPlatformProcess::ConditionalSleep([this, config, connectingStartedTime]() { return InitializeTick(config, connectingStartedTime); }, 0.5);
	} else if (phase == ELifecyclePhase::POST_INITIALIZATION) {
		if (ConnectionState != EApConnectionState::Connected) {
			FString message = FString::Printf(TEXT("Failed to connect to Archipelago server: \"%s\", for user \"%s\""), *config.Url, *config.Login);

			ChatMessageQueue.Enqueue(TPair<FString, FLinearColor>(message, FLinearColor::Red));
		}
	}
}

bool AApSubsystem::InitializeTick(FApConfigurationStruct config, FDateTime connectingStartedTime) {
	if (ConnectionState == EApConnectionState::Connecting) {
		if ((FDateTime::Now() - connectingStartedTime).GetSeconds() > 10)
			TimeoutConnection();
		else
			CheckConnectionState(config);
	} else if (ConnectionState == EApConnectionState::Connected) {
		if (!shouldParseItemsToScout) {
			if (hasLoadedSlotData) {
				HintUnlockedHubRecipies();
			} else {
				// awaiting slot data callback
			}
		} else {
			ParseScoutedItems();
			return true;
		}
	}

	return ConnectionState == EApConnectionState::ConnectionFailed;
}

void AApSubsystem::ConnectToArchipelago(FApConfigurationStruct config) {
	std::string const uri = TCHAR_TO_UTF8(*config.Url);
	std::string const user = TCHAR_TO_UTF8(*config.Login);
	std::string const password = TCHAR_TO_UTF8(*config.Password);

	AP_Init(uri.c_str(), "Satisfactory", user.c_str(), password.c_str());

	AP_SetItemClearCallback(AApSubsystem::ItemClearCallback);
	AP_SetItemRecvCallback(AApSubsystem::ItemReceivedCallback);
	AP_SetLocationCheckedCallback(AApSubsystem::LocationCheckedCallback);
	AP_RegisterSetReplyCallback(AApSubsystem::SetReplyCallback);
	AP_SetLocationInfoCallback(AApSubsystem::LocationScoutedCallback);
	AP_RegisterSlotDataRawCallback("Data", AApSubsystem::ParseSlotData);

	ConnectionState = EApConnectionState::Connecting;
	ConnectionStateDescription = LOCTEXT("Connecting", "Connecting...");

	AP_Start();
}

void AApSubsystem::OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubSystem::OnResearchCompleted(schematic), MAM Research Completed"));

	//if (schematic.) //if name is Archipelago #xxxx send check to server
}


void AApSubsystem::OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubSystem::OnSchematicCompleted(schematic)"));

	ESchematicType type = UFGSchematic::GetType(schematic);

	if (type != ESchematicType::EST_Milestone || !locationsPerMileStone.Contains(schematic))
		return;

	for (auto location : locationsPerMileStone[schematic])
		AP_SendItem(location.location);
}

void AApSubsystem::ItemClearCallback() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ItemClearCallback()"));

}

void AApSubsystem::ItemReceivedCallback(int64_t item, bool notify) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ItemReceivedCallback(%i, %s)"), item, (notify ? TEXT("true") : TEXT("false")));

	AApSubsystem* self = AApSubsystem::Get();
	self->ReceivedItems.Enqueue(item);
}

void AApSubsystem::LocationCheckedCallback(int64_t id) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::LocationCheckedCallback(%i)"), id);

}

void AApSubsystem::SetReplyCallback(AP_SetReply setReply) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::SetReplyCallback(%s)"), *UApUtils::FStr(setReply.key));

	if (callbacks.count(setReply.key))
		callbacks[setReply.key](setReply);
}

void AApSubsystem::LocationScoutedCallback(std::vector<AP_NetworkItem> scoutedLocations) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::HintUnlockedHubRecipies(vector[%i])"), scoutedLocations.size());

	AApSubsystem* self = AApSubsystem::Get();

	self->scoutedLocations = TArray<AP_NetworkItem>();

	for (auto location : scoutedLocations)
		self->scoutedLocations.Add(location);
	
	self->shouldParseItemsToScout = true;
}

void AApSubsystem::ParseSlotData(std::string json) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ParseSlotData(%s)"), *UApUtils::FStr(json));

	FString jsonString(json.c_str());

	const TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*jsonString);

	FJsonSerializer serializer;
	TSharedPtr<FJsonObject> parsedJson;

	serializer.Deserialize(reader, parsedJson);
	if (!parsedJson.IsValid()) {
		UE_LOG(LogApSubsystem, Fatal, TEXT("Archipelago SlotData Invalid! %s"), *jsonString);
		// TODO kick people out to the main menu screen or something, this keeps them hanging forever on the loading screen with no clear indication
		// Switched to Fatal for now so it closes the game, but there must be a better way
		return;
	}

	AApSubsystem* self = AApSubsystem::Get();

	for (TSharedPtr<FJsonValue> tier : parsedJson->GetArrayField("HubLayout")) {
		TArray<TMap<FString, int>> milestones;

		for (TSharedPtr<FJsonValue> milestone : tier->AsArray()) {
			TMap<FString, int> costs;

			for (TPair<FString, TSharedPtr<FJsonValue>> cost : milestone->AsObject()->Values) {
				int itemId = FCString::Atoi(*cost.Key);

				verify(ItemIdToGameItemDescriptor.Contains(itemId));

				costs.Add(ItemIdToGameItemDescriptor[itemId], cost.Value->AsNumber());
			}

			milestones.Add(costs);
		}

		self->hubLayout.Add(milestones);
	}

	TSharedPtr<FJsonObject> options = parsedJson->GetObjectField("Options");
	
	self->currentPlayerSlot = parsedJson->GetIntegerField("Slot");
	self->numberOfChecksPerMilestone = parsedJson->GetIntegerField("SlotsPerMilestone");
	self->finalSpaceElevatorTier = options->GetIntegerField("FinalElevatorTier");
	self->finalResourceSinkPoints = options->GetIntegerField("FinalResourceSinkPoints");
	self->hasLoadedSlotData = true;
}

void AApSubsystem::MonitorDataStoreValue(std::string key, AP_DataType dataType, std::string defaultValue, std::function<void(AP_SetReply)> callback) {
	callbacks[key] = callback;

	std::map<std::string, AP_DataType> keylist = { { key, dataType } };
	AP_SetNotify(keylist);

	AP_SetServerDataRequest setDefaultAndRecieceUpdate;
	setDefaultAndRecieceUpdate.key = key;

	AP_DataStorageOperation setDefault;
	setDefault.operation = "default";
	setDefault.value = &defaultValue;

	std::vector<AP_DataStorageOperation> operations;
	operations.push_back(setDefault);

	setDefaultAndRecieceUpdate.operations = operations;
	setDefaultAndRecieceUpdate.default_value = &defaultValue;
	setDefaultAndRecieceUpdate.type = dataType;
	setDefaultAndRecieceUpdate.want_reply = true;

	AP_SetServerData(&setDefaultAndRecieceUpdate);
}

void AApSubsystem::SetServerData(AP_SetServerDataRequest* setDataRequest) {
	AP_SetServerData(setDataRequest);
}

TEnumAsByte<EApConnectionState> AApSubsystem::GetConnectionState() {
	return TEnumAsByte<EApConnectionState>(ConnectionState);
}

// Called every frame
void AApSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (ConnectionState != EApConnectionState::Connected)
		return;

	// Consider processing only one queue item per tick for performance reasons
	int64_t item;
	while (ReceivedItems.Dequeue(item)) {
		if (ItemSchematics.Contains(item))
			SManager->GiveAccessToSchematic(ItemSchematics[item], nullptr);
		else if (auto trapName = ItemTraps.Find(item))
			// TODO no AP server defined traps yet to test this on yet, but can use chat command
			AApTrapSubsystem::Get()->SpawnTrap(*trapName, nullptr);
	}

	HandleAPMessages();

	if (!hasSendGoal) {
		if (
				(finalSpaceElevatorTier > 0 && 
				 PManager->GetGamePhase() >= finalSpaceElevatorTier)
			 || (finalResourceSinkPoints > 0 && 
				  resourceSinkSubsystem->GetNumTotalPoints(EResourceSinkTrack::RST_Default) >= finalResourceSinkPoints)
		) {
			AP_StoryComplete();
			hasSendGoal = true;
		}
	}
}

void AApSubsystem::CheckConnectionState(FApConfigurationStruct config) {
	if (ConnectionState == EApConnectionState::Connecting) {
		AP_ConnectionStatus status = AP_GetConnectionStatus();

		if (status == AP_ConnectionStatus::Authenticated) {
			ConnectionState = EApConnectionState::Connected;
			ConnectionStateDescription = LOCTEXT("AuthSuccess", "Authentication succeeded.");
			UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::Tick(), Successfully Authenticated"));
		} else if (status == AP_ConnectionStatus::ConnectionRefused) {
			ConnectionState = EApConnectionState::ConnectionFailed;
			ConnectionStateDescription = LOCTEXT("ConnectionRefused", "Connection refused by server. Check your connection details and load the save again.");
			UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::CheckConnectionState(), ConnectionRefused"));
			FString message = FString::Printf(TEXT("Failed to connect to Archipelago server: \"%s\", for user \"%s\""), *config.Url, *config.Login);

			ChatMessageQueue.Enqueue(TPair<FString, FLinearColor>(message, FLinearColor::Red));
		}
	}
}

void AApSubsystem::ParseScoutedItems() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ParseScoutedItems(vector[%i])"), scoutedLocations.Num());

	TMap<FString, TSubclassOf<UFGSchematic>> schematicsPerMilestone = TMap<FString, TSubclassOf<UFGSchematic>>();

	for (auto& location : scoutedLocations) {
		if (location.locationName.starts_with("Hub")) {
			std::string milestoneString = location.locationName.substr(0, location.locationName.find(","));
			FString milestone = UApUtils::FStr(milestoneString);

			if (!schematicsPerMilestone.Contains(milestone)) {
				TSubclassOf<UFGSchematic> schematic = UApUtils::FindOrCreateClass(TEXT("/Archipelago/"), *milestone, UFGSchematic::StaticClass());
				schematicsPerMilestone.Add(milestone, schematic);
			}

			if (!locationsPerMileStone.Contains(schematicsPerMilestone[milestone])) {
				locationsPerMileStone.Add(schematicsPerMilestone[milestone], TArray<AP_NetworkItem>{ location });
			} else {
				locationsPerMileStone[schematicsPerMilestone[milestone]].Add(location);
			}
		}
	}

	const auto recipeAssets = UApUtils::GetBlueprintAssetsIn("/Game/FactoryGame/Recipes");
	const auto itemDescriptorAssets = UApUtils::GetBlueprintAssetsIn("/Game/FactoryGame/Resource");

	for (auto& itemPerMilestone : locationsPerMileStone) {
		FString schematicName;
		for (auto schematicAndName : schematicsPerMilestone) {
			if (itemPerMilestone.Key == schematicAndName.Value) {
				schematicName = schematicAndName.Key;
				break;
			}
		}

		CreateHubSchematic(recipeAssets, itemDescriptorAssets, schematicName, itemPerMilestone.Key, itemPerMilestone.Value);
	}

	scoutedLocations.Empty();
	shouldParseItemsToScout = false;
}

void AApSubsystem::CreateSchematicBoundToItemId(int64_t item) {
	FString recipy = ItemIdToGameBuilding.Contains(item) ? ItemIdToGameBuilding[item] : ItemIdToGameRecipe[item];
	FString name = UApUtils::FStr("AP_ItemId_" + std::to_string(item));
	// https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_Schematic.json
	FString json = FString::Printf(TEXT(R"({
		"Name": "%s",
		"Type": "Custom",
		"Recipes": [ "%s" ]
	})"), *name, *recipy);

	FContentLib_Schematic schematic = UCLSchematicBPFLib::GenerateCLSchematicFromString(json);
	TSubclassOf<UFGSchematic> factorySchematic = UApUtils::FindOrCreateClass(TEXT("/Archipelago/"), *name, UFGSchematic::StaticClass());
	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, contentLibSubsystem);

	contentRegistry->RegisterSchematic(FName(TEXT("Archipelago")), factorySchematic);

	ItemSchematics.Add(item, factorySchematic);
}

void AApSubsystem::CreateRecipe(AP_NetworkItem item) {
	FString name(("AP_ITEM_RECIPE_" + item.playerName + " - " + item.itemName).c_str());
	FString uniqueId = UApUtils::FStr(item.location);
	// https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_Recipe.json
	FString json = FString::Printf(TEXT(R"({
		 "Name": "%s",
		 "Ingredients": [],
		 "Products": [
			  {
					"Item": "AP_Logo_Item",
					"Amount": 1
			  }
		 ],
		 "ManufacturingDuration": 1,
		 "ProducedIn": [
			  "Build_HadronCollider"
		 ]
	})"), *name);

	FContentLib_Recipe clRecipy = UCLRecipeBPFLib::GenerateCLRecipeFromString(json);
	TSubclassOf<UFGRecipe> factoryRecipy = UApUtils::FindOrCreateClass(TEXT("/Archipelago/"), *uniqueId, UFGRecipe::StaticClass());
	UCLRecipeBPFLib::InitRecipeFromStruct(contentLibSubsystem, clRecipy, factoryRecipy);

	contentRegistry->RegisterRecipe(FName(TEXT("Archipelago")), factoryRecipy);
}

void AApSubsystem::CreateDescriptor(AP_NetworkItem item) {
	FString name(("AP_ITEM_DESC_" + item.playerName + " " + item.itemName).c_str());
	FString uniqueId = UApUtils::FStr(item.location);
	// https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_Item.json
	FString json = FString::Printf(TEXT(R"({
		 "Name": "%s",
		 "Description": "TODO: Implement",
		 "StackSize": "One",
		 "Category": "AP",
		 "VisualKit": "Kit_AP_Logo",
		 "NameShort": "APITM",
		 "CanBeDiscarded": false,
		 "RememberPickUp": false,
		 "EnergyValue": 0,
		 "RadioactiveDecay": 0,
		 "ResourceSinkPoints": 0
	})"), *name);

	FContentLib_Item clItem = UCLItemBPFLib::GenerateCLItemFromString(json);
	TSubclassOf<UFGItemDescriptor> factoryItem = UApUtils::FindOrCreateClass(TEXT("/Archipelago/"), *uniqueId, UFGItemDescriptor::StaticClass());
	UCLItemBPFLib::InitItemFromStruct(factoryItem, clItem, contentLibSubsystem);

	//contentRegistry->RegisterItem(FName(TEXT("Archipelago")), factoryItem); //no idea how/where to register items
}

void AApSubsystem::CreateHubSchematic(TMap<FName, FAssetData> recipeAssets, TMap<FName, FAssetData> itemDescriptorAssets, FString name, TSubclassOf<UFGSchematic> factorySchematic, TArray<AP_NetworkItem> items) {
	int delimeterPos;
	name.FindChar('-', delimeterPos);
	int32 tier = FCString::Atoi(*name.Mid(delimeterPos - 1, 1));
	int32 milestone = FCString::Atoi(*name.Mid(delimeterPos + 1, 1));

	FString costs = "";
	for (auto& cost : hubLayout[tier - 1][milestone - 1]) {
		FString costJson = FString::Printf(TEXT(R"({
			"Item": "%s",
			"Amount": %i
		},)"), *cost.Key, cost.Value);

		costs += costJson;
	}

	// https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_Schematic.json
	FString json = FString::Printf(TEXT(R"({
		"Name": "%s",
		"Type": "Milestone",
		"Time": 200,
		"Tier": %i,
		"VisualKit": "Kit_AP_Logo",
		"Cost": [ %s ]
	})"), *name, tier, *costs);

	FContentLib_Schematic schematic = UCLSchematicBPFLib::GenerateCLSchematicFromString(json);

	for (auto& item : items)
		schematic.InfoCards.Add(CreateUnlockInfoOnly(recipeAssets, itemDescriptorAssets, item));

	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, contentLibSubsystem);

	contentRegistry->RegisterSchematic(FName(TEXT("Archipelago")), factorySchematic);
}

FContentLib_UnlockInfoOnly AApSubsystem::CreateUnlockInfoOnly(TMap<FName, FAssetData> recipeAssets, TMap<FName, FAssetData> itemDescriptorAssets, AP_NetworkItem item) {
	FFormatNamedArguments Args;
	if (item.flags == 0b001) {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeAdvancement", "progression item"));
	} else if (item.flags == 0b010) {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeUseful", "useful item"));
	} else if (item.flags == 0b100) {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeTrap", "trap"));
	} else {
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionTypeJunk", "normal item"));
	}

	Args.Add(TEXT("ApItemName"), UApUtils::FText(item.itemName));

	FContentLib_UnlockInfoOnly infoCard;

	if (item.player == currentPlayerSlot) {
		Args.Add(TEXT("ApPlayerName"), FText::FromString(TEXT("your")));

		infoCard.mUnlockName = UApUtils::FText(item.itemName);

		if (ItemIdToGameBuilding.Contains(item.item)) {
			UpdateInfoOnlyUnlockWithBuildingInfo(&infoCard, Args, recipeAssets, &item);
		} else if (ItemIdToGameRecipe.Contains(item.item)) {
			UpdateInfoOnlyUnlockWithRecipeInfo(&infoCard, Args, recipeAssets, &item);
		} else if (ItemIdToGameItemDescriptor.Contains(item.item)) {
			UpdateInfoOnlyUnlockWithItemInfo(&infoCard, Args, itemDescriptorAssets, &item);
		} else {
			UpdateInfoOnlyUnlockWithGenericApInfo(&infoCard, Args, &item);
		}
	} else {
		Args.Add(TEXT("ApPlayerName"), UApUtils::FText(item.playerName + "'s"));

		infoCard.mUnlockName = FText::Format(LOCTEXT("NetworkItemUnlockDisplayName", "{ApPlayerName} {ApItemName}"), Args);

		UpdateInfoOnlyUnlockWithGenericApInfo(&infoCard, Args, &item);
	}

	return infoCard;
}

void AApSubsystem::UpdateInfoOnlyUnlockWithBuildingInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, TMap<FName, FAssetData> buildingRecipyAssets, AP_NetworkItem* item) {
	UFGRecipe* recipe = GetRecipeByName(buildingRecipyAssets, ItemIdToGameBuilding[item->item]);
	UFGItemDescriptor* itemDescriptor = recipe->GetProducts()[0].ItemClass.GetDefaultObject();

	infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
	infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Building.Recipe_Icon_Building");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalBuildingDescription", "This will unlock your {ApItemName}"), Args);
}

void AApSubsystem::UpdateInfoOnlyUnlockWithRecipeInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, TMap<FName, FAssetData> recipeAssets, AP_NetworkItem* item) {
	UFGRecipe* recipe = GetRecipeByName(recipeAssets, ItemIdToGameRecipe[item->item]);
	UFGItemDescriptor* itemDescriptor = recipe->GetProducts()[0].ItemClass.GetDefaultObject();

	TArray<FString> BuildingArray;
	TArray<TSubclassOf<UObject>> buildings;
	recipe->GetProducedIn(buildings);
	for (TSubclassOf<UObject> buildingObject : buildings) {
		if (buildingObject->IsChildOf(AFGBuildable::StaticClass())) {
			AFGBuildable* building = Cast<AFGBuildable>(buildingObject.GetDefaultObject());
			if (building != nullptr) {
				if (building->IsA(AFGBuildableAutomatedWorkBench::StaticClass()))
					BuildingArray.Add("Workbench");
				else
					BuildingArray.Add(building->mDisplayName.ToString());
			}
		}
	}

	TArray<FString> CostsArray;
	for (FItemAmount cost : recipe->GetIngredients()) {
		UFGItemDescriptor* costItemDescriptor = cost.ItemClass.GetDefaultObject();
		CostsArray.Add(costItemDescriptor->GetItemNameFromInstanceAsString());
	}

	TArray<FString> OutputArray;
	for (FItemAmount product : recipe->GetProducts()) {
		UFGItemDescriptor* productItemDescriptor = product.ItemClass.GetDefaultObject();
		OutputArray.Add(productItemDescriptor->GetItemNameFromInstanceAsString());
	}

	Args.Add(TEXT("Building"), FText::FromString(FString::Join(BuildingArray, TEXT(", "))));
	Args.Add(TEXT("Costs"), FText::FromString(FString::Join(CostsArray, TEXT(", "))));
	Args.Add(TEXT("Output"), FText::FromString(FString::Join(OutputArray, TEXT(", "))));

	infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
	infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Decor.Recipe_Icon_Decor");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalRecipeDescription", "This will unlock {ApPlayerName} {ApItemName} which is considered a {ProgressionType}.\nProduced in: {Building}.\nCosts: {Costs}.\nProduces: {Output}."), Args);
}

void AApSubsystem::UpdateInfoOnlyUnlockWithItemInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, TMap<FName, FAssetData> itemDescriptorAssets, AP_NetworkItem* item) {
	UFGItemDescriptor* itemDescriptor = GetItemDescriptorByName(itemDescriptorAssets, ItemIdToGameItemDescriptor[item->item]);

	infoCard->BigIcon = infoCard->SmallIcon = UApUtils::GetImagePathForItem(itemDescriptor);
	infoCard->CategoryIcon = TEXT("/Game/FactoryGame/Buildable/Factory/TradingPost/UI/RecipeIcons/Recipe_Icon_Item.Recipe_Icon_Item");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockPersonalItemDescription", "This will give {ApPlayerName} {ApItemName}"), Args);
}

void AApSubsystem::UpdateInfoOnlyUnlockWithGenericApInfo(FContentLib_UnlockInfoOnly* infoCard, FFormatNamedArguments Args, AP_NetworkItem* item) {
	infoCard->CategoryIcon = TEXT("/Archipelago/Assets/ArchipelagoIconWhite128.ArchipelagoIconWhite128");
	infoCard->mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockDescription", "This will unlock {ApPlayerName} {ApItemName} which is considered a {ProgressionType}."), Args);

	if (item->flags == 0b001) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/AP-Purple.AP-Purple");
	}	else if (item->flags == 0b010) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/AP-Blue.AP-Blue");
	} else if (item->flags == 0b100) {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/AP-Red.AP-Red");
	} else {
		infoCard->BigIcon = infoCard->SmallIcon = TEXT("/Archipelago/Assets/AP-Cyan.AP-Cyan");
	}
}

void AApSubsystem::HandleAPMessages() {
	for (int i = 0; i < 10; i++) {
		TPair<FString, FLinearColor> queuedMessage;
		if (ChatMessageQueue.Dequeue(queuedMessage)) {
			SendChatMessage(queuedMessage.Key, queuedMessage.Value);
		} else {
			if (!AP_IsMessagePending())
				return;

			AP_Message* message = AP_GetLatestMessage();
			SendChatMessage(UApUtils::FStr(message->text), FLinearColor::White);

			AP_ClearLatestMessage();
		}
	}
}

void AApSubsystem::SendChatMessage(const FString& Message, const FLinearColor& Color) {
	// TODO this does not replicate to multiplayer clients
	AFGChatManager* ChatManager = AFGChatManager::Get(GetWorld());
	FChatMessageStruct MessageStruct;
	MessageStruct.MessageString = Message;
	MessageStruct.MessageType = EFGChatMessageType::CMT_SystemMessage;
	MessageStruct.ServerTimeStamp = GetWorld()->TimeSeconds;
	MessageStruct.CachedColor = Color;
	ChatManager->AddChatMessageToReceived(MessageStruct);

	UE_LOG(LogApChat, Display, TEXT("Archipelago Chat Message: %s"), *Message);
}

void AApSubsystem::HintUnlockedHubRecipies() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::HintUnlockedHubRecipies()"));

	std::vector<int64_t> locations;

	int maxMilestones = 5;
	int maxSlots = 10;

	int64_t hubBaseId = 1338000;

	for (int tier = 1; tier <= hubLayout.Num(); tier++)
	{
		for (int milestone = 1; milestone <= maxMilestones; milestone++)
		{
			for (int slot = 1; slot <= maxSlots; slot++)
			{
				if (milestone <= hubLayout[tier - 1].Num() && slot <= numberOfChecksPerMilestone)
					locations.push_back(hubBaseId);

				hubBaseId++;
			}
		}
	}

	AP_SendLocationScouts(locations, 0);
}

UFGRecipe* AApSubsystem::GetRecipeByName(TMap<FName, FAssetData> recipeAssets, FString name) {
	return Cast<UFGRecipe>(UApUtils::FindAssetByName(recipeAssets, name.Append("_C")));
}

UFGItemDescriptor* AApSubsystem::GetItemDescriptorByName(TMap<FName, FAssetData> itemDescriptorAssets, FString name) {
	return Cast<UFGItemDescriptor>(UApUtils::FindAssetByName(itemDescriptorAssets, name));
}

void AApSubsystem::TimeoutConnection() {
	ConnectionState = EApConnectionState::ConnectionFailed;
	ConnectionStateDescription = LOCTEXT("AuthFailed", "Authentication failed. Check your connection details and load the save again.");
	UE_LOG(LogApSubsystem, Error, TEXT("AApSubsystem::TimeoutConnectionIfNotConnected(), Authenticated Failed"));

	SetActorTickEnabled(false);
}

FApConfigurationStruct AApSubsystem::GetActiveConfig() {
	UConfigManager* ConfigManager = GEngine->GetEngineSubsystem<UConfigManager>();
	FConfigId ConfigId { "Archipelago", "" };
	auto Config = ConfigManager->GetConfigurationById(ConfigId);
	auto ConfigProperty = URuntimeBlueprintFunctionLibrary::GetModConfigurationPropertyByClass(Config);
	auto CPSection = Cast<UConfigPropertySection>(ConfigProperty);

	FApConfigurationStruct config;
	config.Enabled = Cast<UConfigPropertyBool>(CPSection->SectionProperties["Enabled"])->Value;
	config.Url = Cast<UConfigPropertyString>(CPSection->SectionProperties["Url"])->Value;
	config.Game = Cast<UConfigPropertyString>(CPSection->SectionProperties["Game"])->Value;
	config.Login = Cast<UConfigPropertyString>(CPSection->SectionProperties["Login"])->Value;
	config.Password = Cast<UConfigPropertyString>(CPSection->SectionProperties["Password"])->Value;

	return config;
}

#pragma optimize("", on)

#undef LOCTEXT_NAMESPACE
