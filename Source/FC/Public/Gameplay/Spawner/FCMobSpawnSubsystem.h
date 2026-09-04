#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FCMobSpawnSubsystem.generated.h"

class AFCMobSpawnerBase;

/**
 * UFCMobSpawnSubsystem
 * 
 * World subsystem that tracks and coordinates all active AFCMobSpawnerBase actors in the level.
 * Provides tag-based query, global mob population statistics, and batch spawner commands.
 */
UCLASS()
class FC_API UFCMobSpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Helper to obtain subsystem instance from any world context object */
	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Subsystem", meta = (WorldContext = "WorldContextObject"))
	static UFCMobSpawnSubsystem* Get(const UObject* WorldContextObject);

	/** Registers an active spawner into tracking registry */
	UFUNCTION(BlueprintCallable, Category = "FC|Spawner|Subsystem")
	void RegisterSpawner(AFCMobSpawnerBase* Spawner);

	/** Unregisters a spawner from tracking registry */
	UFUNCTION(BlueprintCallable, Category = "FC|Spawner|Subsystem")
	void UnregisterSpawner(AFCMobSpawnerBase* Spawner);

	/** Returns all currently registered spawners in this world */
	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Subsystem")
	void GetAllSpawners(TArray<AFCMobSpawnerBase*>& OutSpawners) const;

	/** Finds all spawners bearing the specified Actor Tag */
	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Subsystem")
	void FindSpawnersByTag(FName Tag, TArray<AFCMobSpawnerBase*>& OutSpawners) const;

	/** Computes total active living mobs across all registered spawners in this world */
	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Subsystem")
	int32 GetGlobalActiveMobCount() const;

	/** Computes total active living mobs across spawners with a specific Actor Tag */
	UFUNCTION(BlueprintPure, Category = "FC|Spawner|Subsystem")
	int32 GetActiveMobCountByTag(FName Tag) const;

	/** Activates all registered spawners (Server only) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Subsystem")
	void ActivateAllSpawners();

	/** Deactivates all registered spawners (Server only) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Subsystem")
	void DeactivateAllSpawners();

	/** Resets all registered spawners and optionally clears active mobs (Server only) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Subsystem")
	void ResetAllSpawners(bool bDestroyActiveMobs = true);

	/** Activates all spawners with the given tag (Server only) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Subsystem")
	void ActivateSpawnersByTag(FName Tag);

	/** Deactivates all spawners with the given tag (Server only) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Subsystem")
	void DeactivateSpawnersByTag(FName Tag);

	/** Resets all spawners with the given tag (Server only) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Subsystem")
	void ResetSpawnersByTag(FName Tag, bool bDestroyActiveMobs = true);

	/** Destroys all living mobs currently spawned across all spawners */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner|Subsystem")
	void DespawnAllWorldMobs();

protected:
	/** Collection of active spawners in the world */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AFCMobSpawnerBase>> RegisteredSpawners;
};
