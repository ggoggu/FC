#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/FCDungeonTypes.h"
#include "FCRoomBase.generated.h"

class UBoxComponent;
class USceneComponent;
class AFCDoorBase;
class AFCDestructibleWall;
class AFCWallBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFCRoomStateChanged, AFCRoomBase*, Room, EFCRoomState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFCRoomDiscoveryChanged, AFCRoomBase*, Room, bool, bDiscovered);

UCLASS()
class FC_API AFCRoomBase : public AActor
{
	GENERATED_BODY()

public:
	AFCRoomBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Server-side initialization
	UFUNCTION(BlueprintCallable, Category = "FC|Room")
	void InitializeRoom(const FFCRoomData& InRoomData);

	// Server-side state transition
	UFUNCTION(BlueprintCallable, Category = "FC|Room")
	void SetRoomState(EFCRoomState NewState);

	// Exploration State Mutation (Server)
	UFUNCTION(BlueprintCallable, Category = "FC|Room|Exploration")
	void SetDiscovered(bool bNewDiscovered);

	UFUNCTION(BlueprintCallable, Category = "FC|Room|Exploration")
	void SetVisited(bool bNewVisited);

	// Door Control (Server)
	UFUNCTION(BlueprintCallable, Category = "FC|Room|Doors")
	void LockAllDoors(bool bLock);

	// Sockets accessors
	UFUNCTION(BlueprintPure, Category = "FC|Room|Sockets")
	FTransform GetDoorSocketTransform(uint8 DirectionBit) const;

	UFUNCTION(BlueprintPure, Category = "FC|Room")
	const FFCRoomData& GetRoomData() const { return RoomData; }

	UFUNCTION(BlueprintPure, Category = "FC|Room")
	EFCRoomState GetRoomState() const { return RoomState; }

	UFUNCTION(BlueprintPure, Category = "FC|Room|Exploration")
	bool IsDiscovered() const { return bIsDiscovered; }

	UFUNCTION(BlueprintPure, Category = "FC|Room|Exploration")
	bool IsVisited() const { return bIsVisited; }

	UFUNCTION(BlueprintPure, Category = "FC|Room")
	FTransform GetPlayerSpawnTransform() const;

	UFUNCTION(BlueprintPure, Category = "FC|Room")
	USceneComponent* GetPlayerSpawnPoint() const { return PlayerSpawnPoint; }

	const TArray<TObjectPtr<AFCDoorBase>>& GetConnectedDoors() const { return ConnectedDoors; }
	const TArray<TObjectPtr<AFCDestructibleWall>>& GetSecretWalls() const { return SecretWalls; }
	const TArray<TObjectPtr<AFCWallBase>>& GetSpawnedWalls() const { return SpawnedWalls; }

public:
	UPROPERTY(BlueprintAssignable, Category = "FC|Room|Events")
	FOnFCRoomStateChanged OnRoomStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "FC|Room|Events")
	FOnFCRoomDiscoveryChanged OnRoomDiscoveryChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Visual/Modular mesh setup in Blueprint or derived C++
	UFUNCTION(BlueprintNativeEvent, Category = "FC|Room")
	void OnSetupDoorsAndWalls(uint8 DoorMask);

	UFUNCTION()
	virtual void OnRoomBoundsBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Combat & Wave handling
	virtual void HandleCombatTriggered();
	virtual void SpawnMonsterWave();
	virtual void CheckWaveCleared();

	UFUNCTION()
	virtual void OnMonsterDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	virtual void OnSecretWallDestroyed(AFCDestructibleWall* Wall);

	UFUNCTION()
	virtual void OnRep_RoomData();

	UFUNCTION()
	virtual void OnRep_RoomState(EFCRoomState OldState);

	UFUNCTION()
	virtual void OnRep_IsDiscovered();

	UFUNCTION()
	virtual void OnRep_IsVisited();

protected:
	// Scene components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<USceneComponent> NorthDoorSocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<USceneComponent> SouthDoorSocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<USceneComponent> EastDoorSocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<USceneComponent> WestDoorSocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<USceneComponent> PlayerSpawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<UBoxComponent> RoomBoundsTrigger;

	// Modular Assets Classes
	UPROPERTY(EditDefaultsOnly, Category = "FC|Room|Classes")
	TSubclassOf<AFCDoorBase> DefaultDoorClass;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Room|Classes")
	TSubclassOf<AFCDestructibleWall> DefaultSecretWallClass;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Room|Classes")
	TSubclassOf<AFCWallBase> DefaultWallClass;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Room|Combat")
	TArray<TSubclassOf<APawn>> MonsterClasses;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Room|Combat")
	int32 MonsterWaveCount = 3;

	// Replicated State
	UPROPERTY(ReplicatedUsing = OnRep_RoomData, BlueprintReadOnly, Category = "FC|Room")
	FFCRoomData RoomData;

	UPROPERTY(ReplicatedUsing = OnRep_RoomState, BlueprintReadOnly, Category = "FC|Room")
	EFCRoomState RoomState = EFCRoomState::Unvisited;

	UPROPERTY(ReplicatedUsing = OnRep_IsDiscovered, BlueprintReadOnly, Category = "FC|Room|Exploration")
	bool bIsDiscovered = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsVisited, BlueprintReadOnly, Category = "FC|Room|Exploration")
	bool bIsVisited = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FC|Room|Doors")
	TArray<TObjectPtr<AFCDoorBase>> ConnectedDoors;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FC|Room|Doors")
	TArray<TObjectPtr<AFCDestructibleWall>> SecretWalls;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FC|Room|Doors")
	TArray<TObjectPtr<AFCWallBase>> SpawnedWalls;

private:
	TArray<TWeakObjectPtr<APawn>> ActiveMonsters;
};
