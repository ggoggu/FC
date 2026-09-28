#pragma once

#include "CoreMinimal.h"
#include "FCDungeonTypes.generated.h"

class AFCRoomBase;

/**
 * Room class entry with selection weight for random dungeon room generation.
 */
USTRUCT(BlueprintType)
struct FFCDungeonRoomWeight
{
	GENERATED_BODY()

	/** Room actor class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Dungeon")
	TSubclassOf<AFCRoomBase> RoomClass = nullptr;

	/** Selection weight (higher value = higher chance). Must be >= 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Dungeon", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Weight = 1.0f;

	FFCDungeonRoomWeight()
		: RoomClass(nullptr)
		, Weight(1.0f)
	{}

	FFCDungeonRoomWeight(TSubclassOf<AFCRoomBase> InRoomClass, float InWeight = 1.0f)
		: RoomClass(InRoomClass)
		, Weight(InWeight)
	{}
};

UENUM(BlueprintType)
enum class EFCRoomType : uint8
{
	Start,
	Normal,
	Shop,
	Treasure,
	Boss,
	Secret
};

UENUM(BlueprintType)
enum class EFCRoomState : uint8
{
	Unvisited,
	Active,
	Cleared
};

UENUM(BlueprintType)
enum class EFCDungeonLayoutPattern : uint8
{
	Compact    UMETA(DisplayName = "Compact (Square Cluster)"),
	Asymmetric UMETA(DisplayName = "Asymmetric (Irregular & Branching)"),
	Sprawling  UMETA(DisplayName = "Sprawling (Long Winding Arms)")
};

UENUM(BlueprintType)
enum class EFCDungeonStartLocation : uint8
{
	Center UMETA(DisplayName = "Center (Middle of Dungeon)"),
	Edge   UMETA(DisplayName = "Edge / End (Outer Perimeter)")
};

namespace EFCDungeonDoor
{
	constexpr uint8 North = 1 << 0; // 1
	constexpr uint8 South = 1 << 1; // 2
	constexpr uint8 East  = 1 << 2; // 4
	constexpr uint8 West  = 1 << 3; // 8
}

USTRUCT(BlueprintType)
struct FFCRoomData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "FC|Dungeon")
	FIntPoint Coordinates;

	UPROPERTY(BlueprintReadWrite, Category = "FC|Dungeon")
	EFCRoomType RoomType;

	// Bitmask for doors: North(1), South(2), East(4), West(8)
	UPROPERTY(BlueprintReadWrite, Category = "FC|Dungeon")
	uint8 DoorMask;

	// Bitmask for secret entrances: North(1), South(2), East(4), West(8)
	UPROPERTY(BlueprintReadWrite, Category = "FC|Dungeon")
	uint8 SecretDoorMask;

	// Distance from start room (used for boss placement)
	UPROPERTY(BlueprintReadWrite, Category = "FC|Dungeon")
	int32 DistanceFromStart;

	FFCRoomData()
		: Coordinates(ForceInitToZero)
		, RoomType(EFCRoomType::Normal)
		, DoorMask(0)
		, SecretDoorMask(0)
		, DistanceFromStart(0)
	{}

	bool HasNorthDoor() const { return (DoorMask & EFCDungeonDoor::North) != 0; }
	bool HasSouthDoor() const { return (DoorMask & EFCDungeonDoor::South) != 0; }
	bool HasEastDoor()  const { return (DoorMask & EFCDungeonDoor::East)  != 0; }
	bool HasWestDoor()  const { return (DoorMask & EFCDungeonDoor::West)  != 0; }

	bool HasNorthSecretDoor() const { return (SecretDoorMask & EFCDungeonDoor::North) != 0; }
	bool HasSouthSecretDoor() const { return (SecretDoorMask & EFCDungeonDoor::South) != 0; }
	bool HasEastSecretDoor()  const { return (SecretDoorMask & EFCDungeonDoor::East)  != 0; }
	bool HasWestSecretDoor()  const { return (SecretDoorMask & EFCDungeonDoor::West)  != 0; }
};
