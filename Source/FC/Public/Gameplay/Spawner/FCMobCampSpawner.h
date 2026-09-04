#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Spawner/FCMobSpawnerBase.h"
#include "FCMobCampSpawner.generated.h"

/**
 * AFCMobCampSpawner
 * 
 * Specialized spawner that maintains enemy population within a min/max range.
 * Automatically replenishes deficit after a randomized delay when count drops below minimum,
 * and respawns the entire group after a randomized delay when all enemies are wiped out.
 * Also supports territory leash monitoring to ensure mobs do not stray beyond camp radius.
 */
UCLASS(BlueprintType, Blueprintable)
class FC_API AFCMobCampSpawner : public AFCMobSpawnerBase
{
	GENERATED_BODY()

public:
	AFCMobCampSpawner();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void StartSpawning() override;
	virtual void StopSpawning() override;
	virtual void ResetSpawner(bool bDestroyActiveMobs = true) override;

	virtual void HandleMobDied(AFCMobCharacter* DeadMob, AActor* Killer) override;
	virtual void HandleMobDestroyed(AFCMobCharacter* DestroyedMob) override;

	/** Manually triggers deficit replenishment */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Camp")
	void ForceReplenishDeficit();

	/** Manually triggers immediate full respawn */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Camp")
	void ForceFullRespawn();

	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Camp")
	int32 GetMinMobCount() const { return MinMobCount; }

	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Camp")
	int32 GetMaxMobCount() const { return MaxMobCount; }

	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Camp")
	int32 GetTargetMobCount() const { return TargetMobCount; }

	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Camp")
	bool IsWipeRespawnPending() const;

	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Camp")
	bool IsReplenishPending() const;

	// =========================================================================
	// Event Delegates
	// =========================================================================

	UPROPERTY(BlueprintAssignable, Category = "FC|Spawner|Camp")
	FFCOnCampReplenishStartedSignature OnCampReplenishStarted;

	UPROPERTY(BlueprintAssignable, Category = "FC|Spawner|Camp")
	FFCOnCampRespawnStartedSignature OnCampRespawnStarted;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Internal handler to check if replenish or wipe respawn is needed */
	void EvaluatePopulationState();

	/** Respawns the full camp population to a newly rolled target count */
	UFUNCTION()
	void RespawnFullCamp();

	/** Replenishes deficit count up to TargetMobCount */
	UFUNCTION()
	void ReplenishDeficit();

	/** Periodic check for mobs straying outside CampTerritoryRadius */
	UFUNCTION()
	void CheckTerritoryLeash();

	// =========================================================================
	// Population Range Configuration
	// =========================================================================

	/** Minimum number of living mobs to maintain in this camp */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Camp|Population", meta = (ClampMin = "1"))
	int32 MinMobCount = 3;

	/** Maximum number of living mobs to maintain in this camp */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Camp|Population", meta = (ClampMin = "1"))
	int32 MaxMobCount = 5;

	/** Currently assigned target mob count for this cycle */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FC|Spawner|Camp|Population")
	int32 TargetMobCount = 3;

	// =========================================================================
	// Respawn & Replenishment Timing Ranges
	// =========================================================================

	/** Whether to automatically replenish when count falls below MinMobCount */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Camp|Timing")
	bool bAutoReplenishDeficit = true;

	/** Minimum wait time in seconds before replenishing deficit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Camp|Timing", meta = (ClampMin = "0.1"))
	float MinReplenishDelay = 5.0f;

	/** Maximum wait time in seconds before replenishing deficit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Camp|Timing", meta = (ClampMin = "0.1"))
	float MaxReplenishDelay = 10.0f;

	/** Whether to respawn camp after all mobs are wiped out */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Camp|Timing")
	bool bRespawnOnWipe = true;

	/** Minimum wait time in seconds before full camp respawns after wipeout */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Camp|Timing", meta = (ClampMin = "0.1"))
	float MinWipeRespawnDelay = 10.0f;

	/** Maximum wait time in seconds before full camp respawns after wipeout */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Camp|Timing", meta = (ClampMin = "0.1"))
	float MaxWipeRespawnDelay = 20.0f;

	// =========================================================================
	// Territory & Leash Configuration
	// =========================================================================

	/** Whether to enforce distance leash around camp */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Camp|Territory")
	bool bEnableTerritoryLeash = true;

	/** Maximum allowed distance in cm from spawner before a mob is considered out of bounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Camp|Territory", meta = (ClampMin = "500.0"))
	float CampTerritoryRadius = 2500.0f;

	/** Interval in seconds between territory leash checks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Camp|Territory", meta = (ClampMin = "0.5"))
	float LeashCheckInterval = 3.0f;

	// =========================================================================
	// Timer Handles
	// =========================================================================

	FTimerHandle ReplenishTimerHandle;
	FTimerHandle WipeRespawnTimerHandle;
	FTimerHandle LeashCheckTimerHandle;
};
