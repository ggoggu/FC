#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/Spawner/FCMobSpawnerTypes.h"
#include "FCMobSpawnerBase.generated.h"

class AFCMobCharacter;
class USceneComponent;
class UBillboardComponent;
class USphereComponent;

/**
 * AFCMobSpawnerBase
 * 
 * Server-authoritative base spawner class providing networked lifecycle management,
 * NavMesh spawn coordinate calculation, active mob tracking, and cosmetic event multicast.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class FC_API AFCMobSpawnerBase : public AActor
{
	GENERATED_BODY()

public:
	AFCMobSpawnerBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Starts spawner activation and timers (Server authority only) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner")
	virtual void StartSpawning();

	/** Stops/pauses spawner (Server authority only) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner")
	virtual void StopSpawning();

	/** Resets spawner state and optionally destroys living mobs (Server authority only) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner")
	virtual void ResetSpawner(bool bDestroyActiveMobs = true);

	/** Immediately destroys all active mobs belonging to this spawner */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner")
	virtual void DespawnAllMobs();

	/** Spawns a single mob actor adhering to location strategy (Server authority only) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Spawner")
	virtual AFCMobCharacter* SpawnSingleMob(TSubclassOf<AFCMobCharacter> MobClassOverride = nullptr);

	/** Callback invoked by AFCMobCharacter when it dies */
	virtual void HandleMobDied(AFCMobCharacter* DeadMob, AActor* Killer);

	/** Callback invoked by AFCMobCharacter when actor is destroyed / EndPlay */
	virtual void HandleMobDestroyed(AFCMobCharacter* DestroyedMob);

	/** Evaluates whether spawner is in a valid state to spawn an enemy */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FC|Spawner")
	bool CanSpawnMob() const;
	virtual bool CanSpawnMob_Implementation() const;

	/** Queries currently alive mobs */
	UFUNCTION(BlueprintPure, Category = "FC|Spawner")
	void GetActiveMobs(TArray<AFCMobCharacter*>& OutMobs) const;

	UFUNCTION(BlueprintPure, Category = "FC|Spawner")
	bool IsSpawnerActive() const { return bIsActive; }

	UFUNCTION(BlueprintPure, Category = "FC|Spawner")
	int32 GetActiveMobCount() const { return ActiveMobCount; }

	UFUNCTION(BlueprintPure, Category = "FC|Spawner")
	float GetSpawnRadius() const { return SpawnRadius; }

	// =========================================================================
	// Event Delegates
	// =========================================================================

	UPROPERTY(BlueprintAssignable, Category = "FC|Spawner")
	FFCOnMobSpawnedSignature OnMobSpawned;

	UPROPERTY(BlueprintAssignable, Category = "FC|Spawner")
	FFCOnMobDiedSignature OnMobDied;

	UPROPERTY(BlueprintAssignable, Category = "FC|Spawner")
	FFCOnAllMobsDefeatedSignature OnAllMobsDefeated;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Selects a random spawn location using configured strategy and NavMesh */
	virtual bool GetRandomSpawnLocation(FVector& OutLocation) const;

	/** Selects a mob class based on entry weights */
	virtual TSubclassOf<AFCMobCharacter> SelectMobClass() const;

	/** Updates replicated ActiveMobCount from ActiveMobs array */
	void SynchronizeActiveMobCount();

	/** Multicast cosmetic cue to play particle/sound on clients */
	UFUNCTION(NetMulticast, Unreliable, Category = "FC|Spawner")
	void Multicast_PlaySpawnEffect(const FVector& SpawnLocation);

	UFUNCTION()
	virtual void OnRep_IsActive();

	UFUNCTION()
	virtual void OnRep_ActiveMobCount();

	// =========================================================================
	// Components & Visuals
	// =========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Spawner|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Spawner|Components")
	TObjectPtr<UBillboardComponent> EditorBillboard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Spawner|Components")
	TObjectPtr<USphereComponent> SpawnRadiusVisualizer;

	// =========================================================================
	// Configuration
	// =========================================================================

	/** Strategy for spawn positioning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Config")
	EFCMobSpawnLocationType LocationType = EFCMobSpawnLocationType::RandomInRadius;

	/** Radius in cm within which mobs can spawn around this spawner */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Config", meta = (ClampMin = "50.0"))
	float SpawnRadius = 800.0f;

	/** If true, projects spawn coordinates to nearest reachable NavMesh point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Config")
	bool bProjectToNavigation = true;

	/** Pool of enemy classes and weights to pick from */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Config")
	TArray<FFCMobSpawnEntry> MobEntries;

	/** Designated actors/points used if LocationType is DesignatedSpawnPoints */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Config")
	TArray<TObjectPtr<AActor>> DesignatedSpawnPoints;

	/** Whether spawner begins automatically on BeginPlay after InitialDelay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Config")
	bool bAutoStart = true;

	/** Delay in seconds before auto-start triggers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Spawner|Config", meta = (ClampMin = "0.0"))
	float InitialDelay = 1.0f;

	// =========================================================================
	// State & Tracking
	// =========================================================================

	UPROPERTY(ReplicatedUsing = OnRep_IsActive, BlueprintReadOnly, Category = "FC|Spawner|State")
	bool bIsActive = false;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveMobCount, BlueprintReadOnly, Category = "FC|Spawner|State")
	int32 ActiveMobCount = 0;

	/** Weak references to living mobs spawned by this spawner */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AFCMobCharacter>> ActiveMobs;

	/** Timer handle for initial delayed activation */
	FTimerHandle InitialDelayTimerHandle;
};
