#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Spawner/FCMobSpawnerBase.h"
#include "Gameplay/Spawner/FCMobSpawnerTypes.h"
#include "FCFixedMobSpawner.generated.h"

/**
 * AFCFixedMobSpawner
 * 
 * Deterministic mob spawner that spawns exact mob types at exact locations/transforms for an exact count.
 * Ideal for dungeons, encounters, boss rooms, and scripted guard placements.
 * Supports per-slot mob class assignment, 3D viewport widget transform editing,
 * optional world actor targets, and independent slot respawn logic.
 */
UCLASS(BlueprintType, Blueprintable)
class FC_API AFCFixedMobSpawner : public AFCMobSpawnerBase
{
	GENERATED_BODY()

public:
	AFCFixedMobSpawner();

	virtual void StartSpawning() override;
	virtual void StopSpawning() override;
	virtual void ResetSpawner(bool bDestroyActiveMobs = true) override;
	virtual void DespawnAllMobs() override;

	virtual void HandleMobDied(AFCMobCharacter* DeadMob, AActor* Killer) override;
	virtual void HandleMobDestroyed(AFCMobCharacter* DestroyedMob) override;

	// =========================================================================
	// Fixed Spawning Commands
	// =========================================================================

	/**
	 * Spawns mobs in all configured slots adhering to their defined transforms and classes.
	 * Exactly matches the count of valid slots configured.
	 * @param bForceRespawn If true, despawns existing living mobs in occupied slots before respawning.
	 * @return The number of mobs successfully spawned in this invocation.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Fixed")
	int32 SpawnAllSlots(bool bForceRespawn = false);

	/**
	 * Spawns a mob at the designated slot index.
	 * @param SlotIndex Zero-based index into SpawnSlots array.
	 * @param bForceRespawn If true, despawns the existing mob in this slot before spawning a new one.
	 * @return Pointer to the spawned mob character, or nullptr on failure.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Fixed")
	AFCMobCharacter* SpawnSlot(int32 SlotIndex, bool bForceRespawn = false);

	/**
	 * Spawns mobs for all slots bearing the specified SlotTag.
	 * @param SlotTag Identifying tag on slots to trigger.
	 * @param bForceRespawn If true, forces respawn even if already alive.
	 * @return Number of mobs spawned.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Fixed")
	int32 SpawnSlotsByTag(FName SlotTag, bool bForceRespawn = false);

	/**
	 * Despawns the living mob belonging to a specific slot index.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Fixed")
	void DespawnSlot(int32 SlotIndex);

	/**
	 * Despawns all living mobs in slots matching the specified tag.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Fixed")
	void DespawnSlotsByTag(FName SlotTag);

	// =========================================================================
	// Slot Queries & State
	// =========================================================================

	/** Returns total number of defined fixed slots (the exact target count) */
	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Fixed")
	int32 GetSlotCount() const { return SpawnSlots.Num(); }

	/** Retrieves the living mob at the specified slot index (or nullptr if dead/unspawned) */
	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Fixed")
	AFCMobCharacter* GetLivingSlotMob(int32 SlotIndex) const;

	/** Checks if a slot currently has an alive mob */
	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Fixed")
	bool IsSlotAlive(int32 SlotIndex) const;

	/** Checks if a slot has a pending respawn timer running */
	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Fixed")
	bool IsSlotPendingRespawn(int32 SlotIndex) const;

	/** Calculates world transform for the given slot */
	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Fixed")
	bool GetSlotWorldTransform(int32 SlotIndex, FTransform& OutTransform) const;

	/** Returns index of slot that owns the given mob, or INDEX_NONE */
	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Fixed")
	int32 FindSlotIndexForMob(const AFCMobCharacter* Mob) const;

	/** Returns all configured spawn slots */
	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Fixed")
	const TArray<FFCFixedMobSpawnSlot>& GetSpawnSlots() const { return SpawnSlots; }

	// =========================================================================
	// Slot Configuration Modification (Runtime / BP)
	// =========================================================================

	/** Adds a new fixed spawn slot dynamically */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Fixed")
	int32 AddSpawnSlot(const FFCFixedMobSpawnSlot& NewSlot);

	/** Clears all slots and cleans up living mobs */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Fixed")
	void ClearSpawnSlots();

	// =========================================================================
	// Event Delegates
	// =========================================================================

	UPROPERTY(BlueprintAssignable, Category = "FC|Spawner|Fixed")
	FFCOnFixedSlotSpawnedSignature OnFixedSlotSpawned;

	UPROPERTY(BlueprintAssignable, Category = "FC|Spawner|Fixed")
	FFCOnFixedSlotMobDiedSignature OnFixedSlotMobDied;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Internal helper to spawn at a specific slot without redundant authority checks */
	AFCMobCharacter* InternalSpawnSlotMob(int32 SlotIndex);

	/** Callback when an individual slot respawn timer fires */
	UFUNCTION()
	void HandleSlotRespawnTimer(int32 SlotIndex);

	/** Synchronizes internal slot tracking arrays to match SpawnSlots size */
	void EnsureTrackingArraysSized();

	// =========================================================================
	// Configuration
	// =========================================================================

	/**
	 * List of deterministic spawn slots.
	 * Each slot specifies the exact mob class and exact location (relative offset or target actor).
	 * RelativeTransform can be manipulated directly in the editor viewport using 3D widget.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Fixed")
	TArray<FFCFixedMobSpawnSlot> SpawnSlots;

	/** Fallback mob class if a slot's MobClass is null */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Fixed")
	TSubclassOf<AFCMobCharacter> DefaultMobClass;

	/** If true, automatically spawns all slots when spawner starts (BeginPlay auto-start or StartSpawning) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Fixed")
	bool bSpawnAllOnStart = true;

	/** If true, clearing/stopping the spawner also cancels pending respawn timers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Fixed")
	bool bClearTimersOnStop = true;

	// =========================================================================
	// Runtime Tracking
	// =========================================================================

	/** Array mapping slot index to living mob pointer */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AFCMobCharacter>> SlotLivingMobs;

	/** Array of respawn timer handles per slot */
	TArray<FTimerHandle> SlotRespawnTimers;
};
