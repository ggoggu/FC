#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/FCDungeonTypes.h"
#include "FCDungeonManager.generated.h"

class AFCRoomBase;
class AFCDestructibleWall;
class APlayerStart;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFCDungeonGenerated);

UCLASS()
class FC_API AFCDungeonManager : public AActor
{
	GENERATED_BODY()

public:
	AFCDungeonManager();

	// Server-side generation of both logic grid and 3D world actors
	UFUNCTION(BlueprintCallable, Category = "FC|Dungeon")
	void GenerateDungeon();

	// Clear previously spawned rooms
	UFUNCTION(BlueprintCallable, Category = "FC|Dungeon")
	void ClearDungeon();

	const TArray<TObjectPtr<AFCRoomBase>>& GetSpawnedRooms() const { return SpawnedRooms; }

	UFUNCTION(BlueprintPure, Category = "FC|Dungeon")
	AFCRoomBase* GetRoomAt(const FIntPoint& Coord) const;

	UFUNCTION(BlueprintCallable, Category = "FC|Dungeon")
	void RevealAdjacentRooms(const FIntPoint& CenterCoord);

	UFUNCTION(BlueprintCallable, Category = "FC|Dungeon")
	void SetLayoutPattern(EFCDungeonLayoutPattern NewPattern) { LayoutPattern = NewPattern; }

	UFUNCTION(BlueprintPure, Category = "FC|Dungeon")
	EFCDungeonLayoutPattern GetLayoutPattern() const { return LayoutPattern; }

	UFUNCTION(BlueprintCallable, Category = "FC|Dungeon")
	void SetStartLocationMode(EFCDungeonStartLocation NewMode) { StartLocationMode = NewMode; }

	UFUNCTION(BlueprintPure, Category = "FC|Dungeon")
	EFCDungeonStartLocation GetStartLocationMode() const { return StartLocationMode; }

	UFUNCTION(BlueprintPure, Category = "FC|Dungeon")
	FIntPoint GetStartRoomCoordinate() const { return StartRoomCoord; }

	UFUNCTION(BlueprintPure, Category = "FC|Dungeon")
	AFCRoomBase* GetStartRoom() const { return GetRoomAt(StartRoomCoord); }

	UFUNCTION(BlueprintPure, Category = "FC|Dungeon")
	APlayerStart* GetStartRoomPlayerStart() const { return StartRoomPlayerStart.Get(); }

	UFUNCTION(BlueprintPure, Category = "FC|Dungeon")
	float GetCellSize() const { return CellSize; }

	UFUNCTION(BlueprintCallable, Category = "FC|Dungeon")
	void SetCellSize(float NewCellSize) { CellSize = FMath::Max(500.0f, NewCellSize); }

	UFUNCTION(BlueprintPure, Category = "FC|Dungeon")
	bool IsAutoSyncDoorOffsetToCellSize() const { return bAutoSyncDoorOffsetToCellSize; }

	UFUNCTION(BlueprintCallable, Category = "FC|Dungeon")
	void SetAutoSyncDoorOffsetToCellSize(bool bEnable) { bAutoSyncDoorOffsetToCellSize = bEnable; }

	/** Returns a randomly chosen room class from DefaultRooms based on weights */
	UFUNCTION(BlueprintPure, Category = "FC|Dungeon")
	TSubclassOf<AFCRoomBase> GetRandomDefaultRoomClass() const;

	UFUNCTION(BlueprintPure, Category = "FC|Dungeon")
	const TArray<FFCDungeonRoomWeight>& GetDefaultRooms() const { return DefaultRooms; }

	UFUNCTION(BlueprintCallable, Category = "FC|Dungeon")
	void SetDefaultRooms(const TArray<FFCDungeonRoomWeight>& InDefaultRooms) { DefaultRooms = InDefaultRooms; }

	UFUNCTION(BlueprintCallable, Category = "FC|Dungeon")
	void AddDefaultRoom(TSubclassOf<AFCRoomBase> RoomClass, float Weight = 1.0f)
	{
		DefaultRooms.Add(FFCDungeonRoomWeight(RoomClass, Weight));
	}

	/** Returns a randomly chosen door class from DefaultDoorPool based on weights (fallbacks safely to AFCDoorBase) */
	UFUNCTION(BlueprintPure, Category = "FC|Dungeon|Doors")
	TSubclassOf<AFCDoorBase> GetRandomDoorFromPool() const;

	UFUNCTION(BlueprintPure, Category = "FC|Dungeon|Doors")
	const TArray<FFCDungeonDoorWeight>& GetDefaultDoorPool() const { return DefaultDoorPool; }

	UFUNCTION(BlueprintCallable, Category = "FC|Dungeon|Doors")
	void SetDefaultDoorPool(const TArray<FFCDungeonDoorWeight>& InPool) { DefaultDoorPool = InPool; }

	UFUNCTION(BlueprintCallable, Category = "FC|Dungeon|Doors")
	void AddDefaultDoor(TSubclassOf<AFCDoorBase> DoorClass, float Weight = 1.0f)
	{
		DefaultDoorPool.Add(FFCDungeonDoorWeight(DoorClass, Weight));
	}

	UFUNCTION(BlueprintPure, Category = "FC|Dungeon|Doors")
	TSubclassOf<AFCDoorBase> ResolveDoorClassBetween(const FIntPoint& RoomA, const FIntPoint& RoomB) const;

	/** Teleport all existing player pawns into the Start Room */
	UFUNCTION(BlueprintCallable, Category = "FC|Dungeon")
	void TeleportPlayersToStartRoom();

	/** Ensure APlayerStart exists and is aligned with the Start Room */
	UFUNCTION(BlueprintCallable, Category = "FC|Dungeon")
	void SetupStartRoomPlayerStart();

public:
	UPROPERTY(BlueprintAssignable, Category = "FC|Dungeon|Events")
	FOnFCDungeonGenerated OnDungeonGenerated;

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void HandleRoomStateChanged(AFCRoomBase* Room, EFCRoomState NewState);

	UFUNCTION()
	virtual void HandleSecretWallDestroyed(AFCDestructibleWall* Wall);

	// Grid configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Dungeon|Config")
	EFCDungeonLayoutPattern LayoutPattern = EFCDungeonLayoutPattern::Asymmetric;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Dungeon|Config")
	EFCDungeonStartLocation StartLocationMode = EFCDungeonStartLocation::Center;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Dungeon|Config")
	bool bPreventClustering = true;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Dungeon|Config")
	int32 TargetRoomCount = 15;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Dungeon|Config")
	int32 MaxSecretRooms = 1;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Dungeon|Config", meta = (ClampMin = "500.0"))
	float CellSize = 7200.0f;

	/** If true, automatically synchronizes spawned rooms' DoorOffset to CellSize * 0.5f for seamless room-to-room door alignment. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Dungeon|Config")
	bool bAutoSyncDoorOffsetToCellSize = true;

	// Modular Room Classes
	/** Weighted list of room classes for default/normal rooms. One is randomly selected per room spawn based on weight. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Dungeon|Classes", meta = (DisplayName = "Default Rooms"))
	TArray<FFCDungeonRoomWeight> DefaultRooms;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Dungeon|Classes")
	TSubclassOf<AFCRoomBase> StartRoomClass;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Dungeon|Classes")
	TSubclassOf<AFCRoomBase> BossRoomClass;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Dungeon|Classes")
	TSubclassOf<AFCRoomBase> ShopRoomClass;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Dungeon|Classes")
	TSubclassOf<AFCRoomBase> TreasureRoomClass;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Dungeon|Classes")
	TSubclassOf<AFCRoomBase> SecretRoomClass;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Dungeon|Classes")
	TSubclassOf<AFCDestructibleWall> SecretWallClass;

	// Modular Door Classes & Pools
	/** Weighted list of door classes for default/normal room connections. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Dungeon|Doors", meta = (DisplayName = "Default Doors"))
	TArray<FFCDungeonDoorWeight> DefaultDoorPool;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Dungeon|SpecialDoors")
	TSubclassOf<AFCDoorBase> BossDoorClass;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Dungeon|SpecialDoors")
	TSubclassOf<AFCDoorBase> ShopDoorClass;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Dungeon|SpecialDoors")
	TSubclassOf<AFCDoorBase> TreasureDoorClass;

	// Local cached list of spawned room actors (Actors replicate themselves individually)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FC|Dungeon")
	TArray<TObjectPtr<AFCRoomBase>> SpawnedRooms;

	// Local cached list of spawned shared door actors
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FC|Dungeon|Doors")
	TArray<TObjectPtr<AFCDoorBase>> SpawnedSharedDoors;

private:
	// In-memory 2D grid of rooms (Server Authority)
	TMap<FIntPoint, FFCRoomData> RoomGrid;

	// Canonical mapping from edge connecting RoomA and RoomB to assigned DoorClass
	TMap<FFCDungeonEdgeKey, TSubclassOf<AFCDoorBase>> EdgeDoorMap;

	// Core Generation Steps (Logic)
	void BuildSkeleton();
	void AssignRoomRoles();
	void CalculateDoorMasks();
	void ResolveEdgeDoorways();

	// Step 4 & 5: World Spawner & Connectors
	void SpawnDungeonRooms();
	void SpawnSharedDoorways();
	void SetupSecretPassages();
	TSubclassOf<AFCRoomBase> GetRoomClassForType(EFCRoomType Type) const;

	// Helper functions
	TArray<FIntPoint> GetAdjacentCoordinates(const FIntPoint& Coord) const;
	int32 CountAdjacentRooms(const FIntPoint& Coord) const;
	bool HasAdjacentRoomOfType(const FIntPoint& Coord, EFCRoomType Type) const;
	void RecalculateDistancesFrom(const FIntPoint& OriginCoord);

	FIntPoint StartRoomCoord = FIntPoint::ZeroValue;

	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerStart> StartRoomPlayerStart;

	UPROPERTY(Transient)
	bool bSpawnedOwnPlayerStart = false;
};
