#include "Subsystem/ApMessagingSubsystem.h"
#include "ApUtils.h"

DEFINE_LOG_CATEGORY(LogApChat);

AApMessagingSubsystem* AApMessagingSubsystem::Get(class UObject* worldContext) {
	if (!worldContext) {
		return nullptr;
	}

	TArray<AActor*> arr;
	// Relies on the fact that nothing has spawned the C++ version before
	// The blueprint one descends from this so it is found instead
	// This would break if a C++ version was spawned or persisted via save game
	UGameplayStatics::GetAllActorsOfClass(worldContext, AApMessagingSubsystem::StaticClass(), arr);
	if (arr.IsValidIndex(0)) {
		return Cast<AApMessagingSubsystem>(arr[0]);
	} else {
		return nullptr;
	}
}

void AApMessagingSubsystem::DisplayMessage_Implementation(const FString& Message, const FLinearColor& Color) {
	UE_LOG(LogArchipelagoCpp, Error, TEXT("No DisplayMessage C++ implementation, why is this getting called instead of the BP one?"));
}

void AApMessagingSubsystem::LocalAddChatMessage(const FString& Message, const FLinearColor& Color) {
	AFGChatManager* ChatManager = AFGChatManager::Get(GetWorld());
	if (!ChatManager) {
		UE_LOG(LogApChat, Error, TEXT("Too early to send chat message. Would have been: %s"), *Message);
		return;
	}
	FChatMessageStruct MessageStruct;
	MessageStruct.MessageString = Message;
	MessageStruct.MessageType = EFGChatMessageType::CMT_SystemMessage;
	MessageStruct.ServerTimeStamp = GetWorld()->TimeSeconds;
	MessageStruct.CachedColor = Color;
	ChatManager->AddChatMessageToReceived(MessageStruct);

	UE_LOG(LogApChat, Display, TEXT("Displaying Local Chat Message: %s"), *Message);
}
