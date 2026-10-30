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
class AFCMobSpawnerBase;
class AFCMobCharacter;

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

	// Spawner Registration & Management
	UFUNCTION(BlueprintCallable, Category = "FC|Room|Combat")
	void RegisterChildSpawner(AFCMobSpawnerBase* Spawner);

	UFUNCTION(BlueprintCallable, Category = "FC|Room|Combat")
	void FindAndRegisterChildSpawners();

	const TArray<TObjectPtr<AFCMobSpawnerBase>>& GetChildSpawners() const { return ChildSpawners; }

	// Sockets accessors
	UFUNCTION(BlueprintPure, Category = "FC|Room|Sockets")
	FTransform GetDoorSocketTransform(uint8 DirectionBit) const;

	UFUNCTION(BlueprintPure, Category = "FC|Room|Doors")
	float GetDoorOffset() const { return DoorOffset; }

	UFUNCTION(BlueprintCallable, Category = "FC|Room|Doors")
	void SetDoorOffset(float NewDoorOffset);

	UFUNCTION(BlueprintCallable, Category = "FC|Room|Doors")
	void UpdateDoorSockets();

	UFUNCTION(BlueprintCallable, Category = "FC|Room|Doors")
	void SetDirectionDoorClass(uint8 DirectionBit, TSubclassOf<AFCDoorBase> InDoorClass);

	UFUNCTION(BlueprintPure, Category = "FC|Room|Doors")
	TSubclassOf<AFCDoorBase> GetDirectionDoorClass(uint8 DirectionBit) const;

	UFUNCTION(BlueprintPure, Category = "FC|Room|Doors")
	float GetDoorOffsetForDirection(uint8 DirectionBit) const;

	/** Register an externally spawned door actor (e.g. shared doorway) to this room's combat lock list */
	UFUNCTION(BlueprintCallable, Category = "FC|Room|Doors")
	void RegisterConnectedDoor(AFCDoorBase* Door);

	UFUNCTION(BlueprintPure, Category = "FC|Room|Doors")
	bool IsManagingDoorsExternally() const { return bManageDoorsExternally; }

	UFUNCTION(BlueprintCallable, Category = "FC|Room|Doors")
	void SetManageDoorsExternally(bool bEnable) { bManageDoorsExternally = bEnable; }

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
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Visual/Modular mesh setup in Blueprint or derived C++
	UFUNCTION(BlueprintNativeEvent, Category = "FC|Room")
	void OnSetupDoorsAndWalls(uint8 DoorMask);

	UFUNCTION()
	virtual void OnRoomBoundsBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Combat & Wave handling
	UFUNCTION(BlueprintNativeEvent, Category = "FC|Room|Combat")
	void HandleCombatTriggered();
	virtual void HandleCombatTriggered_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "FC|Room|Combat")
	void SpawnMonsterWave();
	virtual void SpawnMonsterWave_Implementation();

	virtual void StartRoomSpawners();
	virtual void CheckWaveCleared();

	UFUNCTION()
	virtual void OnMonsterDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	virtual void OnMonsterDeath(AActor* DeadActor, AActor* Killer);

	UFUNCTION()
	virtual void OnChildSpawnerAllMobsDefeated(AFCMobSpawnerBase* Spawner);

	UFUNCTION()
	virtual void OnChildSpawnerMobSpawned(AFCMobCharacter* Mob);

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

	UFUNCTION()
	virtual void OnRep_DoorOffset();

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

	// Spawners attached to or managed by this room
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Room|Combat")
	TArray<TObjectPtr<AFCMobSpawnerBase>> ChildSpawners;

	// Replicated State
	UPROPERTY(ReplicatedUsing = OnRep_RoomData, BlueprintReadOnly, Category = "FC|Room")
	FFCRoomData RoomData;

	UPROPERTY(ReplicatedUsing = OnRep_RoomState, BlueprintReadOnly, Category = "FC|Room")
	EFCRoomState RoomState = EFCRoomState::Unvisited;

	UPROPERTY(ReplicatedUsing = OnRep_IsDiscovered, BlueprintReadOnly, Category = "FC|Room|Exploration")
	bool bIsDiscovered = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsVisited, BlueprintReadOnly, Category = "FC|Room|Exploration")
	bool bIsVisited = false;

	/** Distance from room center to door/wall sockets (half-width of room). Defaults to 3000 cm (half of 6000 grid / matches RoomBoundsTrigger). */
	UPROPERTY(ReplicatedUsing = OnRep_DoorOffset, EditAnywhere, BlueprintReadWrite, Category = "FC|Room|Doors", meta = (ClampMin = "100.0"))
	float DoorOffset = 3000.0f;

	/** If true, automatically synchronizes DoorOffset to RoomBoundsTrigger's extent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Room|Doors")
	bool bSyncDoorOffsetToTrigger = true;

	/** If true, doorway actors are managed and spawned externally (e.g. by AFCDungeonManager for canonical shared edges) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Room|Doors")
	bool bManageDoorsExternally = false;

	/** Server-authoritative assigned door classes [0: North, 1: South, 2: East, 3: West]. Transient, not replicated. */
	UPROPERTY(Transient)
	TArray<TSubclassOf<AFCDoorBase>> DirectionDoorClasses;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FC|Room|Doors")
	TArray<TObjectPtr<AFCDoorBase>> ConnectedDoors;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FC|Room|Doors")
	TArray<TObjectPtr<AFCDestructibleWall>> SecretWalls;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FC|Room|Doors")
	TArray<TObjectPtr<AFCWallBase>> SpawnedWalls;

private:
	TArray<TWeakObjectPtr<APawn>> ActiveMonsters;
	TArray<TWeakObjectPtr<AFCMobSpawnerBase>> ActiveSpawnersRemaining;
};
