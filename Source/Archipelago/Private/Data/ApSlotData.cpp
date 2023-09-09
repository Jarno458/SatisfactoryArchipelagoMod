#include "Data/ApSlotData.h"
#include "Data/ApItemIdMapping.h"

FApSlotData::FApSlotData() {
}

bool FApSlotData::ParseSlotData(std::string json, FApSlotData* data) {
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

				verify(UApItemIdMapping::ItemIdToGameItemDescriptor.Contains(itemId));

				costs.Add(UApItemIdMapping::ItemIdToGameItemDescriptor[itemId], cost.Value->AsNumber());
			}

			milestones.Add(costs);
		}

		data->hubLayout.Add(milestones);
	}

	TSharedPtr<FJsonObject> options = parsedJson->GetObjectField("Options");

	data->currentPlayerSlot = parsedJson->GetIntegerField("Slot");
	data->numberOfChecksPerMilestone = parsedJson->GetIntegerField("SlotsPerMilestone");
	data->finalSpaceElevatorTier = options->GetIntegerField("FinalElevatorTier");
	data->finalResourceSinkPoints = options->GetIntegerField("FinalResourceSinkPoints");
	data->hasLoadedSlotData = true;
	return true;
}
