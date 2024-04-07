#pragma once

#include "CoreMinimal.h"
#include "FGSaveInterface.h"
#include "Subsystem/ModSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogApConnectionInfoSubsystem, Log, All);

#include "ApConnectionInfoSubsystem.generated.h"

UENUM(BlueprintType)
enum class EApConnectionState : uint8 {
	NotYetAttempted UMETA(DisplayName = "Not Yet Attempted"),
	Connecting UMETA(DisplayName = "Connecting"),
	Connected UMETA(DisplayName = "Connection Successful"),
	ConnectionFailed UMETA(DisplayName = "Connection Failed")
};

UCLASS()
class ARCHIPELAGO_API AApConnectionInfoSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

	// Begin IFGSaveInterface
	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void GatherDependencies_Implementation(TArray<UObject*>& out_dependentObjects) override {};
	virtual bool NeedTransform_Implementation() override { return false; };
	virtual bool ShouldSave_Implementation() const override { return true; };
	// End IFSaveInterface
	
public:
	// Sets default values for this actor's properties
	AApConnectionInfoSubsystem();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// Get subsystem. Server-side only, null on clients
	static AApConnectionInfoSubsystem* Get(class UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Archipelago Connection Info Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AApConnectionInfoSubsystem* Get(UObject* worldContext);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE EApConnectionState GetConnectionState() const { return ConnectionState; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE FText GetConnectionStateDescription() const { return ConnectionStateDescription; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE int GetCurrentPlayerTeam() const { return currentPlayerTeam; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE int GetCurrentPlayerSlot() const { return currentPlayerSlot; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE FString GetRoomSeed() const { return roomSeed; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE FString GetSlotDataJson() const { return slotDataJson; };

private:
	UPROPERTY(Replicated)
	EApConnectionState ConnectionState;
	UPROPERTY(Replicated)
	FText ConnectionStateDescription;
	UPROPERTY(Replicated)
	int currentPlayerTeam = 0;
	UPROPERTY(Replicated)
	int currentPlayerSlot = 0;
	UPROPERTY(SaveGame)
	FString roomSeed;
	UPROPERTY(SaveGame)
	FString slotDataJson;

public:
	friend class AApSubsystem;
};
