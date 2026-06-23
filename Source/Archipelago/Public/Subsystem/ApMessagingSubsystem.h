#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"

#include "ApMessagingSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApChat, All, All);

UENUM(BlueprintType)
enum class EApMessageType : uint8 {
	ApSystem UMETA(DisplayName = "Mod System Messages"),
	ItemSend UMETA(DisplayName = "Item Sent"),
	ItemReceived UMETA(DisplayName = "Item Received"),
	Hint UMETA(DisplayName = "Hint"),
	Countdown UMETA(DisplayName = "Countdown"),
	Other UMETA(DisplayName = "Other"),
	GiftsReceived UMETA(DisplayName = "Gifts Received"),
	VaultReceived UMETA(DisplayName = "Vault Item Received")
};


/**
  * A message to display to the user at a later point
  */
USTRUCT(BlueprintType)
struct ARCHIPELAGO_API FApStoredChatMessage {
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite)
	FLinearColor PrefixColor;

	UPROPERTY(BlueprintReadWrite)
	FString Contents;
};

/**
 * Blueprint implemented subsystem for communicating Archipelago messages to players
 */
UCLASS(Abstract, Blueprintable)
class ARCHIPELAGO_API AApMessagingSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadWrite, Replicated)
	TArray<FApStoredChatMessage> OnJoinMessages;

	// Sends a chat message to all connected Satisfactory players
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void DisplayMessage(EApMessageType type, const FString& Message, const FLinearColor& Color);
	
	UFUNCTION(BlueprintPure, Category = "Archipelago", DisplayName = "GetArchipelagoMessagingSubsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApMessagingSubsystem* Get(class UWorld* worldContext);

	// Displays a chat message to the local client, you probably don't want to call this directly 
	UFUNCTION(BlueprintCallable)
	void LocalAddChatMessage(const FString& Message, const FLinearColor& Color);
};
