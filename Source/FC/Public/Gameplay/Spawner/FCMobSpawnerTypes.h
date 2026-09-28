#pragma once

#include "CoreMinimal.h"
#include "FCMobSpawnerTypes.generated.h"

class AFCMobCharacter;
class AFCMobSpawnerBase;

/**
 * Strategy for selecting spawn transform/coordinates
 */
UENUM(BlueprintType)
enum class EFCMobSpawnLocationType : uint8
{
	RandomInRadius UMETA(DisplayName = "Random In Radius"),
	AtSpawner UMETA(DisplayName = "At Spawner"),
	DesignatedSpawnPoints UMETA(DisplayName = "Designated Spawn Points")
};

/**
 * Entry defining an enemy class and its weighted selection probability
 */
USTRUCT(BlueprintType)
struct FFCMobSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawn")
	TSubclassOf<AFCMobCharacter> MobClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawn", meta = (ClampMin = "0.01"))
	float Weight = 1.0f;
};

/**
 * Slot defining an exact mob class and an exact spawn location/transform
 */
USTRUCT(BlueprintType)
struct FFCFixedMobSpawnSlot
{
	GENERATED_BODY()

	/** Specific mob class to spawn in this slot. If null, uses the spawner's DefaultMobClass */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawn")
	TSubclassOf<AFCMobCharacter> MobClass;

	/** Transform offset relative to the spawner actor (editable via 3D widget in editor) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawn", meta = (MakeEditWidget = true))
	FTransform RelativeTransform = FTransform::Identity;

	/** Optional target actor in the level whose transform will override RelativeTransform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawn")
	TObjectPtr<AActor> TargetSpawnActor = nullptr;

	/** Optional tag to identify or group this slot (e.g. "Archer", "BossGuard") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawn")
	FName SlotTag = NAME_None;

	/** If true, this slot automatically respawns after the mob dies */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawn")
	bool bRespawnOnDeath = false;

	/** Delay in seconds before respawning this slot after death */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawn", meta = (EditCondition = "bRespawnOnDeath", ClampMin = "0.1"))
	float RespawnDelay = 5.0f;
};

// =============================================================================
// Spawner Event Delegates
// =============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFCOnMobSpawnedSignature, AFCMobCharacter*, SpawnedMob);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFCOnMobDiedSignature, AFCMobCharacter*, DeadMob, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFCOnAllMobsDefeatedSignature, AFCMobSpawnerBase*, Spawner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFCOnCampReplenishStartedSignature, int32, NeededCount, float, DelaySeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFCOnCampRespawnStartedSignature, float, DelaySeconds, int32, NewTargetCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFCOnFixedSlotSpawnedSignature, int32, SlotIndex, AFCMobCharacter*, SpawnedMob);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFCOnFixedSlotMobDiedSignature, int32, SlotIndex, AFCMobCharacter*, DeadMob, AActor*, Killer);
