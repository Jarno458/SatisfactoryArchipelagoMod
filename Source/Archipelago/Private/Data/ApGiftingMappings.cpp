#include "Data/ApGiftingMappings.h"

const TMap<FString, int64> UApGiftingMappings::HardcodedItemNameToIdMappings = {
	{"Coffee", 1338157}, // BP_EquipmentDescriptorCup_C, 
};

const TMap<EGiftTrait, EGiftTrait> UApGiftingMappings::TraitParents = {
	{EGiftTrait::Electronics, EGiftTrait::Material},
	{EGiftTrait::Iron, EGiftTrait::Metal},
	{EGiftTrait::Silver, EGiftTrait::Metal},
	{EGiftTrait::Copper, EGiftTrait::Metal},
	{EGiftTrait::Gold, EGiftTrait::Metal},
	{EGiftTrait::Metal, EGiftTrait::Material},
	{EGiftTrait::Speed, EGiftTrait::Buff},
	{EGiftTrait::Grass, EGiftTrait::Material},
	{EGiftTrait::Bomb, EGiftTrait::Weapon},
	{EGiftTrait::Stone, EGiftTrait::Material},
	{EGiftTrait::Ore, EGiftTrait::Mineral},
	{EGiftTrait::Damage, EGiftTrait::Trap},
	{EGiftTrait::Wood, EGiftTrait::Material},
	{EGiftTrait::Vegetable, EGiftTrait::Food},
	{EGiftTrait::Seed, EGiftTrait::Food},
	{EGiftTrait::Fruit, EGiftTrait::Food},
	{EGiftTrait::Food, EGiftTrait::Consumable},
};

const TMap<EGiftTrait, int64> UApGiftingMappings::TraitDefaultItemIds = {
	{EGiftTrait::Electronics, 1338001}, // Desc_CircuitBoardHighSpeed_C, //AI Limiter
	{EGiftTrait::Iron, 1338051}, // Desc_IronPlate_C,
	{EGiftTrait::Silver, 1338002}, // Desc_AluminumPlate_C
	{EGiftTrait::Copper, 1338030}, // Desc_CopperSheet_C
	{EGiftTrait::Gold, 1338018}, // Desc_GoldIngot_C, //Caterium Ingot
	{EGiftTrait::Steel, 1338102},  // Desc_SteelPlate_C
	{EGiftTrait::Speed, 1338003}, // Desc_Crystal_C, //Blue Power Slug
	{EGiftTrait::Grass, 1338054}, //Desc_Leaves_C
	{EGiftTrait::Resource, 1338110}, // Desc_HogParts_C
	{EGiftTrait::Tool, 1338173}, // BP_EquipmentDescriptorObjectScanner_C
	{EGiftTrait::Bomb, 1338097}, // Desc_GunpowderMK2_C, //Smokeless Powder
	{EGiftTrait::Ore, 1338021}, // Desc_Coal_C
	{EGiftTrait::Flower, 1338041}, // Desc_FlowerPetals_C,
	{EGiftTrait::Stone, 1338025}, // Desc_Cement_C, //Concrete
	{EGiftTrait::Material, 1338081}, // Desc_PolymerResin_C,
	{EGiftTrait::Radioactive, 1338038}, // Desc_UraniumCell_C,
	{EGiftTrait::Damage, 1338038}, // Desc_UraniumCell_C,
	{EGiftTrait::Crystal, 1338086}, // Desc_QuartzCrystal_C,
	{EGiftTrait::Artifact, 1338057}, // Desc_WAT2_C, //Mercer Sphere
	{EGiftTrait::Mineral, 1338089}, // Desc_RawQuartz_C
	{EGiftTrait::Wood, 1338116}, // Desc_Wood_C, 
	{EGiftTrait::Vegetable, 1338150}, // Desc_Shroom_C, //Bacon Agaric
	{EGiftTrait::Seed, 1338151}, // Desc_Nut_C, 
	{EGiftTrait::Armor, 1338164}, // BP_EquipmentDescriptorHazmatSuit_C,
	{EGiftTrait::Slowness, 1338175}, // Desc_Parachute_C, 
	{EGiftTrait::Fiber, 1338061}, // Desc_Mycelia_C, 
	{EGiftTrait::Coal, 1338021}, // Desc_Coal_C, 
	{EGiftTrait::Metal, 1338102},  // Desc_SteelPlate_C
	{EGiftTrait::Buff, 1338003}, // Desc_Crystal_C, //Blue Power Slug
	{EGiftTrait::Weapon, 1338154}, // Desc_Chainsaw_C,
	{EGiftTrait::Trap, 1338038}, // Desc_UraniumCell_C,
	{EGiftTrait::Food, 1338151}, // Desc_Nut_C, 
	{EGiftTrait::Consumable, 1338151}, // Desc_Nut_C, 
	{EGiftTrait::Drink, 1338075}, // Desc_PackagedWater_C,
	{EGiftTrait::Vegetable, 1338150}, // Desc_Shroom_C, //Bacon Agaric
	{EGiftTrait::Heal, 1338174}, // Desc_Berry_C
	{EGiftTrait::Fruit, 1338174}, // Desc_Berry_C
};

const TMap<int64, TMap<EGiftTrait, float>> UApGiftingMappings::TraitsPerItemRatings = {
	{1338000, {{EGiftTrait::Electronics, 1.0f}}}, // Desc_SpaceElevatorPart_5_C, //Adaptive Control Unit
	{1338001, {{EGiftTrait::Electronics, 1.0f}}}, // Desc_CircuitBoardHighSpeed_C, //AI Limiter
	{1338002, {{EGiftTrait::Silver, 1.0f}}}, // Desc_AluminumPlate_C, 
	{1338003, {{EGiftTrait::Speed, 1.0f}}}, // Desc_Crystal_C, //Blue Power Slug
	{1338004, {{EGiftTrait::Speed, 1.0f}}}, // Desc_Crystal_mk2_C, //Yellow Power Slug
	{1338005, {{EGiftTrait::Grass, 1.0f},{EGiftTrait::Resource, 0.1f}}}, // Desc_AlienProtein_C, 
	{1338006, {{EGiftTrait::Speed, 1.0f}}}, // Desc_Crystal_mk3_C, //Purple Power Slug
	{1338007, {{EGiftTrait::Silver, 1.0f}}}, // Desc_AluminumCasing_C, 
	{1338008, {{EGiftTrait::Silver, 1.0f}}}, // Desc_AluminumIngot_C, 
	{1338009, {{EGiftTrait::Silver, 1.0f}}}, // Desc_AluminumScrap_C, 
	{1338010, {{EGiftTrait::Electronics, 1.0f}}}, // Desc_SpaceElevatorPart_7_C, //Assembly Director System
	{1338011, {{EGiftTrait::Copper, 1.0f}}}, // Desc_SpaceElevatorPart_3_C, //Automated Wiring
	{1338012, {{EGiftTrait::Copper, 1.0f}}}, // Desc_Battery_C, 
	{1338013, {{EGiftTrait::Silver, 1.0f},{EGiftTrait::Ore, 1.0f}}}, // Desc_OreBauxite_C, 
	{1338014, {{EGiftTrait::Tool, 1.0f}}}, // BP_EquipmentDescriptorBeacon_C, //Beacon
	{1338015, {{EGiftTrait::Grass, 1.0f}}}, // Desc_GenericBiomass_C, 
	{1338016, {{EGiftTrait::Bomb, 1.0f}}}, // Desc_Gunpowder_C, //Black Powder
	{1338017, {{EGiftTrait::Copper, 1.0f}}}, // Desc_Cable_C, 
	{1338018, {{EGiftTrait::Gold, 1.0f}}}, // Desc_GoldIngot_C, //Caterium Ingot
	{1338019, {{EGiftTrait::Gold, 1.0f},{EGiftTrait::Ore, 1.0f}}}, // Desc_OreGold_C, //Caterium Ore
	{1338020, {{EGiftTrait::Electronics, 1.0f}}}, // Desc_CircuitBoard_C, 
	{1338021, {{EGiftTrait::Coal, 1.0f},{EGiftTrait::Ore, 1.0f}}}, // Desc_Coal_C, 
	{1338022, {{EGiftTrait::Flower, 2.0f}}}, // Desc_ColorCartridge_C, 
	{1338023, {{EGiftTrait::Coal, 1.0f}}}, // Desc_CompactedCoal_C, 
	{1338024, {{EGiftTrait::Electronics, 1.0f}}}, // Desc_Computer_C, 
	{1338025, {{EGiftTrait::Stone, 1.0f},{EGiftTrait::Ore, 1.0f}}}, // Desc_Cement_C, //Concrete
	{1338026, {{EGiftTrait::Silver, 1.0f}}}, // Desc_CoolingSystem_C, 
	{1338027, {{EGiftTrait::Copper, 1.0f}}}, // Desc_CopperIngot_C, 
	{1338028, {{EGiftTrait::Copper, 1.0f}}}, // Desc_OreCopper_C, //Copper Ore
	{1338029, {{EGiftTrait::Copper, 1.0f}}}, // Desc_CopperDust_C, 
	{1338030, {{EGiftTrait::Copper, 1.0f}}}, // Desc_CopperSheet_C, 
	{1338031, {{EGiftTrait::Copper, 5.0f}}}, // Desc_CharacterRunStatue_C, 
	{1338032, {{EGiftTrait::Crystal, 1.0f}}}, // Desc_CrystalOscillator_C, 
	{1338033, {{EGiftTrait::Copper, 1.0f}}}, // Desc_ElectromagneticControlRod_C, 
	{1338034, {{EGiftTrait::Material, 1.0f}}}, // Desc_FluidCanister_C, 
	{1338035, {{EGiftTrait::Silver, 1.0f}}}, // Desc_GasTank_C, //Empty Fluid Tank
	{1338036, {{EGiftTrait::Steel, 0.5f},{EGiftTrait::Stone, 0.5f}}}, // Desc_SteelPlateReinforced_C, //Encased Industrial Beam
	{1338037, {{EGiftTrait::Stone, 1.0f},{EGiftTrait::Radioactive, 12.0f},{EGiftTrait::Damage, 12.0f}}}, // Desc_PlutoniumCell_C, 
	{1338038, {{EGiftTrait::Radioactive,	0.5f},{EGiftTrait::Damage, 0.05f}}}, // Desc_UraniumCell_C, 
	{1338039, {{EGiftTrait::Fiber, 1.0f}}}, // Desc_Fabric_C, 
	{1338040, {{EGiftTrait::Resource, 1.0f}}}, // Desc_ResourceSinkCoupon_C, 
	{1338041, {{EGiftTrait::Flower, 1.0f}}}, // Desc_FlowerPetals_C, 
	{1338042, {{EGiftTrait::Silver, 1.0f}}}, // Desc_ModularFrameFused_C, 
	{1338043, {{EGiftTrait::Electronics, 1.0f}}}, // Desc_HardDrive_C, 
	{1338044, {{EGiftTrait::Silver, 1.0f}}}, // Desc_AluminumPlateReinforced_C, //Heatsink
	{1338045, {{EGiftTrait::Steel, 1.0f}}}, // Desc_ModularFrameHeavy_C, 
	{1338046, {{EGiftTrait::Electronics, 1.0f}}}, // Desc_HighSpeedConnector_C, 
	{1338047, {{EGiftTrait::Gold, 5.0f}}}, // Desc_CharacterSpin_Statue_C, 
	{1338048, {{EGiftTrait::Silver, 5.0f}}}, // Desc_CharacterClap_Statue_C, 
	{1338049, {{EGiftTrait::Iron, 1.0f}}}, // Desc_IronIngot_C, 
	{1338050, {{EGiftTrait::Iron, 1.0f},{EGiftTrait::Ore, 1.0f}}}, // Desc_OreIron_C, 
	{1338051, {{EGiftTrait::Iron, 1.0f}}}, // Desc_IronPlate_C, 
	{1338052, {{EGiftTrait::Iron, 1.0f}}}, // Desc_IronRod_C, 
	{1338053, {{EGiftTrait::Gold, 5.0f}}}, // Desc_GoldenNut_Statue_C, 
	{1338054, {{EGiftTrait::Grass, 1.0f}}}, // Desc_Leaves_C, 
	{1338055, {{EGiftTrait::Stone, 1.0f},{EGiftTrait::Ore, 1.0f}}}, // Desc_Stone_C, //Limestone
	{1338056, {{EGiftTrait::Copper, 0.4f},{EGiftTrait::Iron, 0.3f},{EGiftTrait::Steel,0.3f}}}, // Desc_SpaceElevatorPart_6_C, //Magnetic Field Generator
	{1338057, {{EGiftTrait::Artifact,1.0f}}}, // Desc_WAT2_C, //Mercer Sphere
	{1338058, {{EGiftTrait::Copper, 1.0f}}}, // Desc_SpaceElevatorPart_4_C, //Modular Engine
	{1338059, {{EGiftTrait::Iron, 1.0f}}}, // Desc_ModularFrame_C, 
	{1338060, {{EGiftTrait::Copper, 1.0f}}}, // Desc_Motor_C, 
	{1338061, {{EGiftTrait::Fiber, 1.0f}}}, // Desc_Mycelia_C, 
	{1338062, {{EGiftTrait::Radioactive, 0.075f},{EGiftTrait::Damage, 0.075f}}}, // Desc_NonFissibleUranium_C, 
	{1338063, {{EGiftTrait::Silver, 0.3f},{EGiftTrait::Electronics, 0.4f},{EGiftTrait::Copper, 0.3f}}}, // Desc_SpaceElevatorPart_9_C, //Nuclear Pasta
	{1338064, {{EGiftTrait::Gold, 5.0f}}}, // Desc_DoggoStatue_C, 
	{1338065, {{EGiftTrait::Resource, 1.0f}}}, // Desc_AlienDNACapsule_C, 
	{1338066, {{EGiftTrait::Silver, 1.0f}}}, // Desc_PackagedAlumina_C, 
	{1338067, {{EGiftTrait::Material, 1.0f}}}, // Desc_Fuel_C, 
	{1338068, {{EGiftTrait::Material, 1.0f}}}, // Desc_PackagedOilResidue_C, 
	{1338069, {{EGiftTrait::Grass, 1.0f}}}, // Desc_PackagedBiofuel_C, 
	{1338070, {{EGiftTrait::Material, 1.0f}}}, // Desc_PackagedNitricAcid_C, 
	{1338071, {{EGiftTrait::Material, 1.0f}}}, // Desc_PackagedNitrogenGas_C, 
	{1338072, {{EGiftTrait::Material, 1.0f}}}, // Desc_PackagedOil_C, 
	{1338073, {{EGiftTrait::Material, 1.0f}}}, // Desc_PackagedSulfuricAcid_C, 
	{1338074, {{EGiftTrait::Material, 1.0f},{EGiftTrait::Speed, 1.0f}}}, // Desc_TurboFuel_C, //Packaged Turno Fuel
	{1338075, {{EGiftTrait::Material, 1.0f},{EGiftTrait::Drink, 1.0f}}}, // Desc_PackagedWater_C, 
	{1338076, {{EGiftTrait::Steel, 0.1f}}}, // Desc_PetroleumCoke_C, 
	{1338077, {{EGiftTrait::Material, 1.0f}}}, // Desc_Plastic_C, 
	{1338078, {{EGiftTrait::Stone, 1.0f},{EGiftTrait::Radioactive, 25.0f},{EGiftTrait::Damage, 25.0f}}}, // Desc_PlutoniumFuelRod_C, 
	{1338079, {{EGiftTrait::Radioactive, 2.0f},{EGiftTrait::Damage, 2.0f}}}, // Desc_PlutoniumPellet_C, 
	{1338080, {{EGiftTrait::Stone, 1.0f},{EGiftTrait::Radioactive, 20.0f},{EGiftTrait::Damage, 20.0f}}}, // Desc_PlutoniumWaste_C, 
	{1338081, {{EGiftTrait::Material, 1.0f}}}, // Desc_PolymerResin_C, 
	{1338082, {{EGiftTrait::Artifact,1.0f},{EGiftTrait::Speed,1.0f}}}, // Desc_CrystalShard_C, //Power Shard
	{1338083, {{EGiftTrait::Gold, 5.0f}}}, // Desc_SpaceGiraffeStatue_C, 
	{1338084, {{EGiftTrait::Silver, 0.4f},{EGiftTrait::Electronics, 0.6f}}}, // Desc_PressureConversionCube_C, 
	{1338085, {{EGiftTrait::Electronics, 1.0f}}}, // Desc_ComputerQuantum_C, 
	{1338086, {{EGiftTrait::Crystal, 1.0f}}}, // Desc_QuartzCrystal_C, 
	{1338087, {{EGiftTrait::Copper, 1.0f}}}, // Desc_HighSpeedWire_C, 
	{1338088, {{EGiftTrait::Electronics, 1.0f}}}, // Desc_ModularFrameLightweight_C, //Radio Control Unit
	{1338089, {{EGiftTrait::Crystal, 1.0f},{EGiftTrait::Mineral,1.0f}}}, // Desc_RawQuartz_C, 
	{1338090, {{EGiftTrait::Iron, 1.0f}}}, // Desc_IronPlateReinforced_C, 
	{1338091, {{EGiftTrait::Copper, 1.0f}}}, // Desc_Rotor_C, 
	{1338092, {{EGiftTrait::Material, 1.0f}}}, // Desc_Rubber_C, 
	//{1338093, {{ }}}, // Desc_SAM_C, 
	{1338094, {{EGiftTrait::Iron, 1.0f}}}, // Desc_IronScrew_C, 
	{1338095, {{EGiftTrait::Crystal, 1.0f}}}, // Desc_Silica_C, 
	{1338096, {{EGiftTrait::Iron, 0.5f},{EGiftTrait::Copper, 0.5f}}}, // Desc_SpaceElevatorPart_1_C, //Smart Plating
	{1338097, {{EGiftTrait::Bomb, 1.0f}}}, // Desc_GunpowderMK2_C, //Smokeless Powder
	{1338098, {{EGiftTrait::Grass, 1.0f}}}, // Desc_Biofuel_C, //Solid Biofuel
	{1338099, {{EGiftTrait::Artifact,1.0f}}}, // Desc_WAT1_C, //Somersloop
	{1338100, {{EGiftTrait::Copper, 1.0f}}}, // Desc_Stator_C, 
	{1338101, {{EGiftTrait::Silver, 50.0f}}}, // Desc_Hog_Statue_C, 
	{1338102, {{EGiftTrait::Steel, 1.0f}}}, // Desc_SteelPlate_C, 
	{1338103, {{EGiftTrait::Steel, 1.0f}}}, // Desc_SteelIngot_C, 
	{1338104, {{EGiftTrait::Steel, 1.0f}}}, // Desc_SteelPipe_C, 
	{1338105, {{EGiftTrait::Bomb, 1.0f}}}, // Desc_Sulfur_C, 
	{1338106, {{EGiftTrait::Electronics, 1.0f}}}, // Desc_ComputerSuper_C, 
	{1338107, {{EGiftTrait::Electronics, 1.0f}}}, // Desc_QuantumOscillator_C, 
	{1338108, {{EGiftTrait::Silver, 0.3f},{EGiftTrait::Electronics, 0.3f},{EGiftTrait::Steel, 0.4f},{EGiftTrait::Speed, 1.0f}}}, // Desc_SpaceElevatorPart_8_C, //Thermal Propulsion Rocket
	{1338109, {{EGiftTrait::Electronics, 0.5f},{EGiftTrait::Steel, 0.5f}}}, // Desc_MotorLightweight_C, 
	{1338110, {{EGiftTrait::Grass, 1.0f},{EGiftTrait::Resource, 0.1f}}}, // Desc_HogParts_C, 
	{1338111, {{EGiftTrait::Radioactive, 1.5f},{EGiftTrait::Damage, 1.5f},{EGiftTrait::Ore, 1.0f}}}, // Desc_OreUranium_C, 
	{1338112, {{EGiftTrait::Stone, 0.5f},{EGiftTrait::Steel, 0.5f},{EGiftTrait::Radioactive, 5.0f},{EGiftTrait::Damage, 5.0f}}}, // Desc_NuclearFuelRod_C, 
	{1338113, {{EGiftTrait::Radioactive, 1.0f},{EGiftTrait::Damage, 1.0f}}}, // Desc_NuclearWaste_C, 
	{1338114, {{EGiftTrait::Steel, 0.5f},{EGiftTrait::Iron, 0.5f}}}, // Desc_SpaceElevatorPart_2_C, //Versatile Framework
	{1338115, {{EGiftTrait::Copper, 1.0f}}}, // Desc_Wire_C, 
	{1338116, {{EGiftTrait::Wood, 1.0}}}, // Desc_Wood_C, 
	{1338117, {{EGiftTrait::Grass, 1.0f},{EGiftTrait::Resource, 0.1f}}}, // Desc_SpitterParts_C, 
	{1338118, {{EGiftTrait::Grass, 1.0f},{EGiftTrait::Resource, 0.1f}}}, // Desc_StingerParts_C, 
	{1338119, {{EGiftTrait::Grass, 1.0f},{EGiftTrait::Resource, 0.1f}}}, // Desc_HatcherParts_C, 

	//Enquipment/Ammo
	{1338150, {{EGiftTrait::Vegetable, 1.0f},{EGiftTrait::Heal, 1.0f}}}, // Desc_Shroom_C, //Bacon Agaric
	{1338151, {{EGiftTrait::Seed, 1.0f},{EGiftTrait::Heal, 1.0f}}}, // Desc_Nut_C, 
	{1338152, {{EGiftTrait::Tool, 1.0f},{EGiftTrait::Speed, 1.0f}}}, // BP_EquipmentDescriptorJumpingStilts_C, 
	{1338153, {{EGiftTrait::Weapon, 1.0f},{EGiftTrait::Buff, 1.0f}}}, //BoomBox
	{1338154, {{EGiftTrait::Weapon, 1.0f}}}, // Desc_Chainsaw_C, 
	{1338155, {{EGiftTrait::Bomb, 1.0f}}}, // Desc_NobeliskCluster_C, 
	//{1338156, {{ }}}, // Unused, 
	{1338157, {{EGiftTrait::Speed, 1.0f}}}, // BP_EquipmentDescriptorCup_C, 
	{1338158, {{EGiftTrait::Speed, 1.0f},{EGiftTrait::Gold, 1.0f}}}, // BP_EquipmentDescriptorCupGold_C, 
	{1338159, {{EGiftTrait::Weapon, 1.0f}}}, // Desc_Rebar_Explosive_C, 
	{1338160, {{EGiftTrait::Tool, 1.0f},{EGiftTrait::Speed, 1.0f}}}, // Desc_GolfCart_C, 
	{1338161, {{EGiftTrait::Tool, 1.0f},{EGiftTrait::Speed, 1.0f}}}, // Desc_GolfCartGold_C, 
	{1338162, {{EGiftTrait::Tool, 1.0f}}}, // BP_EquipmentDescriptorGasmask_C, 
	{1338163, {{EGiftTrait::Bomb, 1.0f}}}, // Desc_NobeliskGas_C, 
	{1338164, {{EGiftTrait::Armor, 1.0f}}}, // BP_EquipmentDescriptorHazmatSuit_C, 
	{1338165, {{EGiftTrait::Weapon, 1.0f}}}, // Desc_CartridgeSmartProjectile_C, 
	{1338166, {{EGiftTrait::Tool, 1.0f},{EGiftTrait::Speed, 1.0f}}}, // BP_EquipmentDescriptorHoverPack_C, 
	{1338167, {{EGiftTrait::Weapon, 1.0f}}}, // Desc_SpikedRebar_C, 
	{1338168, {{EGiftTrait::Tool, 1.0f},{EGiftTrait::Speed, 1.0f}}}, // BP_EquipmentDescriptorJetPack_C, 
	{1338169, {{EGiftTrait::Heal, 1.0f}}}, // Desc_Medkit_C, 
	{1338170, {{EGiftTrait::Bomb, 1.0f}}}, // Desc_NobeliskExplosive_C, 
	{1338171, {{EGiftTrait::Bomb, 1.0f}}}, // BP_EquipmentDescriptorNobeliskDetonator_C, 
	{1338172, {{EGiftTrait::Bomb, 1.0f}}}, // Desc_NobeliskNuke_C, 
	{1338173, {{EGiftTrait::Tool, 1.0f}}}, // BP_EquipmentDescriptorObjectScanner_C, 
	{1338174, {{EGiftTrait::Fruit, 1.0f},{EGiftTrait::Heal, 1.0f}}}, // Desc_Berry_C, 
	{1338175, {{EGiftTrait::Tool, 1.0f},{EGiftTrait::Slowness, 1.0f}}}, // Desc_Parachute_C, 
	{1338176, {{EGiftTrait::Bomb, 1.0f}}}, // Desc_NobeliskShockwave_C, 
	{1338177, {{EGiftTrait::Weapon, 1.0f}}}, // Desc_RebarGunProjectile_C, 
	{1338178, {{EGiftTrait::Weapon, 1.0f}}}, // BP_EquipmentDescriptorRifle_C, 
	{1338179, {{EGiftTrait::Weapon, 1.0f}}}, // Desc_CartridgeStandard_C, 
	{1338180, {{EGiftTrait::Weapon, 1.0f}}}, // Desc_Rebar_Spreadshot_C, 
	{1338181, {{EGiftTrait::Weapon, 1.0f}}}, // Desc_Rebar_Stunshot_C, 
	{1338182, {{EGiftTrait::Weapon, 1.0f}}}, // Desc_CartridgeChaos_C, 
	{1338183, {{EGiftTrait::Weapon, 1.0f}}}, // BP_EquipmentDescriptorStunSpear_C, 
	{1338184, {{EGiftTrait::Weapon, 1.0f}}}, // BP_EquipmentDescriptorShockShank_C, 
	{1338185, {{EGiftTrait::Tool, 1.0f},{EGiftTrait::Speed, 1.0f}}}, // BP_EqDescZipLine_C, 
	{1338186, {{EGiftTrait::Tool, 1.0f}}}, // BP_ItemDescriptorPortableMiner_C,
};

// The Unsinkables
const TMap<int64, int> UApGiftingMappings::HardcodedSinkValues = {
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