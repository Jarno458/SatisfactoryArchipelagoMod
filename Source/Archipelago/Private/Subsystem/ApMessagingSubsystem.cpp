#include "Subsystem/ApMessagingSubsystem.h"
#include "ApUtils.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogApChat);

void AApMessagingSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AApMessagingSubsystem, OnJoinMessages);
}

AApMessagingSubsystem* AApMessagingSubsystem::Get(class UWorld* worldContext) {
	return UApUtils::GetSubsystemActorIncludingParentClases<AApMessagingSubsystem>(worldContext);
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
	MessageStruct.MessageText = FText::FromString(Message);
	MessageStruct.MessageType = EFGChatMessageType::CMT_SystemMessage;
	MessageStruct.ServerTimeStamp = GetWorld()->TimeSeconds;
	MessageStruct.MessageSenderColor = Color;
	ChatManager->AddChatMessageToReceived(MessageStruct);

	UE_LOG(LogApChat, Display, TEXT("Displaying Local Chat Message: %s"), *Message);
}
