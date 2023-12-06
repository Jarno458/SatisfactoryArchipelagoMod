#include "Data/ApSlotData.h"
#include "ApUtils.h"

// Possibly switch this over to reading from mod config?
// #define UNKNOWN_ITEMID_LOG_MESSAGE_TYPE Fatal
#define UNKNOWN_ITEMID_LOG_MESSAGE_TYPE Error

FApSlotData::FApSlotData() {
}

bool FApSlotData::ParseSlotData(std::string json, FApSlotData* data) {
	//TODO use https://docs.unrealengine.com/5.1/en-US/API/Runtime/JsonUtilities/FJsonObjectConverter/JsonObjectStringToUStruct/

	FString jsonString(json.c_str());

	const TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*jsonString);

	FJsonSerializer serializer;
	TSharedPtr<FJsonObject> parsedJson;

	serializer.Deserialize(reader, parsedJson);
	if (!parsedJson.IsValid()) {
		return false;
	}

	for (TSharedPtr<FJsonValue> tier : parsedJson->GetArrayField("HubLayout")) {
		TArray<TMap<FString, int>> milestones;

		for (TSharedPtr<FJsonValue> milestone : tier->AsArray()) {
			TMap<FString, int> costs;

			for (TPair<FString, TSharedPtr<FJsonValue>> cost : milestone->AsObject()->Values) {
				int itemId = FCString::Atoi(*cost.Key);

				if (UApMappings::ItemIdToGameItemDescriptor.Contains(itemId)) {
					costs.Add(UApMappings::ItemIdToGameItemDescriptor[itemId], cost.Value->AsNumber());
				} else {
					// Edit the define to control if this crashes or just errors
					UE_LOG(LogArchipelagoCpp, UNKNOWN_ITEMID_LOG_MESSAGE_TYPE, TEXT("Archipelago slot data contained AP itemId %d that is not present in ItemIdToGameItemDescriptor mappings. Are you using an out of date version of the mod for the Archipelago server version?"), itemId);
					costs.Add(UApMappings::ItemIdToGameItemDescriptor[MISSING_ITEMID_DEVELOPER_BACKUP], cost.Value->AsNumber());
				}
			}

			milestones.Add(costs);
		}

		data->hubLayout.Add(milestones);
	}

	TSharedPtr<FJsonObject> options = parsedJson->GetObjectField("Options");

	data->numberOfChecksPerMilestone = parsedJson->GetIntegerField("SlotsPerMilestone");
	data->finalSpaceElevatorTier = options->GetIntegerField("FinalElevatorTier");
	data->finalResourceSinkPoints = options->GetIntegerField("FinalResourceSinkPoints");
	data->hasLoadedSlotData = true;

	return true;
}
