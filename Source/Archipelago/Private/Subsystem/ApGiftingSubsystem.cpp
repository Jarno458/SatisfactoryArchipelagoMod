#include "ApGiftingSubsystem.h"
#include "Data/ApMappings.h"

DEFINE_LOG_CATEGORY(LogApGiftingSubsystem);

//TODO REMOVE
#pragma optimize("", off)

const TMap<FString, FString> AApGiftingSubsystem::TraitParents = {
	{"Electronics", "Material"},
	{"Iron", "Metal"},
	{"Silver", "Metal"},
	{"Copper", "Metal"},
	{"Gold", "Metal"},
	{"Metal", "Material"},
	{"Speed", "Buff"},
	{"Grass", "Material"},
	{"Bomb", "Weapon"},
	{"Stone", "Material"},
	{"Ore", "Mineral"},
	{"Damage", "Trap"},
	{"Wood", "Material"},
	{"Vegetable", "Food"},
	{"Seed", "Food"},
	{"Fruit", "Food"},
	{"Food", "Consumable"},
};

const TMap<FString, int64_t> AApGiftingSubsystem::TraitDefaultItemIds = {
	{"Electronics", 1338001}, // Desc_CircuitBoardHighSpeed_C, //AI Limiter
	{"Iron", 1338051}, // Desc_IronPlate_C,
	{"Silver", 1338002}, // Desc_AluminumPlate_C
	{"Copper", 1338030}, // Desc_CopperSheet_C
	{"Gold", 1338018}, // Desc_GoldIngot_C, //Caterium Ingot
	{"Steel", 1338102},  // Desc_SteelPlate_C
	{"Speed", 1338003}, // Desc_Crystal_C, //Blue Power Slug
	{"Grass", 1338054}, //Desc_Leaves_C
	{"Resource", 1338110}, // Desc_HogParts_C
	{"Tool", 1338173}, // BP_EquipmentDescriptorObjectScanner_C
	{"Bomb", 1338097}, // Desc_GunpowderMK2_C, //Smokeless Powder
	{"Ore", 1338021}, // Desc_Coal_C
	{"Flower", 1338041}, // Desc_FlowerPetals_C,
	{"Stone", 1338025}, // Desc_Cement_C, //Concrete
	{"Material", 1338081}, // Desc_PolymerResin_C,
	{"Radioactive", 1338038}, // Desc_UraniumCell_C,
	{"Damage", 1338038}, // Desc_UraniumCell_C,
	{"Crystal", 1338086}, // Desc_QuartzCrystal_C,
	{"Artifact", 1338057}, // Desc_WAT2_C, //Mercer Sphere
	{"Mineral", 1338089}, // Desc_RawQuartz_C
	{"Wood", 1338116}, // Desc_Wood_C, 
	{"Vegetable", 1338150}, // Desc_Shroom_C, 
	{"Seed", 1338151}, // Desc_Nut_C, 
	{"Armor", 1338164}, // BP_EquipmentDescriptorHazmatSuit_C,
	{"Slowness", 1338175}, // Desc_Parachute_C, 
	{"Fiber", 1338061}, // Desc_Mycelia_C, 
	{"Coal", 1338021}, // Desc_Coal_C, 
};

const TMap<int64_t, TMap<FString, float>> AApGiftingSubsystem::TraitsPerItem = {
	{1338000, {{"Electronics", 1.0f}}}, // Desc_SpaceElevatorPart_5_C, //Adaptive Control Unit
	{1338001, {{"Electronics", 1.0f}}}, // Desc_CircuitBoardHighSpeed_C, //AI Limiter
	{1338002, {{"Silver", 1.0f}}}, // Desc_AluminumPlate_C, 
	{1338003, {{"Speed", 1.0f}}}, // Desc_Crystal_C, //Blue Power Slug
	{1338004, {{"Speed", 1.0f}}}, // Desc_Crystal_mk2_C, //Yellow Power Slug
	{1338005, {{"Grass", 1.0f},{"Resource", 0.1f}}}, // Desc_AlienProtein_C, 
	{1338006, {{"Speed", 1.0f}}}, // Desc_Crystal_mk3_C, //Purple Power Slug
	{1338007, {{"Silver", 1.0f}}}, // Desc_AluminumCasing_C, 
	{1338008, {{"Silver", 1.0f}}}, // Desc_AluminumIngot_C, 
	{1338009, {{"Silver", 1.0f}}}, // Desc_AluminumScrap_C, 
	{1338010, {{"Electronics", 1.0f}}}, // Desc_SpaceElevatorPart_7_C, //Assembly Director System
	{1338011, {{"Copper", 1.0f}}}, // Desc_SpaceElevatorPart_3_C, //Automated Wiring
	{1338012, {{"Copper", 1.0f}}}, // Desc_Battery_C, 
	{1338013, {{"Silver", 1.0f},{"Ore", 1.0f}}}, // Desc_OreBauxite_C, 
	{1338014, {{"Tool", 1.0f}}}, // BP_EquipmentDescriptorBeacon_C, //Beacon
	{1338015, {{"Grass", 1.0f}}}, // Desc_GenericBiomass_C, 
	{1338016, {{"Bomb", 1.0f}}}, // Desc_Gunpowder_C, //Black Powder
	{1338017, {{"Copper", 1.0f}}}, // Desc_Cable_C, 
	{1338018, {{"Gold", 1.0f}}}, // Desc_GoldIngot_C, //Caterium Ingot
	{1338019, {{"Gold", 1.0f},{"Ore", 1.0f}}}, // Desc_OreGold_C, //Caterium Ore
	{1338020, {{"Electronics", 1.0f}}}, // Desc_CircuitBoard_C, 
	{1338021, {{"Coal", 1.0f},{"Ore", 1.0f}}}, // Desc_Coal_C, 
	{1338022, {{"Flower", 2.0f}}}, // Desc_ColorCartridge_C, 
	{1338023, {{"Coal", 1.0f}}}, // Desc_CompactedCoal_C, 
	{1338024, {{"Electronics", 1.0f}}}, // Desc_Computer_C, 
	{1338025, {{"Stone", 1.0f},{"Ore", 1.0f}}}, // Desc_Cement_C, //Concrete
	{1338026, {{"Silver", 1.0f}}}, // Desc_CoolingSystem_C, 
	{1338027, {{"Copper", 1.0f}}}, // Desc_CopperIngot_C, 
	{1338028, {{"Copper", 1.0f}}}, // Desc_OreCopper_C, //Copper Ore
	{1338029, {{"Copper", 1.0f}}}, // Desc_CopperDust_C, 
	{1338030, {{"Copper", 1.0f}}}, // Desc_CopperSheet_C, 
	{1338031, {{"Copper", 10.0f}}}, // Desc_CharacterRunStatue_C, 
	{1338032, {{"Crystal", 1.0f}}}, // Desc_CrystalOscillator_C, 
	{1338033, {{"Copper", 1.0f}}}, // Desc_ElectromagneticControlRod_C, 
	{1338034, {{"Material", 1.0f}}}, // Desc_FluidCanister_C, 
	{1338035, {{"Silver", 1.0f}}}, // Desc_GasTank_C, //Empty Fluid Tank
	{1338036, {{"Steel", 0.5f},{"Stone", 0.5f}}}, // Desc_SteelPlateReinforced_C, //Encased Industrial Beam
	{1338037, {{"Stone", 1.0f},{"Radioactive", 12.0f},{"Damage", 12.0f}}}, // Desc_PlutoniumCell_C, 
	{1338038, {{"Radioactive",	0.5f},{"Damage", 0.05f}}}, // Desc_UraniumCell_C, 
	{1338039, {{"Fiber", 1.0f}}}, // Desc_Fabric_C, 
	{1338040, {{"Resource", 1.0f}}}, // Desc_ResourceSinkCoupon_C, 
	{1338041, {{"Flower", 1.0f}}}, // Desc_FlowerPetals_C, 
	{1338042, {{"Silver", 1.0f}}}, // Desc_ModularFrameFused_C, 
	{1338043, {{"Electronics", 1.0f}}}, // Desc_HardDrive_C, 
	{1338044, {{"Silver", 1.0f}}}, // Desc_AluminumPlateReinforced_C, //Heatsink
	{1338045, {{"Steel", 1.0f}}}, // Desc_ModularFrameHeavy_C, 
	{1338046, {{"Electronics", 1.0f}}}, // Desc_HighSpeedConnector_C, 
	{1338047, {{"Gold", 10.0f}}}, // Desc_CharacterSpin_Statue_C, 
	{1338048, {{"Silver", 10.0f}}}, // Desc_CharacterClap_Statue_C, 
	{1338049, {{"Iron", 1.0f}}}, // Desc_IronIngot_C, 
	{1338050, {{"Iron", 1.0f},{"Ore", 1.0f}}}, // Desc_OreIron_C, 
	{1338051, {{"Iron", 1.0f}}}, // Desc_IronPlate_C, 
	{1338052, {{"Iron", 1.0f}}}, // Desc_IronRod_C, 
	{1338053, {{"Gold", 1000.0f}}}, // Desc_GoldenNut_Statue_C, 
	{1338054, {{"Grass", 1.0f}}}, // Desc_Leaves_C, 
	{1338055, {{"Stone", 1.0f},{"Ore", 1.0f}}}, // Desc_Stone_C, //Limestone
	{1338056, {{"Copper", 0.4f},{"Iron", 0.3f},{"Steel",0.3f}}}, // Desc_SpaceElevatorPart_6_C, //Magnetic Field Generator
	{1338057, {{"Artifact",1.0f}}}, // Desc_WAT2_C, //Mercer Sphere
	{1338058, {{"Copper", 1.0f}}}, // Desc_SpaceElevatorPart_4_C, //Modular Engine
	{1338059, {{"Iron", 1.0f}}}, // Desc_ModularFrame_C, 
	{1338060, {{"Copper", 1.0f}}}, // Desc_Motor_C, 
	{1338061, {{"Fiber", 1.0f}}}, // Desc_Mycelia_C, 
	{1338062, {{"Radioactive", 0.075f},{"Damage", 0.075f}}}, // Desc_NonFissibleUranium_C, 
	{1338063, {{"Silver", 0.3f},{"Electronics", 0.4f},{"Copper", 0.3f}}}, // Desc_SpaceElevatorPart_9_C, //Nuclear Pasta
	{1338064, {{"Gold", 100.0f}}}, // Desc_DoggoStatue_C, 
	{1338065, {{"Resource", 1.0f}}}, // Desc_AlienDNACapsule_C, 
	{1338066, {{"Silver", 1.0f}}}, // Desc_PackagedAlumina_C, 
	{1338067, {{"Material", 1.0f}}}, // Desc_Fuel_C, 
	{1338068, {{"Material", 1.0f}}}, // Desc_PackagedOilResidue_C, 
	{1338069, {{"Grass", 1.0f}}}, // Desc_PackagedBiofuel_C, 
	{1338070, {{"Material", 1.0f}}}, // Desc_PackagedNitricAcid_C, 
	{1338071, {{"Material", 1.0f}}}, // Desc_PackagedNitrogenGas_C, 
	{1338072, {{"Material", 1.0f}}}, // Desc_PackagedOil_C, 
	{1338073, {{"Material", 1.0f}}}, // Desc_PackagedSulfuricAcid_C, 
	{1338074, {{"Material", 1.0f},{"Speed", 1.0f}}}, // Desc_TurboFuel_C, //Packaged Turno Fuel
	{1338075, {{"Material", 1.0f},{"Drink", 1.0f}}}, // Desc_PackagedWater_C, 
	{1338076, {{"Steel", 0.1f}}}, // Desc_PetroleumCoke_C, 
	{1338077, {{"Material", 1.0f}}}, // Desc_Plastic_C, 
	{1338078, {{"Stone", 1.0f},{"Radioactive", 25.0f},{"Damage", 25.0f}}}, // Desc_PlutoniumFuelRod_C, 
	{1338079, {{"Radioactive", 2.0f},{"Damage", 2.0f}}}, // Desc_PlutoniumPellet_C, 
	{1338080, {{"Stone", 1.0f},{"Radioactive", 20.0f},{"Damage", 20.0f}}}, // Desc_PlutoniumWaste_C, 
	{1338081, {{"Material", 1.0f}}}, // Desc_PolymerResin_C, 
	{1338082, {{"Artifact",1.0f},{"Speed",1.0f}}}, // Desc_CrystalShard_C, //Power Shard
	{1338083, {{"Gold", 200.0f}}}, // Desc_SpaceGiraffeStatue_C, 
	{1338084, {{"Silver", 0.4f},{"Electronics", 0.6f}}}, // Desc_PressureConversionCube_C, 
	{1338085, {{"Electronics", 1.0f}}}, // Desc_ComputerQuantum_C, 
	{1338086, {{"Crystal", 1.0f}}}, // Desc_QuartzCrystal_C, 
	{1338087, {{"Copper", 1.0f}}}, // Desc_HighSpeedWire_C, 
	{1338088, {{"Electronics", 1.0f}}}, // Desc_ModularFrameLightweight_C, //Radio Control Unit
	{1338089, {{"Crystal", 1.0f},{"Mineral",1.0f}}}, // Desc_RawQuartz_C, 
	{1338090, {{"Iron", 1.0f}}}, // Desc_IronPlateReinforced_C, 
	{1338091, {{"Copper", 1.0f}}}, // Desc_Rotor_C, 
	{1338092, {{"Material", 1.0f}}}, // Desc_Rubber_C, 
	{1338093, {{ }}}, // Desc_SAM_C, 
	{1338094, {{"Iron", 1.0f}}}, // Desc_IronScrew_C, 
	{1338095, {{"Crystal", 1.0f}}}, // Desc_Silica_C, 
	{1338096, {{"Iron", 0.5f},{"Copper", 0.5f}}}, // Desc_SpaceElevatorPart_1_C, //Smart Plating
	{1338097, {{"Bomb", 1.0f}}}, // Desc_GunpowderMK2_C, //Smokeless Powder
	{1338098, {{"Grass", 1.0f}}}, // Desc_Biofuel_C, //Solid Biofuel
	{1338099, {{"Artifact",1.0f}}}, // Desc_WAT1_C, //Somersloop
	{1338100, {{"Copper", 1.0f}}}, // Desc_Stator_C, 
	{1338101, {{"Silver", 50.0f}}}, // Desc_Hog_Statue_C, 
	{1338102, {{"Steel", 1.0f}}}, // Desc_SteelPlate_C, 
	{1338103, {{"Steel", 1.0f}}}, // Desc_SteelIngot_C, 
	{1338104, {{"Steel", 1.0f}}}, // Desc_SteelPipe_C, 
	{1338105, {{"Bomb", 1.0f}}}, // Desc_Sulfur_C, 
	{1338106, {{"Electronics", 1.0f}}}, // Desc_ComputerSuper_C, 
	{1338107, {{"Electronics", 1.0f}}}, // Desc_QuantumOscillator_C, 
	{1338108, {{"Silver", 0.3f},{"Electronics", 0.3f},{"Steel", 0.4f},{"Speed", 1.0f}}}, // Desc_SpaceElevatorPart_8_C, //Thermal Propulsion Rocket
	{1338109, {{"Electronics", 0.5f},{"Steel", 0.5f}}}, // Desc_MotorLightweight_C, 
	{1338110, {{"Grass", 1.0f},{"Resource", 0.1f}}}, // Desc_HogParts_C, 
	{1338111, {{"Radioactive", 1.5f},{"Damage", 1.5f},{"Ore", 1.0f}}}, // Desc_OreUranium_C, 
	{1338112, {{"Stone", 0.5f},{"Steel", 0.5f},{"Radioactive", 5.0f},{"Damage", 5.0f}}}, // Desc_NuclearFuelRod_C, 
	{1338113, {{"Radioactive", 1.0f},{"Damage", 1.0f}}}, // Desc_NuclearWaste_C, 
	{1338114, {{"Steel", 0.5f},{"Iron", 0.5f}}}, // Desc_SpaceElevatorPart_2_C, //Versatile Framework
	{1338115, {{"Copper", 1.0f}}}, // Desc_Wire_C, 
	{1338116, {{"Wood", 1.0}}}, // Desc_Wood_C, 
	{1338117, {{"Grass", 1.0f},{"Resource", 0.1f}}}, // Desc_SpitterParts_C, 
	{1338118, {{"Grass", 1.0f},{"Resource", 0.1f}}}, // Desc_StingerParts_C, 
	{1338119, {{"Grass", 1.0f},{"Resource", 0.1f}}}, // Desc_HatcherParts_C, 

	//Enquipment/Ammo
	{1338150, {{"Vegetable", 1.0f},{"Heal", 1.0f}}}, // Desc_Shroom_C, //Bacon Agaric
	{1338151, {{"Seed", 1.0f},{"Heal", 1.0f}}}, // Desc_Nut_C, 
	{1338152, {{"Tool", 1.0f},{"Speed", 1.0f}}}, // BP_EquipmentDescriptorJumpingStilts_C, 
	{1338153, {{"Weapon", 1.0f},{"Buff", 1.0f}}}, //BoomBox
	{1338154, {{"Weapon", 1.0f}}}, // Desc_Chainsaw_C, 
	{1338155, {{"Bomb", 1.0f}}}, // Desc_NobeliskCluster_C, 
	//{1338156, {{ }}}, // Unused, 
	{1338157, {{"Speed", 1.0f}}}, // BP_EquipmentDescriptorCup_C, 
	{1338158, {{"Speed", 1.0f},{"Gold", 1.0f}}}, // BP_EquipmentDescriptorCupGold_C, 
	{1338159, {{"Weapon", 1.0f}}}, // Desc_Rebar_Explosive_C, 
	{1338160, {{"Tool", 1.0f},{"Speed", 1.0f}}}, // Desc_GolfCart_C, 
	{1338161, {{"Tool", 1.0f},{"Speed", 1.0f}}}, // Desc_GolfCartGold_C, 
	{1338162, {{"Tool", 1.0f}}}, // BP_EquipmentDescriptorGasmask_C, 
	{1338163, {{"Bomb", 1.0f}}}, // Desc_NobeliskGas_C, 
	{1338164, {{"Armor", 1.0f}}}, // BP_EquipmentDescriptorHazmatSuit_C, 
	{1338165, {{"Weapon", 1.0f}}}, // Desc_CartridgeSmartProjectile_C, 
	{1338166, {{"Tool", 1.0f},{"Speed", 1.0f}}}, // BP_EquipmentDescriptorHoverPack_C, 
	{1338167, {{"Weapon", 1.0f}}}, // Desc_SpikedRebar_C, 
	{1338168, {{"Tool", 1.0f},{"Speed", 1.0f}}}, // BP_EquipmentDescriptorJetPack_C, 
	{1338169, {{"Heal", 1.0f}}}, // Desc_Medkit_C, 
	{1338170, {{"Bomb", 1.0f}}}, // Desc_NobeliskExplosive_C, 
	{1338171, {{"Bomb", 1.0f}}}, // BP_EquipmentDescriptorNobeliskDetonator_C, 
	{1338172, {{"Bomb", 1.0f}}}, // Desc_NobeliskNuke_C, 
	{1338173, {{"Tool", 1.0f}}}, // BP_EquipmentDescriptorObjectScanner_C, 
	{1338174, {{"Fruit", 1.0f},{"Heal", 1.0f}}}, // Desc_Berry_C, 
	{1338175, {{"Tool", 1.0f}, {"Slowness", 1.0f}}}, // Desc_Parachute_C, 
	{1338176, {{"Bomb", 1.0f}}}, // Desc_NobeliskShockwave_C, 
	{1338177, {{"Weapon", 1.0f}}}, // Desc_RebarGunProjectile_C, 
	{1338178, {{"Weapon", 1.0f}}}, // BP_EquipmentDescriptorRifle_C, 
	{1338179, {{"Weapon", 1.0f}}}, // Desc_CartridgeStandard_C, 
	{1338180, {{"Weapon", 1.0f}}}, // Desc_Rebar_Spreadshot_C, 
	{1338181, {{"Weapon", 1.0f}}}, // Desc_Rebar_Stunshot_C, 
	{1338182, {{"Weapon", 1.0f}}}, // Desc_CartridgeChaos_C, 
	{1338183, {{"Weapon", 1.0f}}}, // BP_EquipmentDescriptorStunSpear_C, 
	{1338184, {{"Weapon", 1.0f}}}, // BP_EquipmentDescriptorShockShank_C, 
	{1338185, {{"Tool", 1.0f},{"Speed", 1.0f}}}, // BP_EqDescZipLine_C, 
	{1338186, {{"Tool", 1.0f}}}, // BP_ItemDescriptorPortableMiner_C,
};

// The Unsinkables
const TMap<int64_t, int> AApGiftingSubsystem::HardcodedSinkValues = {
	{1338003, 10}, // Desc_Crystal_C, //Blue Power Slug
	{1338004, 30}, // Desc_Crystal_mk2_C, //Yellow Power Slug
	{1338006, 100}, // Desc_Crystal_mk3_C, //Purple Power Slug
	{1338082, 50}, // Desc_CrystalShard_C, //Power Shard

	{1338110, 700}, // Desc_HogParts_C,
	{1338119, 750}, // Desc_HatcherParts_C,
	{1338117, 800}, // Desc_SpitterParts_C,
	{1338118, 900}, // Desc_StingerParts_C,
	{1338005, 1000}, // Desc_AlienProtein_C, //High value as 1 Protein = 100 biomass = 12 * 100 = 1200 points
	{1338120, 14}, // Desc_AlienDNACapsule_C,

	{1338043, 900}, //Desc_HardDrive_C
	{1338099, 1000}, //Desc_WAT1_C, //Somersloop
	{1338057, 1050}, //Desc_WAT2_C, //Mercer Sphere

	// Uranium = 35
	//Encased Uranium Cell = 147
	{1338113, 40000}, // Desc_NuclearWaste_C,
	// Uranium Fuel Rod = 44092
	{1338062, 60000}, // Desc_NonFissibleUranium_C,
	{1338079, 80000}, // Desc_PlutoniumPellet_C, 
	{1338037, 100000}, // Desc_PlutoniumCell_C,
	{1338080, 125000}, // Desc_PlutoniumWaste_C, 
	// Plutonium Fuel Rod = 153184

	//Cups, Statues and the Boom Box
	{1338031, 25}, // Desc_CharacterRunStatue_C, 
	{1338101, 50}, // Desc_Hog_Statue_C, 
	{1338048, 51}, // Desc_CharacterClap_Statue_C, 
	{1338064, 100}, // Desc_DoggoStatue_C, 
	{1338047, 150}, // Desc_CharacterSpin_Statue_C, 
	{1338083, 200}, // Desc_SpaceGiraffeStatue_C, 
	{1338053, 1000}, // Desc_GoldenNut_Statue_C, 

	{1338157, 1}, // BP_EquipmentDescriptorCup_C, 
	{1338158, 10000}, // BP_EquipmentDescriptorCupGold_C, 

	{1338153, 10}, //BoomBox

	{1338151, 5}, // Desc_Nut_C, 
	{1338174, 12}, // Desc_Berry_C, 
	{1338150, 30}, // Desc_Shroom_C, //Bacon Agaric
	// Medicinal Inhaler = 125

	{1338085, 300000}, // Desc_ComputerQuantum_C, 
	{1338107, 400000}, // Desc_QuantumOscillator_C,
};


AApGiftingSubsystem* AApGiftingSubsystem::Get() {
	return Get(GEngine->GameViewport->GetWorld());
}

AApGiftingSubsystem* AApGiftingSubsystem::Get(class UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	check(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AApGiftingSubsystem>();
}

AApGiftingSubsystem::AApGiftingSubsystem() : Super() {
	UE_LOG(LogApGiftingSubsystem, Display, TEXT("AApGiftingSubsystem::AApGiftingSubsystem()"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.1f;
}

void AApGiftingSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogApGiftingSubsystem, Display, TEXT("AApGiftingSubsystem::BeginPlay()"));

	UWorld* world = GetWorld();
	ap = AApSubsystem::Get(world);
	portalSubSystem = AApPortalSubsystem::Get(world);
	resourceSinkSubsystem = AFGResourceSinkSubsystem::Get(world);
	mappingSubsystem = AApMappingsSubsystem::Get(world);
}

void AApGiftingSubsystem::Tick(float dt) {
	Super::Tick(dt);

	if (!apInitialized) {
		if (ap->ConnectionState == EApConnectionState::Connected && mappingSubsystem->IsInitialized()) {
			LoadItemNameMapping();

			ap->SetGiftBoxState(true);

			apInitialized = true;
		}
	} else {
		FDateTime currentTime = FDateTime::Now();

		if ((currentTime - lastPoll).GetSeconds() >= pollInterfall)
		{
			lastPoll = currentTime;

			ProcessInputQueue();
			PullAllGiftsAsync();
		}
	}
}

void AApGiftingSubsystem::LoadItemNameMapping() {
	for (TPair<int64_t, FApItemInfo> itemInfoMapping : mappingSubsystem->ItemInfo) {
		ItemToItemId.Add(itemInfoMapping.Value.Class, itemInfoMapping.Key);
	}

	for (TPair<FString, int64_t> traitDefault : TraitDefaultItemIds) {
		TraitAvarageValue.Add(traitDefault.Key, GetResourceSinkPointsForItem(mappingSubsystem->ItemInfo[traitDefault.Value].Class, traitDefault.Value));
	}
}

void AApGiftingSubsystem::PullAllGiftsAsync() {
	TArray<FApReceiveGift> gifts = ap->GetGifts();

	UpdatedProcessedIds(gifts);

	for (FApReceiveGift gift : gifts) {
		if (ProcessedIds.Contains(gift.Id))
			continue;

		ProcessedIds.Add(gift.Id);

		//try match on name
		if (mappingSubsystem->NameToItemId.Contains(gift.ItemName)) {
			portalSubSystem->Enqueue(mappingSubsystem->ItemInfo[mappingSubsystem->NameToItemId[gift.ItemName]].Class, gift.Amount);
		}	else {
			//if name cant be matched, try match on traits
			TSubclassOf<UFGItemDescriptor> itemClass = TryGetItemClassByTraits(gift.Traits);

			if (itemClass != nullptr) {
				portalSubSystem->Enqueue(itemClass, gift.Amount);
			} else {
				ap->RejectGift(gift.Id);
			}
		}

		ap->AcceptGift(gift.Id);
	}
}

void AApGiftingSubsystem::UpdatedProcessedIds(TArray<FApReceiveGift> gifts) {
	TSet<FString> currentGiftIds;
	for (FApReceiveGift gift : gifts) {
		currentGiftIds.Add(gift.Id);
	}

	TArray<FString> giftIdsToForget;
	for (FString id : ProcessedIds) {
		if (!currentGiftIds.Contains(id))
			giftIdsToForget.Add(id);
	}

	for (FString id : giftIdsToForget) {
		ProcessedIds.Remove(id);
	}
}

void AApGiftingSubsystem::EnqueueForSending(FApPlayer targetPlayer, FInventoryStack itemStack) {
	if (!InputQueue.Contains(targetPlayer)) {
		TSharedPtr<TQueue<FInventoryStack, EQueueMode::Mpsc>> queue = MakeShareable(new TQueue<FInventoryStack, EQueueMode::Mpsc>());
		InputQueue.Add(targetPlayer, queue);
	}

	InputQueue[targetPlayer]->Enqueue(itemStack);
}

void AApGiftingSubsystem::ProcessInputQueue() {
	TMap<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend;

	for (TPair<FApPlayer, TSharedPtr<TQueue<FInventoryStack, EQueueMode::Mpsc>>> stacksPerPlayer : InputQueue) {
		if (!itemsToSend.Contains(stacksPerPlayer.Key))
			itemsToSend.Add(stacksPerPlayer.Key, TMap<TSubclassOf<UFGItemDescriptor>, int>());

		FInventoryStack stack;
		while (stacksPerPlayer.Value->Dequeue(stack))
		{
			TSubclassOf<UFGItemDescriptor> cls = stack.Item.GetItemClass();

			if (!itemsToSend[stacksPerPlayer.Key].Contains(cls))
				itemsToSend[stacksPerPlayer.Key].Add(cls, stack.NumItems);
			else
				itemsToSend[stacksPerPlayer.Key][cls] += stack.NumItems;
		}
	}

	if (!itemsToSend.IsEmpty())
		Send(itemsToSend);
}

void AApGiftingSubsystem::Send(TMap<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSend) {
	for (TPair<FApPlayer, TMap<TSubclassOf<UFGItemDescriptor>, int>> itemsToSendPerPlayer : itemsToSend) {
		if (itemsToSendPerPlayer.Value.Num() <= 0)
			continue;

		for (TPair<TSubclassOf<UFGItemDescriptor>, int> stack : itemsToSendPerPlayer.Value) {
			FApSendGift gift;

			if (!ItemToItemId.Contains(stack.Key)) {
				gift.ItemName = UFGItemDescriptor::GetItemName(stack.Key).ToString();
				gift.Amount = stack.Value;
				gift.ItemValue = 0;
				gift.Traits = TArray<FApGiftTrait>();
				gift.Receiver = itemsToSendPerPlayer.Key;
			} else {
				int64_t itemId = ItemToItemId[stack.Key];
				FString itemName = mappingSubsystem->ItemInfo[itemId].Name;
				int itemValue = GetResourceSinkPointsForItem(stack.Key, itemId);

				gift.ItemName = mappingSubsystem->ItemInfo[itemId].Name;
				gift.Amount = stack.Value;
				gift.ItemValue = itemValue;
				gift.Traits = GetTraitsForItem(itemId, itemValue);
				gift.Receiver = itemsToSendPerPlayer.Key;
			}

			ap->SendGift(gift);
		}
	}
}

TSubclassOf<UFGItemDescriptor> AApGiftingSubsystem::TryGetItemClassByTraits(TArray<FApGiftTrait> traits) {
	//TODO process item traits and quality, like "Metal" with quality 2 might be Encased Steam Beam
	return nullptr;
}

TArray<FApGiftTrait> AApGiftingSubsystem::GetTraitsForItem(int64_t itemId, int itemValue) {
	if (!TraitsPerItem.Contains(itemId))
		return TArray<FApGiftTrait>();

	TMap<FString, FApGiftTrait> Traits;

	for (TPair<FString, float> trait : TraitsPerItem[itemId]) {
		int traitDefualt;
		if (TraitAvarageValue.Contains(trait.Key))
			traitDefualt = TraitAvarageValue[trait.Key];
		else
			traitDefualt = 1;

		float traitValue = (0.1f * itemValue / traitDefualt) * trait.Value;

		FApGiftTrait traitSpecification;
		traitSpecification.Trait = trait.Key;
		traitSpecification.Quality = traitValue;
		traitSpecification.Duration = 1.0f;

		Traits.Add(trait.Key, traitSpecification);

		FString traitName = trait.Key;
		while (TraitParents.Contains(traitName)) {
			traitName = TraitParents[traitName];

			if (Traits.Contains(traitName)) {
				Traits[traitName].Quality += traitValue;
			} else {
				FApGiftTrait parentTraitSpecification;
				parentTraitSpecification.Trait = traitName;
				parentTraitSpecification.Quality = traitValue;
				parentTraitSpecification.Duration = 1.0f;

				Traits.Add(traitName, parentTraitSpecification);
			}
		}
	}

	TArray<FApGiftTrait> traitsToReturn;
	Traits.GenerateValueArray(traitsToReturn);
	return traitsToReturn;
}

int AApGiftingSubsystem::GetResourceSinkPointsForItem(TSubclassOf<UFGItemDescriptor> itemClass, int64_t itemId) {
	if (HardcodedSinkValues.Contains(itemId))
		return HardcodedSinkValues[itemId];
	
	int value = resourceSinkSubsystem->GetResourceSinkPointsForItem(itemClass);

	if (value == 0) {
		FString itemName = UFGItemDescriptor::GetItemName(itemClass).ToString();
		UE_LOG(LogApGiftingSubsystem, Error, TEXT("AApGiftingSubsystem::GetResourceSinkPointsForItem(\"%s\", % i) Not sink value for item"), *itemName, itemId);
		value = 1;
	}

	return value;
}

#pragma optimize("", on)