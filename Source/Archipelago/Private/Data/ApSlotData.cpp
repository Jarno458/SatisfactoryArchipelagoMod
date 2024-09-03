#include "Data/ApSlotData.h"

/*
bool FApSlotData::ParseSlotData(FString jsonString, FApSlotData* data) {
	//TODO use https://docs.unrealengine.com/5.1/en-US/API/Runtime/JsonUtilities/FJsonObjectConverter/JsonObjectStringToUStruct/

	const TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*jsonString);

	FJsonSerializer serializer;
	TSharedPtr<FJsonObject> parsedJson;

	serializer.Deserialize(reader, parsedJson);
	if (!parsedJson.IsValid()) {
		return false;
	}

	for (TSharedPtr<FJsonValue> tier : parsedJson->GetArrayField("HubLayout")) {
		TArray<TMap<int64, int>> milestones;

		for (TSharedPtr<FJsonValue> milestone : tier->AsArray()) {
			TMap<int64, int> costs;

			for (TPair<FString, TSharedPtr<FJsonValue>> cost : milestone->AsObject()->Values) {
				int64 itemId = FCString::Atoi64(*cost.Key);

				costs.Add(itemId, cost.Value->AsNumber());
			}

			milestones.Add(costs);
		}

		data->hubLayout.Add(milestones);
	}

	TSharedPtr<FJsonObject> options = parsedJson->GetObjectField("Options");

	data->numberOfChecksPerMilestone = parsedJson->GetIntegerField("SlotsPerMilestone");
	data->finalSpaceElevatorTier = options->GetIntegerField("FinalElevatorTier");
	data->finalResourceSinkPoints = options->GetIntegerField("FinalResourceSinkPoints");
	data->freeSampleEquipment = options->GetIntegerField("FreeSampleEquipment");
	data->freeSampleBuildings = options->GetIntegerField("FreeSampleBuildings");
	data->freeSampleParts = options->GetIntegerField("FreeSampleParts");
	data->freeSampleRadioactive = options->GetBoolField("FreeSampleRadioactive");

	if (!options->TryGetBoolField("EnergyLink", data->energyLink))
		data->energyLink = false;

	if (!options->TryGetBoolField("EnableHardDriveGacha", data->enableHardDriveGacha))
		data->enableHardDriveGacha = false;

	data->hasLoadedSlotData = true;

	return true;
}
*/