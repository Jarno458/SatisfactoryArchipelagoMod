#include "Data/ApMappings.h"

const TMap<int64, FString> UApMappings::ItemIdToGameItemDescriptor = {
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
	{1338088, TEXT("Desc_ModularFrameLightweight_C")}, //Radio Control Unit
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
	{1338110, TEXT("Desc_HogParts_C")},
	{1338111, TEXT("Desc_OreUranium_C")},
	{1338112, TEXT("Desc_NuclearFuelRod_C")},
	{1338113, TEXT("Desc_NuclearWaste_C")},
	{1338114, TEXT("Desc_SpaceElevatorPart_2_C")}, //Versatile Framework
	{1338115, TEXT("Desc_Wire_C")},
	{1338116, TEXT("Desc_Wood_C")},
	{1338117, TEXT("Desc_SpitterParts_C")},
	{1338118, TEXT("Desc_StingerParts_C")},
	{1338119, TEXT("Desc_HatcherParts_C")},
	{1338120, TEXT("Desc_AlienDNACapsule_C")},

	//Enquipment/Ammo
	{1338150, TEXT("Desc_Shroom_C")},
	{1338151, TEXT("Desc_Nut_C")},
	{1338152, TEXT("BP_EquipmentDescriptorJumpingStilts_C")},
	//{1338153, TEXT("")}, //BoomBox
	{1338154, TEXT("Desc_Chainsaw_C")},
	{1338155, TEXT("Desc_NobeliskCluster_C")},
	//{1338156, TEXT("Unused")},
	{1338157, TEXT("BP_EquipmentDescriptorCup_C")},
	{1338158, TEXT("BP_EquipmentDescriptorCupGold_C")},
	{1338159, TEXT("Desc_Rebar_Explosive_C")},
	{1338160, TEXT("Desc_GolfCart_C")},
	{1338161, TEXT("Desc_GolfCartGold_C")},
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
	{1338186, TEXT("BP_ItemDescriptorPortableMiner_C")},
	{1338187, TEXT("Desc_Filter_C")},
	//TODO implement custom unlocks
	{1338188, TEXT("Desc_Filter_C")}, //3 "/Script/Engine.Blueprint'/Game/FactoryGame/Unlocks/BP_UnlockInventorySlot.BP_UnlockInventorySlot'"
	{1338189, TEXT("Desc_Filter_C")}, //6 "/Script/Engine.Blueprint'/Game/FactoryGame/Unlocks/BP_UnlockInventorySlot.BP_UnlockInventorySlot'"
	{1338190, TEXT("Desc_Filter_C")} //1 "/Script/Engine.Blueprint'/Game/FactoryGame/Unlocks/BP_UnlockArmEquipmentSlot.BP_UnlockArmEquipmentSlot'"
};

const TMap<int64, FString> UApMappings::ItemIdToGameRecipe = {
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
	//{1338309, TEXT("")}, //Bauxite
	{1338310, TEXT("Recipe_AluminumScrap")},
	{1338311, TEXT("Recipe_Alternate_ElectroAluminumScrap")},
	{1338312, TEXT("Recipe_Alternate_InstantScrap")},
	{1338313, TEXT("Recipe_IngotAluminum")},
	{1338314, TEXT("Recipe_PureAluminumIngot")},
	{1338315, TEXT("Recipe_AluminumSheet")},
	{1338316, TEXT("Recipe_AluminumCasing")},
	{1338317, TEXT("Recipe_Alternate_AlcladCasing")},
	{1338318, TEXT("Recipe_HeatSink")},
	{1338319, TEXT("Recipe_Alternate_HeatSink_1")},
	//{1338320, TEXT("")}, //Nitrogen Gas
	{1338321, TEXT("Recipe_NitricAcid")},
	{1338322, TEXT("Recipe_FusedModularFrame")},
	{1338323, TEXT("Recipe_Alternate_HeatFusedFrame")},
	{1338324, TEXT("Recipe_RadioControlUnit")},
	{1338325, TEXT("Recipe_Alternate_RadioControlUnit_1")},
	{1338326, TEXT("Recipe_Alternate_RadioControlSystem")},
	{1338327, TEXT("Recipe_PressureConversionCube")},
	{1338328, TEXT("Recipe_CoolingSystem")},
	{1338329, TEXT("Recipe_Alternate_CoolingDevice")},
	{1338330, TEXT("Recipe_MotorTurbo")},
	{1338331, TEXT("Recipe_Alternate_TurboMotor_1")},
	{1338332, TEXT("Recipe_Alternate_TurboPressureMotor")},
	{1338333, TEXT("Recipe_Battery")},
	{1338334, TEXT("Recipe_Alternate_ClassicBattery")},
	{1338335, TEXT("Recipe_ComputerSuper")},
	{1338336, TEXT("Recipe_Alternate_OCSupercomputer")},
	{1338337, TEXT("Recipe_Alternate_SuperStateComputer")},
	//{1338338, TEXT("")}, //Uranium
	{1338339, TEXT("Recipe_SulfuricAcid")},
	//{1338340, TEXT("")}, //Unused
	{1338341, TEXT("Recipe_UraniumCell")},
	{1338342, TEXT("Recipe_Alternate_UraniumCell_1")},
	{1338343, TEXT("Recipe_NuclearFuelRod")},
	{1338344, TEXT("Recipe_Alternate_NuclearFuelRod_1")},
	{1338345, TEXT("Recipe_Beacon")},
	{1338346, TEXT("Recipe_Alternate_Beacon_1")},
	//{1338347, TEXT("")}, // Recipe: Uranium Waste
	{1338348, TEXT("Recipe_NonFissileUranium")},
	{1338349, TEXT("Recipe_Alternate_FertileUranium")},
	{1338350, TEXT("Recipe_Plutonium")},
	{1338351, TEXT("Recipe_PlutoniumCell")},
	{1338352, TEXT("Recipe_Alternate_InstantPlutoniumCell")},
	{1338353, TEXT("Recipe_PlutoniumFuelRod")},
	{1338354, TEXT("Recipe_Alternate_PlutoniumFuelUnit")},
	{1338355, TEXT("Recipe_FilterGasMask")},
	{1338356, TEXT("Recipe_FilterHazmat")},
	{1338357, TEXT("Recipe_SpaceElevatorPart_7")},
	{1338358, TEXT("Recipe_SpaceElevatorPart_6")},
	{1338359, TEXT("Recipe_CopperDust")},
	{1338360, TEXT("Recipe_SpaceElevatorPart_9")},
	{1338361, TEXT("Recipe_SpaceElevatorPart_8")},
	//{1338362, TEXT("")}, //Recipe: Leaves
	//{1338363, TEXT("")}, //Recipe: Wood
	//{1338364, TEXT("")}, //Recipe: Hatcher Remains
	//{1338365, TEXT("")}, //Recipe: Hog Remains
	//{1338366, TEXT("")}, //Recipe: Plasma Spitter Remains
	//{1338367, TEXT("")}, //Recipe: Stinger Remains
	{1338368, TEXT("Recipe_Protein_Crab")},
	{1338369, TEXT("Recipe_Protein_Hog")},
	{1338370, TEXT("Recipe_Protein_Spitter")},
	{1338371, TEXT("Recipe_Protein_Stinger")},
	{1338372, TEXT("Recipe_Biomass_Leaves")},
	{1338373, TEXT("Recipe_Biomass_Wood")},
	{1338374, TEXT("Recipe_Biomass_Mycelia")},
	{1338375, TEXT("Recipe_Biomass_AlienProtein")},
	{1338376, TEXT("Recipe_CartridgeChaos_Packaged")}, 
	{1338377, TEXT("Recipe_Fabric")},
	{1338378, TEXT("Recipe_Alternate_PolyesterFabric")},
	{1338379, TEXT("Recipe_Biofuel")},
	{1338380, TEXT("Recipe_LiquidBiofuel")},
	{1338381, TEXT("Recipe_FluidCanister")},
	{1338382, TEXT("Recipe_Alternate_CoatedIronCanister")},
	{1338383, TEXT("Recipe_Alternate_SteelCanister")},
	{1338384, TEXT("Recipe_GasTank")}, // Empty Fluid Tank
	{1338385, TEXT("Recipe_PackagedAlumina")}, // TODO include unpackage
	{1338386, TEXT("Recipe_Fuel")}, //Packaged Fuel
	{1338387, TEXT("Recipe_Alternate_DilutedPackagedFuel")},
	{1338388, TEXT("Recipe_PackagedOilResidue")},
	{1338389, TEXT("Recipe_PackagedBiofuel")}, //Packaged Liquid Biofuel
	{1338390, TEXT("Recipe_PackagedNitricAcid")},
	{1338391, TEXT("Recipe_PackagedNitrogen")},
	{1338392, TEXT("Recipe_PackagedCrudeOil")},
	{1338393, TEXT("Recipe_PackagedSulfuricAcid")},
	{1338394, TEXT("Recipe_PackagedTurboFuel")},
	{1338395, TEXT("Recipe_PackagedWater")},
	{1338396, TEXT("Recipe_Alternate_Turbofuel")},
	{1338397, TEXT("Recipe_Alternate_TurboHeavyFuel")},
	{1338398, TEXT("Recipe_Alternate_TurboBlendFuel")},
	{1338399, TEXT("Recipe_HazmatSuit")},
	{1338400, TEXT("Recipe_Gasmask")},
	{1338401, TEXT("Recipe_Gunpowder")},
	{1338402, TEXT("Recipe_BladeRunners")},
	{1338403, TEXT("Recipe_Chainsaw")},
	{1338404, TEXT("Recipe_NobeliskCluster")},
	{1338405, TEXT("Recipe_Rebar_Explosive")},
	{1338406, TEXT("Recipe_FactoryCart")},
	{1338407, TEXT("Recipe_NobeliskGas")},
	{1338408, TEXT("Recipe_GoldenCart")},
	{1338409, TEXT("Recipe_CartridgeSmart")}, //Homing Rifle Cartridge
	{1338410, TEXT("Recipe_SpikedRebar")}, //Iron Rebar
	{1338411, TEXT("Recipe_Nobelisk")},
	{1338412, TEXT("Recipe_NobeliskNuke")},
	{1338413, TEXT("Recipe_NutritionalInhaler")},
	{1338414, TEXT("Recipe_ObjectScanner")},
	{1338415, TEXT("Recipe_Parachute")},
	{1338416, TEXT("Recipe_MedicinalInhalerAlienOrgans")}, //Protein Inhaler
	{1338403, TEXT("Recipe_NobeliskShockwave")},
	{1338417, TEXT("Recipe_RebarGun")},
	{1338418, TEXT("Recipe_SpaceRifleMk1")},
	{1338419, TEXT("Recipe_Cartridge")},
	{1338420, TEXT("Recipe_Rebar_Spreadshot")}, //Shatter Rebar
	{1338421, TEXT("Recipe_Rebar_Stunshot")},
	{1338422, TEXT("Recipe_TherapeuticInhaler")},
	{1338423, TEXT("Recipe_CartridgeChaos")},
	{1338424, TEXT("Recipe_MedicinalInhaler")},
	{1338425, TEXT("Recipe_XenoBasher")},
	{1338426, TEXT("Recipe_XenoZapper")},
	{1338427, TEXT("Recipe_ZipLine")},
	{1338428, TEXT("Recipe_Alternate_Gunpowder_1")},
	{1338429, TEXT("Recipe_GunpowderMK2")},
	{1338430, TEXT("Recipe_AlienDNACapsule")},
	{1338431, TEXT("Recipe_PowerCrystalShard_1")},
	{1338432, TEXT("Recipe_PowerCrystalShard_2")},
	{1338433, TEXT("Recipe_PowerCrystalShard_3")},
	//TODO implement
	// "BP_UnlockInventorySlot"
	//{1338188, TEXT("Small inflated Pocket Dimension")},
	//{1338189, TEXT("Inflated Pocket Dimension")},
	//{1338190, TEXT("Expanded Toolbelt")},
};

const TMap<int64, TArray<FString>> UApMappings::ItemIdToGameBuilding = {
	{1338600, {TEXT("Recipe_ConstructorMk1")}},
	{1338601, {TEXT("Recipe_AssemblerMk1")}},
	{1338602, {TEXT("Recipe_ManufacturerMk1")}},
	{1338603, {TEXT("Recipe_Packager")}},
	{1338604, {TEXT("Recipe_OilRefinery")}},
	{1338605, {TEXT("Recipe_Blender")}},
	{1338606, {TEXT("Recipe_HadronCollider")}},
	{1338607, {TEXT("Recipe_GeneratorBiomass")}},
	{1338608, {TEXT("Recipe_GeneratorCoal")}},
	{1338609, {TEXT("Recipe_GeneratorGeoThermal")}},
	{1338610, {TEXT("Recipe_GeneratorNuclear")}},
	{1338611, {TEXT("Recipe_MinerMk1")}},
	{1338612, {TEXT("Recipe_MinerMk2")}},
	{1338613, {TEXT("Recipe_MinerMk3")}},
	{1338614, {TEXT("Recipe_OilPump")}},
	{1338615, {TEXT("Recipe_WaterPump")}},
	{1338616, {TEXT("Recipe_SmelterBasicMk1")}},
	{1338617, {TEXT("Recipe_SmelterMk1")}},


	{1338618, {TEXT("Recipe_GeneratorFuel")}},
	{1338619, {TEXT("Recipe_FrackingSmasher"), TEXT("Recipe_FrackingExtractor")}},
	{1338620, {TEXT("Recipe_Workshop")}},
	{1338621, {TEXT("Recipe_ResourceSink")}},
	{1338622, {TEXT("Recipe_ResourceSinkShop")}},
	//1338623 Schematic
	{1338624, {TEXT("Recipe_BlueprintDesigner")}},
	{1338625, {TEXT("Recipe_PipeStorageTank")}},
	{1338626, {TEXT("Recipe_IndustrialTank")}},
	{1338627, {TEXT("Recipe_JumpPad")}},
	//1338628 Schematic
	{1338629, {TEXT("Recipe_Mam")}},
	{1338630, {TEXT("Recipe_StoragePlayer")}}, //TODO: include "Recipe_StorageMedkit", "Recipe_StorageHazard"
	{1338631, {TEXT("Recipe_PowerStorageMk1")}},
	{1338632, {TEXT("Recipe_UJellyLandingPad")}},
	{1338633, {TEXT("Recipe_PowerSwitch")}},
	{1338634, {TEXT("Recipe_PriorityPowerSwitch")}},
	{1338635, {TEXT("Recipe_StorageContainerMk1")}},
	{1338636, {TEXT("Recipe_LookoutTower")}},
	{1338637, {TEXT("Recipe_PowerPoleMk1")}},
	{1338638, {TEXT("Recipe_PowerPoleMk2")}},
	{1338639, {TEXT("Recipe_PowerPoleMk3")}},
	{1338640, {TEXT("Recipe_StorageContainerMk2")}},
	{1338641, {TEXT("Recipe_ConveyorAttachmentMerger")}},
	{1338642, {TEXT("Recipe_ConveyorAttachmentSplitter")}},
	{1338643, {TEXT("Recipe_ConveyorBeltMk1")}}, //TODO include "Recipe_ConveyorPole"
	{1338644, {TEXT("Recipe_ConveyorBeltMk2")}},
	{1338645, {TEXT("Recipe_ConveyorBeltMk3")}},
	{1338646, {TEXT("Recipe_ConveyorBeltMk4")}},
	{1338647, {TEXT("Recipe_ConveyorBeltMk5")}},
	{1338648, {TEXT("Recipe_ConveyorLiftMk1")}},
	{1338649, {TEXT("Recipe_ConveyorLiftMk2")}},
	{1338650, {TEXT("Recipe_ConveyorLiftMk3")}},
	{1338651, {TEXT("Recipe_ConveyorLiftMk4")}},
	{1338652, {TEXT("Recipe_ConveyorLiftMk5")}},
	//1338653 Schematic
	{1338654, {TEXT("Recipe_ConveyorPoleStackable")}},
	{1338655, {TEXT("Recipe_ConveyorPoleWall")}},
	{1338656, {TEXT("Recipe_FoundationPassthrough_Lift")}},
	{1338657, {TEXT("Recipe_ConveyorCeilingAttachment")}},
	{1338658, {TEXT("Recipe_Pipeline")}}, //TODO include: "Recipe_PipeSupport"
	{1338659, {TEXT("Recipe_PipelineMK2")}},
	{1338660, {TEXT("Recipe_PipelinePump")}},
	{1338661, {TEXT("Recipe_PipelinePumpMK2")}},
	{1338662, {TEXT("Recipe_PipelineJunction_Cross")}},
	{1338663, {TEXT("Recipe_Valve")}},
	{1338664, {TEXT("Recipe_PipeSupportStackable")}},
	{1338665, {TEXT("Recipe_PipeSupportWall")}},
	{1338666, {TEXT("Recipe_PipeSupportWallHole")}},
	{1338667, {TEXT("Recipe_FoundationPassthrough_Pipe")}},
	{1338668, {TEXT("Recipe_LightsControlPanel")}},
	{1338669, {TEXT("Recipe_FloodlightWall")}},
	{1338670, {TEXT("Recipe_StreetLight")}},
	{1338671, {TEXT("Recipe_FloodlightPole")}},
	{1338672, {TEXT("Recipe_CeilingLight")}},
	{1338673, {TEXT("Recipe_PowerTower")}}, //TODO include "Recipe_PowerTowerPlatform"
	{1338674, {TEXT("Recipe_Wall_8x4_01")}}, //TODO include "Recipe_Wall_Orange_8x1"
	{1338675, {TEXT("Recipe_RadarTower")}},
	{1338676, {TEXT("Recipe_ConveyorAttachmentSplitterSmart")}},
	{1338677, {TEXT("Recipe_ConveyorAttachmentSplitterProgrammable")}},
	//1338678 Schematic
	//1338679 Schematic
	//1338680 Schematic
	//1338681 Schematic
	//1338682 Schematic
	//1338683 Schematic
	//1338684 Schematic
	//1338685 Schematic
	//{1338686, {TEXT("")}},
	//{1338687, {TEXT("")}},
	//{1338688, {TEXT("")}},
	//{1338689, {TEXT("")}},
	//{1338690, {TEXT("")}},
	//{1338691, {TEXT("")}},
	{1338692, {TEXT("Recipe_Foundation_8x4_01")}}, //TODO include: "Recipe_Foundation_8x1_01", "Recipe_Foundation_8x2_01", "Recipe_Ramp_8x1_01", "Recipe_Ramp_8x2_01","Recipe_Ramp_8x4_01"
	//1338693 Schematic
	//1338694 Schematic
	//1338695 Schematic
	//1338696 Schematic
	//1338697 Schematic
	//1338698 Schematic
	//1338699 Schematic

	{1338999, {TEXT("Recipe_SpaceElevator")}},
};

const TMap<int64, FString> UApMappings::ItemIdToGameSchematic = {
	{1338628, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_Ladders.ResourceSink_Ladders")},

	{1338623, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_BeamSet.ResourceSink_BeamSet")},
	{1338653, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/Parts/Tier3/ResourceSink_SteelBeam.ResourceSink_SteelBeam")},

	{1338678, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_LabelSigns.ResourceSink_LabelSigns")},
	{1338679, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_DisplaySigns.ResourceSink_DisplaySigns")},
	{1338680, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_BillboardSigns.ResourceSink_BillboardSigns")},
	{1338681, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/Customizer_Background/CBG_Steel_Walls_Basic.CBG_Steel_Walls_Basic")}, //TODO include i dont f*cking know
	{1338682, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_FoudationPillar.ResourceSink_FoudationPillar")},
	{1338683, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_ConcretePillarSet.ResourceSink_ConcretePillarSet'")},
	{1338684, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_FramePillarSet.ResourceSink_FramePillarSet'")},
	{1338685, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/Customizer_Background/CBG_Concrete_Walls_Basic.CBG_Concrete_Walls_Basic")},

	{1338693, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_HalfFoundations.ResourceSink_HalfFoundations'")},
	//{1338694, TEXT("Corner Ramp Pack")},
	{1338695, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_InvertedRampPack.ResourceSink_InvertedRampPack'")},
	{1338696, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_InvertedCornerRamps.ResourceSink_InvertedCornerRamps'")},
	//{1338697, TEXT("Quarter Pipes Pack")},
	{1338698, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_QuarterPipeExtensions.ResourceSink_QuarterPipeExtensions'")},
	{1338699, TEXT("/Script/Engine.Blueprint'/Game/FactoryGame/Schematics/ResourceSink/ResourceSink_FrameworkFoundations.ResourceSink_FrameworkFoundations'")},



	//TODO
	// walls
	// roofs
	// wall power
	// frame foundations
	// stairs


};

const TMap<int64, FName> UApMappings::ItemIdToTrap = {
	// Regenerate via /Script/Blutility.EditorUtilityWidgetBlueprint'/Archipelago/Debug/EU_GenerateTrapIds.EU_GenerateTrapIds'
	{1338900, FName(TEXT("HogBasic"))},
	{1338901, FName(TEXT("HogAlpha"))},
	{1338902, FName(TEXT("HogJohnny"))},
	{1338903, FName(TEXT("HogCliff"))},
	{1338904, FName(TEXT("HogCliffNuclear"))},
	{1338905, FName(TEXT("TheBees"))},
	{1338906, FName(TEXT("Hatcher"))},
	{1338907, FName(TEXT("DoggoGiftPulseNobelisk"))},
	{1338908, FName(TEXT("DoggoGiftNukeNobelisk"))},
	{1338909, FName(TEXT("DoggoGiftSlug_Nice"))},
	{1338910, FName(TEXT("DoggoGiftGasNobelisk"))},
	{1338911, FName(TEXT("SporeFlower"))},
	{1338912, FName(TEXT("StingerGas"))},
	{1338913, FName(TEXT("StingerElite"))},
	{1338914, FName(TEXT("StingerSmall"))},
	{1338915, FName(TEXT("SpitterForest"))},
	{1338916, FName(TEXT("SpitterForestAlpha"))},
	{1338917, FName(TEXT("NuclearWaste"))},
	{1338918, FName(TEXT("PlutoniumWaste"))},
};


const TMap<int64, FString> UApMappings::ItemIdToGameName2 = {
};