#pragma once

#include "CoreMinimal.h"
#include "FGChatManager.h"
#include "Subsystem/ModSubsystem.h"

#include "ApMessagingSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApChat, All, All);

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
	void DisplayMessage(const FString& Message, const FLinearColor& Color);
	
	// C++ getter for the subsystem - the real one is implemented in Blueprint,
	// so calling this function gets you the "real one" that inherits from the
	// C++ class and not the C++ abstract class.
	UFUNCTION(BlueprintPure, Category = "Archipelago", DisplayName = "GetArchipelagoMessagingSubsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApMessagingSubsystem* Get(class UObject* worldContext);

	// Displays a chat message to the local client, you probably don't want to call this directly 
	UFUNCTION(BlueprintCallable)
	void LocalAddChatMessage(const FString& Message, const FLinearColor& Color);
};
