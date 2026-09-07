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

// =============================================================================
// Spawner Event Delegates
// =============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFCOnMobSpawnedSignature, AFCMobCharacter*, SpawnedMob);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFCOnMobDiedSignature, AFCMobCharacter*, DeadMob, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFCOnAllMobsDefeatedSignature, AFCMobSpawnerBase*, Spawner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFCOnCampReplenishStartedSignature, int32, NeededCount, float, DelaySeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFCOnCampRespawnStartedSignature, float, DelaySeconds, int32, NewTargetCount);
