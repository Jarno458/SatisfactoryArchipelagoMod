#include "Subsystem/ApSubsystem.h"
#include "ApUtils.h"

DEFINE_LOG_CATEGORY(LogApSubsystem);

DEFINE_LOG_CATEGORY(LogApChat);

//TODO REMOVE
#pragma optimize("", off)

#define LOCTEXT_NAMESPACE "Archipelago"

std::map<std::string, std::function<void(AP_SetReply)>> AApSubsystem::callbacks;

TMap<int64_t, FString> AApSubsystem::ItemIdToGameName = {
	//Parts
	{1337500, TEXT("Desc_SpaceElevatorPart_5_C")}, //Adaptive Control Unit
	{1337501, TEXT("Desc_CircuitBoardHighSpeed_C")}, //AI Limiter
	{1337502, TEXT("Desc_AluminumPlate_C")},
	{1337503, TEXT("Desc_Crystal_C")}, //Blue Power Slug
	{1337504, TEXT("Desc_Crystal_mk2_C")}, //Yellow Power Slug
	{1337505, TEXT("Desc_AlienProtein_C")},
	{1337506, TEXT("Desc_Crystal_mk3_C")}, //Purple Power Slug
	{1337507, TEXT("Desc_AluminumCasing_C")},
	{1337508, TEXT("Desc_AluminumIngot_C")},
	{1337509, TEXT("Desc_AluminumScrap_C")},
	{1337510, TEXT("Desc_SpaceElevatorPart_7_C")}, //Assembly Director System
	{1337511, TEXT("Desc_SpaceElevatorPart_3_C")}, //Automated Wiring
	{1337512, TEXT("Desc_Battery_C")},
	{1337513, TEXT("Desc_OreBauxite_C")},
	{1337514, TEXT("BP_EquipmentDescriptorBeacon_C")}, //Beacon
	{1337515, TEXT("Desc_GenericBiomass_C")},
	{1337516, TEXT("Desc_Gunpowder_C")}, //Black Powder
	{1337517, TEXT("Desc_Cable_C")},
	{1337518, TEXT("Desc_GoldIngot_C")}, //Caterium Ingot
	{1337519, TEXT("Desc_OreGold_C")}, //Caterium Ore
	{1337520, TEXT("Desc_CircuitBoard_C")},
	{1337521, TEXT("Desc_Coal_C")},
	{1337522, TEXT("Desc_ColorCartridge_C")},
	{1337523, TEXT("Desc_CompactedCoal_C")},
	{1337524, TEXT("Desc_Computer_C")},
	{1337525, TEXT("Desc_Cement_C")}, //Concrete
	{1337526, TEXT("Desc_CoolingSystem_C")},
	{1337527, TEXT("Desc_CopperIngot_C")},
	{1337528, TEXT("Desc_OreCopper_C")}, //Copper Ore
	{1337529, TEXT("Desc_CopperDust_C")},
	{1337530, TEXT("Desc_CopperSheet_C")},
	{1337531, TEXT("Desc_CharacterRunStatue_C")},
	{1337532, TEXT("Desc_CrystalOscillator_C")},
	{1337533, TEXT("Desc_ElectromagneticControlRod_C")},
	{1337534, TEXT("Desc_FluidCanister_C")},
	{1337535, TEXT("Desc_GasTank_C")}, //Empty Fluid Tank
	{1337536, TEXT("Desc_SteelPlateReinforced_C")}, //Encased Industrial Beam
	{1337537, TEXT("Desc_PlutoniumCell_C")},
	{1337538, TEXT("Desc_UraniumCell_C")},
	{1337539, TEXT("Desc_Fabric_C")},
	{1337540, TEXT("Desc_ResourceSinkCoupon_C")},
	{1337541, TEXT("Desc_FlowerPetals_C")},
	{1337542, TEXT("Desc_ModularFrameFused_C")},
	{1337543, TEXT("Desc_HardDrive_C")},
	{1337544, TEXT("Desc_AluminumPlateReinforced_C")}, //Heatsink
	{1337545, TEXT("Desc_ModularFrameHeavy_C")},
	{1337546, TEXT("Desc_HighSpeedConnector_C")},
	{1337547, TEXT("Desc_CharacterSpin_Statue_C")},
	{1337548, TEXT("Desc_CharacterClap_Statue_C")},
	{1337549, TEXT("Desc_IronIngot_C")},
	{1337550, TEXT("Desc_OreIron_C")},
	{1337551, TEXT("Desc_IronPlate_C")},
	{1337552, TEXT("Desc_IronRod_C")},
	{1337553, TEXT("Desc_GoldenNut_Statue_C")},
	{1337554, TEXT("Desc_Leaves_C")},
	{1337555, TEXT("Desc_Stone_C")}, //Limestone
	{1337556, TEXT("Desc_SpaceElevatorPart_6_C")}, //Magnetic Field Generator
	{1337557, TEXT("Desc_WAT2_C")}, //Mercer Sphere
	{1337558, TEXT("Desc_SpaceElevatorPart_4_C")}, //Modular Engine
	{1337559, TEXT("Desc_ModularFrame_C")},
	{1337560, TEXT("Desc_Motor_C")},
	{1337561, TEXT("Desc_Mycelia_C")},
	{1337562, TEXT("Desc_NonFissibleUranium_C")},
	{1337563, TEXT("Desc_SpaceElevatorPart_9_C")}, //Nuclear Pasta
	{1337564, TEXT("Desc_DoggoStatue_C")},
	{1337565, TEXT("Desc_AlienDNACapsule_C")},
	{1337566, TEXT("Desc_PackagedAlumina_C")},
	{1337567, TEXT("Desc_Fuel_C")},
	{1337568, TEXT("Desc_PackagedOilResidue_C")},
	{1337569, TEXT("Desc_PackagedBiofuel_C")},
	{1337570, TEXT("Desc_PackagedNitricAcid_C")},
	{1337571, TEXT("Desc_PackagedNitrogenGas_C")},
	{1337572, TEXT("Desc_PackagedOil_C")},
	{1337573, TEXT("Desc_PackagedSulfuricAcid_C")},
	{1337574, TEXT("Desc_TurboFuel_C")}, //Packaged Turno Fuel
	{1337575, TEXT("Desc_PackagedWater_C")},
	{1337576, TEXT("Desc_PetroleumCoke_C")},
	{1337577, TEXT("Desc_Plastic_C")},
	{1337578, TEXT("Desc_PlutoniumFuelRod_C")},
	{1337579, TEXT("Desc_PlutoniumPellet_C")},
	{1337580, TEXT("Desc_PlutoniumWaste_C")},
	{1337581, TEXT("Desc_PolymerResin_C")},
	{1337582, TEXT("Desc_CrystalShard_C")}, //Power Shard
	{1337583, TEXT("Desc_SpaceGiraffeStatue_C")},
	{1337584, TEXT("Desc_PressureConversionCube_C")},
	{1337585, TEXT("Desc_ComputerQuantum_C")},
	{1337586, TEXT("Desc_QuartzCrystal_C")},
	{1337587, TEXT("Desc_HighSpeedWire_C")},
	{1337588, TEXT("Desc_ModularFrameLightweight_C")},
	{1337589, TEXT("Desc_RawQuartz_C")},
	{1337590, TEXT("Desc_IronPlateReinforced_C")},
	{1337591, TEXT("Desc_Rotor_C")},
	{1337592, TEXT("Desc_Rubber_C")},
	{1337593, TEXT("Desc_SAM_C")},
	{1337594, TEXT("Desc_IronScrew_C")},
	{1337595, TEXT("Desc_Silica_C")},
	{1337596, TEXT("Desc_SpaceElevatorPart_1_C")}, //Smart Plating
	{1337597, TEXT("Desc_GunpowderMK2_C")}, //Smokeless Powder
	{1337598, TEXT("Desc_Biofuel_C")}, //Solid Biofuel
	{1337599, TEXT("Desc_WAT1_C")}, //Somersloop
	{1337600, TEXT("Desc_Stator_C")},
	{1337601, TEXT("Desc_Hog_Statue_C")},
	{1337502, TEXT("Desc_SteelPlate_C")},
	{1337503, TEXT("Desc_SteelIngot_C")},
	{1337504, TEXT("Desc_SteelPipe_C")},
	{1337505, TEXT("Desc_Sulfur_C")},
	{1337506, TEXT("Desc_ComputerSuper_C")},
	{1337507, TEXT("Desc_QuantumOscillator_C")},
	{1337508, TEXT("Desc_SpaceElevatorPart_8_C")}, //Thermal Propulsion Rocket
	{1337509, TEXT("Desc_MotorLightweight_C")},
	{1337510, TEXT("Desc_HogParts_C'")},
	{1337511, TEXT("Desc_OreUranium_C")},
	{1337512, TEXT("Desc_NuclearFuelRod_C")},
	{1337513, TEXT("Desc_NuclearWaste_C")},
	{1337514, TEXT("Desc_SpaceElevatorPart_2_C")}, //Versatile Framework
	{1337515, TEXT("Desc_Wire_C")},
	{1337516, TEXT("Desc_Wood_C")},
	{1337517, TEXT("Desc_SpitterParts_C")},
	{1337518, TEXT("Desc_StingerParts_C")},
	{1337519, TEXT("Desc_HatcherParts_C")},

	//Enquipment/Ammo
	{1337650, TEXT("Desc_Shroom_C")},
	{1337651, TEXT("Desc_Nut_C")},
	{1337652, TEXT("BP_EquipmentDescriptorJumpingStilts_C")},
	//{1337653, TEXT("")}, //BoomBox
	{1337654, TEXT("Desc_Chainsaw_C")},
	{1337655, TEXT("Desc_NobeliskCluster_C")},
	//{1337656, TEXT("Unused")},
	{1337657, TEXT("BP_EquipmentDescriptorCup")},
	{1337658, TEXT("BP_EquipmentDescriptorCupGold")},
	{1337659, TEXT("Desc_Rebar_Explosive_C")},
	{1337660, TEXT("Desc_GolfCart_C")},
	{1337661, TEXT("Desc_GolfCartGold")},
	{1337662, TEXT("BP_EquipmentDescriptorGasmask_C")},
	{1337663, TEXT("Desc_NobeliskGas_C")},
	{1337664, TEXT("BP_EquipmentDescriptorHazmatSuit_C")},
	{1337665, TEXT("Desc_CartridgeSmartProjectile_C")},
	{1337666, TEXT("BP_EquipmentDescriptorHoverPack_C")},
	{1337667, TEXT("Desc_SpikedRebar_C")},
	{1337668, TEXT("BP_EquipmentDescriptorJetPack_C")},
	{1337669, TEXT("Desc_Medkit_C")},
	{1337670, TEXT("Desc_NobeliskExplosive_C")},
	{1337671, TEXT("BP_EquipmentDescriptorNobeliskDetonator_C")},
	{1337672, TEXT("Desc_NobeliskNuke_C")},
	{1337673, TEXT("BP_EquipmentDescriptorObjectScanner_C")},
	{1337674, TEXT("Desc_Berry_C")},
	{1337675, TEXT("Desc_Parachute_C")},
	{1337676, TEXT("Desc_NobeliskShockwave_C")},
	{1337677, TEXT("Desc_RebarGunProjectile_C")},
	{1337678, TEXT("BP_EquipmentDescriptorRifle_C")},
	{1337679, TEXT("Desc_CartridgeStandard_C")},
	{1337680, TEXT("Desc_Rebar_Spreadshot_C")},
	{1337681, TEXT("Desc_Rebar_Stunshot_C")},
	{1337682, TEXT("Desc_CartridgeChaos_C")},
	{1337683, TEXT("BP_EquipmentDescriptorStunSpear_C")},
	{1337684, TEXT("BP_EquipmentDescriptorShockShank_C")},
	{1337685, TEXT("BP_EqDescZipLine_C")},
	{1337686, TEXT("BP_ItemDescriptorPortableMiner_C")}
};


TMap<int64_t, FString> AApSubsystem::ItemIdToGameSchematic = {
	{1337700, TEXT("Recipe_IronPlateReinforced")},
	{1337701, TEXT("Recipe_Alternate_AdheredIronPlate")},
	{1337702, TEXT("Recipe_Alternate_ReinforcedIronPlate_1")},
	{1337703, TEXT("Recipe_Alternate_ReinforcedIronPlate_2")},
	{1337704, TEXT("Recipe_Rotor")},
	{1337705, TEXT("Recipe_Alternate_CopperRotor")},
	{1337706, TEXT("Recipe_Alternate_Rotor")},
	{1337707, TEXT("Recipe_Stator")},
	{1337708, TEXT("Recipe_Alternate_Stator")},
	{1337709, TEXT("Recipe_Plastic")},
	{1337710, TEXT("Recipe_ResidualPlastic")},
	{1337711, TEXT("Recipe_Alternate_Plastic_1")},
	{1337712, TEXT("Recipe_Rubber")},
	{1337713, TEXT("Recipe_ResidualRubber")},
	{1337714, TEXT("Recipe_Alternate_RecycledRubber")},
	{1337715, TEXT("Recipe_IronPlate")},
	{1337716, TEXT("Recipe_Alternate_CoatedIronPlate")},
	{1337717, TEXT("Recipe_Alternate_SteelCoatedPlate")},
	{1337718, TEXT("Recipe_IronRod")},
	{1337719, TEXT("Recipe_Alternate_SteelRod")},
	{1337720, TEXT("Recipe_Screw")},
	{1337721, TEXT("Recipe_Alternate_Screw")},
	{1337722, TEXT("Recipe_Alternate_Screw_2")},
	{1337723, TEXT("Recipe_Wire")},
	{1337724, TEXT("Recipe_Alternate_FusedWire")},
	{1337725, TEXT("Recipe_Alternate_Wire_1")}, 
	{1337726, TEXT("Recipe_Alternate_Wire_2")},
	{1337727, TEXT("Recipe_Cable")},
	{1337728, TEXT("Recipe_Alternate_CoatedCable")},
	{1337729, TEXT("Recipe_Alternate_Cable_1")},
	{1337730, TEXT("Recipe_Alternate_Cable_2")},
	{1337731, TEXT("Recipe_Quickwire")},
	{1337732, TEXT("Recipe_Alternate_Quickwire")},
	{1337733, TEXT("Recipe_CopperSheet")},
	{1337734, TEXT("Recipe_Alternate_SteamedCopperSheet")},
	{1337735, TEXT("Recipe_SteelPipe")},
	{1337736, TEXT("Recipe_SteelBeam")},
	//{1337737, TEXT("")}, //Crude Oil
	{1337738, TEXT("Recipe_Alternate_HeavyOilResidue")},
	{1337739, TEXT("Recipe_Alternate_PolymerResin")},
	{1337740, TEXT("Recipe_LiquidFuel")},
	{1337741, TEXT("Recipe_ResidualFuel")},
	{1337742, TEXT("Recipe_Alternate_DilutedPackagedFuel")},
	//{1337743, TEXT("")}, //Water
	{1337744, TEXT("Recipe_Concrete")},
	{1337745, TEXT("Recipe_Alternate_RubberConcrete")},
	{1337746, TEXT("Recipe_Alternate_WetConcrete")},
	{1337747, TEXT("Recipe_Alternate_Concrete")},
	{1337748, TEXT("Recipe_Silica")},
	{1337749, TEXT("Recipe_Alternate_Silica")},
	{1337750, TEXT("Recipe_QuartzCrystal")},
	{1337751, TEXT("Recipe_Alternate_PureQuartzCrystal")},
	{1337752, TEXT("Recipe_IngotIron")},
	{1337753, TEXT("Recipe_Alternate_PureIronIngot")},
	{1337754, TEXT("Recipe_Alternate_IngotIron")},
	{1337755, TEXT("Recipe_IngotSteel")},
	{1337756, TEXT("Recipe_Alternate_CokeSteelIngot")},
	{1337757, TEXT("Recipe_Alternate_IngotSteel_2")},
	{1337758, TEXT("Recipe_Alternate_IngotSteel_1")},
	{1337759, TEXT("Recipe_IngotCopper")},
	{1337760, TEXT("Recipe_Alternate_CopperAlloyIngot")},
	{1337761, TEXT("Recipe_Alternate_PureCopperIngot")},
	{1337762, TEXT("Recipe_IngotCaterium")},
	{1337763, TEXT("Recipe_Alternate_PureCateriumIngot")},
	//{1337764, TEXT("")}, //Limestone
	//{1337765, TEXT("")}, //Raw Quartz
	//{1337766, TEXT("")}, //Iron Ore
	//{1337767, TEXT("")}, //Copper Ore
	//{1337768, TEXT("")}, //Coal
	//{1337769, TEXT("")}, //Sulfur
	//{1337770, TEXT("")}, //Caterium Ore
	{1337771, TEXT("Recipe_PetroleumCoke")},
	{1337772, TEXT("Recipe_Alternate_EnrichedCoal")},
	{1337773, TEXT("Recipe_Motor")},
	{1337774, TEXT("Recipe_Alternate_Motor_1")},
	{1337775, TEXT("Recipe_Alternate_ElectricMotor")},
	{1337776, TEXT("Recipe_ModularFrame")},
	{1337777, TEXT("Recipe_Alternate_BoltedFrame")},
	{1337778, TEXT("Recipe_Alternate_ModularFrame")},
	{1337779, TEXT("Recipe_ModularFrameHeavy")},
	{1337780, TEXT("Recipe_Alternate_FlexibleFramework")},
	{1337781, TEXT("Recipe_Alternate_ModularFrameHeavy")},
	{1337782, TEXT("Recipe_EncasedIndustrialBeam")},
	{1337783, TEXT("Recipe_Alternate_EncasedIndustrialBeam")},
	{1337784, TEXT("Recipe_Computer")},
	{1337785, TEXT("Recipe_Alternate_Computer_2")},
	{1337786, TEXT("Recipe_Alternate_Computer_1")},
	{1337787, TEXT("Recipe_CircuitBoard")},
	{1337788, TEXT("Recipe_Alternate_ElectrodeCircuitBoard")},
	{1337789, TEXT("Recipe_Alternate_CircuitBoard_1")},
	{1337790, TEXT("Recipe_Alternate_CircuitBoard_2")},
	{1337791, TEXT("Recipe_CrystalOscillator")},
	{1337792, TEXT("Recipe_Alternate_CrystalOscillator")},
	{1337793, TEXT("Recipe_AILimiter")},
	{1337794, TEXT("Recipe_ElectromagneticControlRod")},
	{1337795, TEXT("Recipe_Alternate_ElectromagneticControlRod_1")},
	{1337796, TEXT("Recipe_HighSpeedConnector")},
	{1337797, TEXT("Recipe_Alternate_HighSpeedConnector")},
	{1337798, TEXT("Recipe_SpaceElevatorPart_1")},
	{1337799, TEXT("Recipe_Alternate_PlasticSmartPlating")},
	{1337800, TEXT("Recipe_SpaceElevatorPart_2")},
	{1337801, TEXT("Recipe_Alternate_FlexibleFramework")},
	{1337802, TEXT("Recipe_SpaceElevatorPart_3")},
	{1337803, TEXT("Recipe_Alternate_HighSpeedWiring")},
	{1337804, TEXT("Recipe_SpaceElevatorPart_4")},
	{1337805, TEXT("Recipe_SpaceElevatorPart_5")},
	{1337806, TEXT("Recipe_Alternate_DilutedFuel")},
	{1337807, TEXT("Recipe_AluminaSolution")},
	{1337808, TEXT("Recipe_Alternate_AutomatedMiner")},

	//Buildings
	{1337900, TEXT("Recipe_ConstructorMk1")},
	{1337901, TEXT("Recipe_AssemblerMk1")},
	{1337902, TEXT("Recipe_ManufacturerMk1")},
	{1337903, TEXT("Recipe_Packager")},
	{1337904, TEXT("Recipe_OilRefinery")},
	{1337905, TEXT("Recipe_Blender")},
	{1337906, TEXT("Recipe_HadronCollider")},
	{1337907, TEXT("Recipe_GeneratorBiomass")},
	{1337908, TEXT("Recipe_GeneratorCoal")},
	{1337909, TEXT("Recipe_GeneratorGeoThermal")},
	{1337910, TEXT("Recipe_GeneratorNuclear")},
	{1337911, TEXT("Recipe_MinerMk1")},
	{1337912, TEXT("Recipe_MinerMk2")},
	{1337913, TEXT("Recipe_MinerMk3")},
	{1337914, TEXT("Recipe_OilPump")},
	{1337915, TEXT("Recipe_WaterPump")},
	{1337916, TEXT("Recipe_SmelterBasicMk1")},
	{1337917, TEXT("Recipe_SmelterMk1")},
	{1337918, TEXT("Recipe_SpaceElevator")},
};


TMap<int64_t, FString> AApSubsystem::ItemIdToGameName2 = {
};

AApSubsystem::AApSubsystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
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

void AApSubsystem::BeginPlay()
{
	Super::BeginPlay();

	RManager = AFGResearchManager::Get(GetWorld());
	SManager = AFGSchematicManager::Get(GetWorld());

	RManager->ResearchCompletedDelegate.AddDynamic(this, &AApSubsystem::OnMamResearchCompleted);
	SManager->PurchasedSchematicDelegate.AddDynamic(this, &AApSubsystem::OnSchematicCompleted);
}

void AApSubsystem::DispatchLifecycleEvent(ELifecyclePhase phase) {
	if (phase == ELifecyclePhase::INITIALIZATION) {
		FApConfigurationStruct config = GetActiveConfig();

		if (!config.Enabled) {
			SetActorTickEnabled(false);
			return;
		}

		contentLibSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UContentLibSubsystem>();
		check(contentLibSubsystem)
		contentRegistry = AModContentRegistry::Get(GetWorld());
		check(contentRegistry)

		UE_LOG(LogApSubsystem, Display, TEXT("Initiating Archipelago server connection in background..."));
		ConnectToArchipelago(config);

		UE_LOG(LogApSubsystem, Display, TEXT("Generating schematics from AP Item IDs..."));
		for (auto item : ItemIdToGameSchematic) {
			CreateSchematicBoundToItemId(item.Key);
		}
	
		FDateTime connectingStartedTime = FDateTime::Now();
		FGenericPlatformProcess::ConditionalSleep([this, config, connectingStartedTime]() { return InitializeTick(config, connectingStartedTime); }, 1);
	}
	else if (phase == ELifecyclePhase::INITIALIZATION) {
		if (ConnectionState != EApConnectionState::Connected) {
			FApConfigurationStruct config = GetActiveConfig();

			FString message = FString::Printf(TEXT("Failed to connect to Archipelago server: \"%s\", for user \"%s\""), *config.Url, *config.Login);

			ChatMessageQueue.Enqueue(TPair<FString, FLinearColor>(message, FLinearColor::Red));
		}
	}
}

bool AApSubsystem::InitializeTick(FApConfigurationStruct config, FDateTime connectingStartedTime) {
	if (ConnectionState == EApConnectionState::Connecting) {
		if ((FDateTime::Now() - connectingStartedTime).GetSeconds() > 5)
			TimeoutConnectionIfNotConnected();
		else
			CheckConnectionState(config);
	} else if (ConnectionState == EApConnectionState::Connected) {
		if (!shouldParseItemsToScout) {
			if (firstHubLocation != 0 && lastHubLocation != 0) {
				HintUnlockedHubRecipies();
			} else {
				// TODO this is where it gets hung up after reloading a save
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
	AP_RegisterSlotDataIntCallback("FirstHubLocation", AApSubsystem::SlotDataFirstHubLocation);
	AP_RegisterSlotDataIntCallback("LastHubLocation", AApSubsystem::SlotDataLastHubLocation);

	ConnectionState = EApConnectionState::Connecting;
	ConnectionStateDescription = LOCTEXT("Connecting", "Connecting...");

	AP_Start();
}

void AApSubsystem::OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubSystem::OnResearchCompleted(schematic), Mam Research Completed"));

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

void AApSubsystem::SlotDataFirstHubLocation(int locationId) {
	AApSubsystem* self = AApSubsystem::Get();

	self->firstHubLocation = locationId;
}

void AApSubsystem::SlotDataLastHubLocation(int locationId) {
	AApSubsystem* self = AApSubsystem::Get();

	self->lastHubLocation = locationId;
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
void AApSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ConnectionState != EApConnectionState::Connected)
		return;

	int64_t item;
	while (ReceivedItems.Dequeue(item)) {
		if (ItemSchematics.Contains(item))
			SManager->GiveAccessToSchematic(ItemSchematics[item], nullptr);
	}

	HandleAPMessages();
}

void AApSubsystem::CheckConnectionState(FApConfigurationStruct config) {
	if (ConnectionState == EApConnectionState::Connecting) {
		AP_ConnectionStatus status = AP_GetConnectionStatus();

		if (status == AP_ConnectionStatus::Authenticated) {
			ConnectionState = EApConnectionState::Connected;
			ConnectionStateDescription = LOCTEXT("AuthSuccess", "Authentication succeeded.");
			UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::Tick(), Successfully Authenticated"));
		}
		else if (status == AP_ConnectionStatus::ConnectionRefused) {
			ConnectionState = EApConnectionState::ConnectionFailed;
			ConnectionStateDescription = LOCTEXT("ConnectionRefused", "Connection refused by server. Check your connection details and load the save again.");

			FString message = FString::Printf(TEXT("Failed to connect to Archipelago server: \"%s\", for user \"%s\""), *config.Url, *config.Login);

			ChatMessageQueue.Enqueue(TPair<FString, FLinearColor>(message, FLinearColor::Red));
		}
	}
}

void AApSubsystem::ParseScoutedItems() {
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::ParseScoutedItems(vector[%i])"), scoutedLocations.Num());

	TMap<FString, TSubclassOf<UFGSchematic>> schematicsPerMilestone = TMap<FString, TSubclassOf<UFGSchematic>>();

	for (auto& location : scoutedLocations) {
		if (location.locationName.starts_with("Hub"))
		{
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

	for (auto& itemPerMilestone : locationsPerMileStone) {
		FString schematicName;
		for (auto schematicAndName : schematicsPerMilestone)
		{
			if (itemPerMilestone.Key == schematicAndName.Value)
			{
				schematicName = schematicAndName.Key;
				break;
			}
		}

		CreateHubSchematic(schematicName, itemPerMilestone.Key, itemPerMilestone.Value);
	}

	scoutedLocations.Empty();
	shouldParseItemsToScout = false;
}

void AApSubsystem::CreateSchematicBoundToItemId(int64_t item) {
	if (!ItemIdToGameSchematic.Contains(item))
		return;
	
	FString name = UApUtils::FStr("AP_ITEM_SCHEMATIC_" + std::to_string(item));
	// https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_Schematic.json
	FString json = FString::Printf(TEXT(R"({
		"Name": "%s",
		"Type": "Custom",
		"Recipes": [ "%s" ]
	})"), *name, *ItemIdToGameSchematic[item]);

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

void AApSubsystem::CreateHubSchematic(FString name, TSubclassOf<UFGSchematic> factorySchematic, TArray<AP_NetworkItem> items) {
	int delimeterPos;
	name.FindChar('-', delimeterPos);
	
	FString tier = name.Mid(delimeterPos - 1, 1);
	// https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_Schematic.json
	FString json = FString::Printf(TEXT(R"({
		"Name": "%s",
		"Type": "Milestone",
		"Time": 1,
		"Tier": %s,
		"VisualKit": "Kit_AP_Logo",
		"Cost": [
			{
				"Item": "Desc_CopperSheet",
				"Amount": 1
			},
		]
	})"), *name, *tier);

	FContentLib_Schematic schematic = UCLSchematicBPFLib::GenerateCLSchematicFromString(json);
	for (auto& item : items) {
		FContentLib_UnlockInfoOnly infoCard;
		FFormatNamedArguments Args;
		Args.Add(TEXT("ApPlayerName"), UApUtils::FText(item.playerName));
		Args.Add(TEXT("ApItemName"), UApUtils::FText(item.itemName));
		Args.Add(TEXT("ProgressionType"), LOCTEXT("NetworkItemProgressionType", "SOMETHING"));
		infoCard.mUnlockName = FText::Format(LOCTEXT("NetworkItemUnlockDisplayName", "{ApPlayerName}'s {ApItemName}"), Args);
		infoCard.mUnlockDescription = FText::Format(LOCTEXT("NetworkItemUnlockDescription", "This will unlock {ApPlayerName}'s {ApItemName} which is considered a {ProgressionType} advancement."), Args);

		const auto icon = FString(TEXT("/Archipelago/Assets/ArchipelagoIcon128.ArchipelagoIcon128"));
		infoCard.BigIcon = infoCard.SmallIcon = icon;
		infoCard.CategoryIcon = FString(TEXT("/Archipelago/Assets/ArchipelagoIconWhite128.ArchipelagoIconWhite128"));
		schematic.InfoCards.Add(infoCard);
	}
	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, contentLibSubsystem);
	contentRegistry->RegisterSchematic(FName(TEXT("Archipelago")), factorySchematic);
}

void AApSubsystem::HandleAPMessages() {
	for (int i = 0; i < 10; i++)
	{
		TPair<FString, FLinearColor> queuedMessage;
		if (ChatMessageQueue.Dequeue(queuedMessage)) {
			SendChatMessage(queuedMessage.Key, queuedMessage.Value);
		}
		else {
			if (!AP_IsMessagePending())
				return;

			AP_Message* message = AP_GetLatestMessage();
			SendChatMessage(UApUtils::FStr(message->text), FLinearColor::White);

			AP_ClearLatestMessage();
		}
	}
}

void AApSubsystem::SendChatMessage(const FString& Message, const FLinearColor& Color) {
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

	for (int64_t i = firstHubLocation; i <= lastHubLocation; i++)
		locations.push_back(i);

	AP_SendLocationScouts(locations, 0);
}

void AApSubsystem::TimeoutConnectionIfNotConnected() {
	if (ConnectionState != EApConnectionState::Connecting)
		return;

	ConnectionState = EApConnectionState::ConnectionFailed;
	ConnectionStateDescription = LOCTEXT("AuthFailed", "Authentication failed. Check your connection details and load the save again.");
	UE_LOG(LogApSubsystem, Display, TEXT("AApSubsystem::TimeoutConnectionIfNotConnected(), Authenticated Failed"));

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
