#include "ApSubsystem.h"

DEFINE_LOG_CATEGORY(ApSubsystem);

//TODO REMOVE
#pragma optimize("", off)

std::map<std::string, std::function<void(AP_SetReply)>> AApSubsystem::callbacks;

TMap<int64_t, FString> AApSubsystem::ItemIdToGameName = {
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
	{1337583, TEXT("Desc_SpaceGiraffeStatue_C"))},
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
};

TMap<int64_t, FString> AApSubsystem::ItemIdToGameName2 = {
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
	//{1337531, TEXT("Desc_LiquidOil_C")},
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
	//{1337547, "Duplicate"},
	//{1337548, "HUB Parts"},
	{1337549, TEXT("Desc_IronIngot_C")},
	{1337550, TEXT("Desc_OreIron_C")},
	{1337551, TEXT("Desc_IronPlate_C")},
	{1337552, TEXT("Desc_IronRod_C")},
	//{1337553, "Duplicate"},
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
	//{1337564, "Duplicated"},
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
	//{1337583, "Unused")},
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
	{1337699, TEXT("Desc_WAT1_C")}, //Somersloop
};

AApSubsystem::AApSubsystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
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
		contentLibSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UContentLibSubsystem>();
		check(contentLibSubsystem)

		FApConfigurationStruct config = GetActiveConfig();

		if (!config.Enabled)	{
			SetActorTickEnabled(false);
			return;
		}

		ConnectToArchipelago(config);

		FDateTime connectingStartedTime = FDateTime::Now();
	
		FGenericPlatformProcess::ConditionalSleep([this, config, connectingStartedTime]() { return InitializeTick(config, connectingStartedTime); }, 1);
	}
	else if (phase == ELifecyclePhase::INITIALIZATION) {
		if (ConnectionState != EApConnectionState::Connected) {
			FApConfigurationStruct config = GetActiveConfig();

			FString message = FString::Printf(TEXT("Failed to connect to Archipelago server: \"%s\", for user \"%s\""), *config.Url, *config.Login);

			SendChatMessage(message, FLinearColor::Red);
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
	std::string const game = TCHAR_TO_UTF8(*config.Game);
	std::string const user = TCHAR_TO_UTF8(*config.Login);
	std::string const password = TCHAR_TO_UTF8(*config.Password);

	AP_Init(uri.c_str(), game.c_str(), user.c_str(), password.c_str());

	AP_SetItemClearCallback(AApSubsystem::ItemClearCallback);
	AP_SetItemRecvCallback(AApSubsystem::ItemReceivedCallback);
	AP_SetLocationCheckedCallback(AApSubsystem::LocationCheckedCallback);
	AP_RegisterSetReplyCallback(AApSubsystem::SetReplyCallback);
	AP_SetLocationInfoCallback(AApSubsystem::LocationScoutedCallback);
	AP_RegisterSlotDataIntCallback("FirstHubLocation", AApSubsystem::SlotDataFirstHubLocation);
	AP_RegisterSlotDataIntCallback("LastHubLocation", AApSubsystem::SlotDataLastHubLocation);

	ConnectionState = EApConnectionState::Connecting;

	AP_Start();
}

void AApSubsystem::OnMamResearchCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubSystem::OnResearchCompleted(schematic), Mam Research Completed"));

	//if (schematic.) //if name is Archipelago #xxxx send check to server
}


void AApSubsystem::OnSchematicCompleted(TSubclassOf<class UFGSchematic> schematic) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubSystem::OnSchematicCompleted(schematic)"));

	ESchematicType type = UFGSchematic::GetType(schematic);

	if (type != ESchematicType::EST_Milestone || !locationsPerMileStone.Contains(schematic))
		return;

	for (auto location : locationsPerMileStone[schematic])
		AP_SendItem(location.location);
}

void AApSubsystem::ItemClearCallback() {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::ItemClearCallback()"));

}

void AApSubsystem::ItemReceivedCallback(int64_t id, bool notify) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::ItemReceivedCallback(%i, %s)"), id, (notify ? TEXT("true") : TEXT("false")));




	//map to Schematic and unlock it
}

void AApSubsystem::LocationCheckedCallback(int64_t id) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::LocationCheckedCallback(%i)"), id);

}

void AApSubsystem::SetReplyCallback(AP_SetReply setReply) {
	FString fstringKey(setReply.key.c_str());
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::SetReplyCallback(%s)"), *fstringKey);

	if (callbacks.count(setReply.key))
		callbacks[setReply.key](setReply);
}

void AApSubsystem::LocationScoutedCallback(std::vector<AP_NetworkItem> scoutedLocations) {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::HintUnlockedHubRecipies(vector[%i])"), scoutedLocations.size());

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

// Called every frame
void AApSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ConnectionState != EApConnectionState::Connected)
		return;

	HandleAPMessages();
}

void AApSubsystem::CheckConnectionState(FApConfigurationStruct config) {
	if (ConnectionState == EApConnectionState::Connecting) {
		AP_ConnectionStatus status = AP_GetConnectionStatus();

		if (status == AP_ConnectionStatus::Authenticated) {
			ConnectionState = EApConnectionState::Connected;

			UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::Tick(), Successfully Authenticated"));
		}
		else if (status == AP_ConnectionStatus::ConnectionRefused) {
			ConnectionState = EApConnectionState::ConnectionFailed;

			FString message = FString::Printf(TEXT("Failed to connect to Archipelago server: \"%s\", for user \"%s\""), *config.Url, *config.Login);

			SendChatMessage(message, FLinearColor::Green);
		}
	}
}

void AApSubsystem::ParseScoutedItems() {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::ParseScoutedItems(vector[%i])"), scoutedLocations.Num());

	TMap<FString, TSubclassOf<UFGSchematic>> schematicsPerMilestone = TMap<FString, TSubclassOf<UFGSchematic>>();

	for (auto& location : scoutedLocations) {
		if (location.locationName.starts_with("Hub"))
		{
			std::string milestoneString = location.locationName.substr(0, location.locationName.find(","));
			FString milestone = FString(milestoneString.c_str());

			if (!schematicsPerMilestone.Contains(milestone)) {
				TSubclassOf<UFGSchematic> schematic = FClassGenerator::GenerateSimpleClass(TEXT("/Archipelago/"), *milestone, UFGSchematic::StaticClass());

				schematicsPerMilestone.Add(milestone, schematic);
			}

			if (!locationsPerMileStone.Contains(schematicsPerMilestone[milestone])) {
				locationsPerMileStone.Add(schematicsPerMilestone[milestone], TArray<AP_NetworkItem>{ location });
			} else {
				locationsPerMileStone[schematicsPerMilestone[milestone]].Add(location);
			}
		}
	}

	AModContentRegistry* contentRegistry = AModContentRegistry::Get(GetWorld());

	for (auto& itemPerMilestone : locationsPerMileStone) {
		for (auto& item : itemPerMilestone.Value)
			CreateRecipe(contentRegistry, item);

		FString schematicName;
		for (auto schematicAndName : schematicsPerMilestone)
		{
			if (itemPerMilestone.Key == schematicAndName.Value)
			{
				schematicName = schematicAndName.Key;
				break;
			}
		}

		CreateHubSchematic(contentRegistry, schematicName, itemPerMilestone.Key, itemPerMilestone.Value);
	}

	scoutedLocations.Empty();
	shouldParseItemsToScout = false;
}

void AApSubsystem::CreateRecipe(AModContentRegistry* contentRegistry, AP_NetworkItem item) {
	FString name((item.playerName + " - " + item.itemName).c_str());
	FString uniqueId(std::to_string(item.location).c_str());
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
	TSubclassOf<UFGRecipe> factoryRecipy = FClassGenerator::GenerateSimpleClass(TEXT("/Archipelago/"), *uniqueId, UFGRecipe::StaticClass());
	UCLRecipeBPFLib::InitRecipeFromStruct(contentLibSubsystem, clRecipy, factoryRecipy);

	contentRegistry->RegisterRecipe(FName(TEXT("Archipelago")), factoryRecipy);
}

void AApSubsystem::CreateItem(AModContentRegistry* contentRegistry, AP_NetworkItem item) {
	FString name((item.playerName + " " + item.itemName).c_str());
	FString uniqueId(std::to_string(item.location).c_str());
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
	TSubclassOf<UFGItemDescriptor> factoryItem = FClassGenerator::GenerateSimpleClass(TEXT("/Archipelago/"), *uniqueId, UFGItemDescriptor::StaticClass());
	UCLItemBPFLib::InitItemFromStruct(factoryItem, clItem, contentLibSubsystem);

	//contentRegistry->RegisterItem(FName(TEXT("Archipelago")), factoryItem); //no idea how/where to register items
}

void AApSubsystem::CreateHubSchematic(AModContentRegistry* contentRegistry, FString name, TSubclassOf<UFGSchematic> factorySchematic, TArray<AP_NetworkItem> items) {
	std::string buildRecipies = "";
	/*for (auto& item : items) {
		if (buildRecipies.length() > 0)
			buildRecipies = +"\", \"" + item.itemName;
		else
			buildRecipies = item.itemName;
	}*/

	/*for (auto& item : items) {
		if (buildRecipies.length() > 0)
			buildRecipies += "\", \"/Archipelago/" + std::to_string(item.location);
		else
			buildRecipies = "/Archipelago/" + std::to_string(item.location);
	}*/

	for (auto& item : items) {
		if (buildRecipies.length() > 0)
			buildRecipies += "\", \"" + std::to_string(item.location);
		else
			buildRecipies = std::to_string(item.location);
	}

	int delimeterPos;
	name.FindChar('-', delimeterPos);

	FString tier = name.RightChop(delimeterPos + 1);
	FString recipies(buildRecipies.c_str());
	// https://raw.githubusercontent.com/budak7273/ContentLib_Documentation/main/JsonSchemas/CL_Schematic.json
	FString json = FString::Printf(TEXT(R"({
		"Name": "%s",
		"Type": "Milestone",
		"Time": 100,
		"Tier": %s,
		"VisualKit": "Kit_AP_Logo",
		"Cost": [
			{
				"Item": "Desc_CopperSheet",
				"Amount": 1
			},
		],
		"Recipes": [ "%s" ]
	})"), *name, *tier, *recipies);

	FContentLib_Schematic schematic = UCLSchematicBPFLib::GenerateCLSchematicFromString(json);
	UCLSchematicBPFLib::InitSchematicFromStruct(schematic, factorySchematic, contentLibSubsystem);

	contentRegistry->RegisterSchematic(FName(TEXT("Archipelago")), factorySchematic);
}

void AApSubsystem::HandleAPMessages() {
	for (int i = 0; i < 10; i++)
	{
		if (!AP_IsMessagePending())
			return;

		AP_Message* message = AP_GetLatestMessage();
		FString fStringMessage(message->text.c_str());

		SendChatMessage(fStringMessage, FLinearColor::White);

		AP_ClearLatestMessage();
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
}

void AApSubsystem::HintUnlockedHubRecipies() {
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::HintUnlockedHubRecipies()"));

	std::vector<int64_t> locations;

	for (int64_t i = firstHubLocation; i <= lastHubLocation; i++)
		locations.push_back(i);

	AP_SendLocationScouts(locations, 0);
}

void AApSubsystem::TimeoutConnectionIfNotConnected() {
	if (ConnectionState != EApConnectionState::Connecting)
		return;
	
	UE_LOG(ApSubsystem, Display, TEXT("AApSubsystem::TimeoutConnectionIfNotConnected(), Authenticated Failed"));

	SetActorTickEnabled(false);

	ConnectionState = EApConnectionState::ConnectionFailed;
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