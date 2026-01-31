#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"

#include "Data/ApTypes.h"

#include "ApSubsystem.h"

#include "ApPlayerInfoSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApPlayerInfoSubsystem, Log, All);

constexpr uint8 RELIABLE_MESSAGING_CHANNEL_ID_PLAYER_INFO = 156; //random value to avoid collisions with other subsystems

USTRUCT(BlueprintType)
struct ARCHIPELAGO_API FReplicatedFApPlayerInfo : public FApPlayer
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	FString Name;
	UPROPERTY(BlueprintReadWrite)
	FString Game;

	// Value constructor
	FReplicatedFApPlayerInfo(int team, uint8 slot, FString name, FString game) : FApPlayer(team, slot), Name(name), Game(game)
	{}

	FReplicatedFApPlayerInfo(FApPlayer player, FString name, FString game) : FApPlayer(player), Name(name), Game(game)
	{}
	// Default constructor
	FReplicatedFApPlayerInfo() : FReplicatedFApPlayerInfo(-1, -1, TEXT(""), TEXT("Archipelago"))
	{}
	// Copy constructor
	FReplicatedFApPlayerInfo(const FReplicatedFApPlayerInfo& Other) : FApPlayer(Other), Name(Other.Name), Game(Other.Game)
	{}
};

enum class EPlayerInfoSubsystemMessageId : uint32
{
	InitialReplication = 0x01,
	PartialUpdate = 0x02,
};

struct FPlayerInfoSubsystemInitialReplicationMessage
{
	static constexpr EPlayerInfoSubsystemMessageId MessageId = EPlayerInfoSubsystemMessageId::InitialReplication;
	TArray<FReplicatedFApPlayerInfo> PlayerInfos;

	friend FArchive& operator<<(FArchive& Ar, FPlayerInfoSubsystemInitialReplicationMessage& Message);
};

struct FPlayerInfoSubsystemUpdateReplicationMessage
{
	static constexpr EPlayerInfoSubsystemMessageId MessageId = EPlayerInfoSubsystemMessageId::PartialUpdate;
	TArray<FReplicatedFApPlayerInfo> PlayerInfos;

	friend FArchive& operator<<(FArchive& Ar, FPlayerInfoSubsystemInitialReplicationMessage& Message);
};

UCLASS()
class ARCHIPELAGO_API AApPlayerInfoSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AApPlayerInfoSubsystem();

	virtual void BeginPlay() override;

	virtual void Tick(float dt) override;

	static AApPlayerInfoSubsystem* Get(UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Player Info Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApPlayerInfoSubsystem* Get(UObject* worldContext);

private:
	UPROPERTY()
	AApSubsystem* ap;
	UPROPERTY()
	AApConnectionInfoSubsystem* connectionInfoSubsystem;

	bool isInitialized;

	TMap<FApPlayer, FString> PlayerGamesMap;
	TMap<FApPlayer, FString> PlayerNamesMap;

	bool hasMultipleTeams = false;

public:
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsInitialized() const { return isInitialized; };

	UFUNCTION(BlueprintPure)
	FString GetPlayerName(FApPlayer player) const;

	UFUNCTION(BlueprintPure)
	FString GetPlayerGame(FApPlayer player) const;

	UFUNCTION(BlueprintPure)
	int GetPlayerCount() const;

private:
	void InitializeData(TArray<FReplicatedFApPlayerInfo> playerInfos);

	void SendInitialReplicationDataForAllClients();
	void SendInitialReplicationData(const APlayerController* PlayerController);

	void UpdateReplicationDataForAllClients(const TArray<FReplicatedFApPlayerInfo>& playerInfosToUpdate) const;
	void SendUpdatedReplicationData(APlayerController* PlayerController, const TArray<FReplicatedFApPlayerInfo> playerInfos) const;
	
//
// called from Player Controller mixin
//
public:
	UFUNCTION(BlueprintCallable)
	void OnPlayerControllerBeginPlay(const APlayerController* PlayerController);

protected:
	/** Handles a reliable message */
	void OnRawDataReceived(TArray<uint8>&& InMessageData);
	/** Sends a a reliable message */
	void SendRawMessage(const APlayerController* PlayerController, EPlayerInfoSubsystemMessageId MessageId, const TFunctionRef<void(FArchive&)>& MessageSerializer) const;

	/** Handles Initial Replication Data for Player Info Subsystem */
	void ReceiveInitialReplicationData(const FPlayerInfoSubsystemInitialReplicationMessage& Message);
	void ReceiveInitialReplicationData(const FPlayerInfoSubsystemUpdateReplicationMessage& Message);
//
//
//
};
