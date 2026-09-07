#pragma once

#include "CoreMinimal.h"
#include "Character/FCCharacterBase.h"
#include "AI/FCAITypes.h"
class AFCMobCharacter;
class AFCMobAIController;
class AFCMobSpawnerBase;
class AFCProjectileBase;
class UFCProjectileDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFCOnChestDroppedSignature, AFCMobCharacter*, DeadMob, AActor*, DroppedChest);

#include "FCMobCharacter.generated.h"

/**
 * Humanoid Mob Character with automated AI navigation and replication
 */
UCLASS()
class FC_API AFCMobCharacter : public AFCCharacterBase
{
	GENERATED_BODY()

public:
	AFCMobCharacter();

	virtual void Die(AActor* Killer = nullptr) override;

	virtual void HandleDamageTaken(float DamageAmount, AActor* DamageCauser, const FHitResult& HitResult) override;

	/** Plays directional death animation on local mesh */
	UFUNCTION(BlueprintCallable, Category = "FC|Mob|Combat")
	void PlayDeathAnimation(EFCDeathDirection Direction);

	/** Replicated multicast RPC to play cosmetic death animation across network */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDeathAnimation(EFCDeathDirection Direction);

	UAnimSequence* GetDeathAnimationForDirection(EFCDeathDirection Direction) const;

	/** Plays directional hit reaction animation on local mesh */
	UFUNCTION(BlueprintCallable, Category = "FC|Mob|Combat")
	void PlayHitAnimation(EFCDeathDirection Direction);

	/** Replicated multicast RPC to play cosmetic hit animation across network */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayHitAnimation(EFCDeathDirection Direction);

	UAnimSequence* GetHitAnimationForDirection(EFCDeathDirection Direction) const;

	/** Sets movement speed to arbitrary value */
	UFUNCTION(BlueprintCallable, Category = "FC|Mob|Movement")
	void SetMovementSpeed(float NewSpeed);

	/** Switch movement speed to patrol speed */
	UFUNCTION(BlueprintCallable, Category = "FC|Mob|Movement")
	void SetPatrolSpeed();

	/** Switch movement speed to chase speed */
	UFUNCTION(BlueprintCallable, Category = "FC|Mob|Movement")
	void SetChaseSpeed();

	float GetPatrolSpeed() const { return PatrolSpeed; }
	float GetChaseSpeed() const { return ChaseSpeed; }

	void SetOwningSpawner(AFCMobSpawnerBase* InSpawner);
	AFCMobSpawnerBase* GetOwningSpawner() const;

	float GetDeathDespawnDelay() const { return DeathDespawnDelay; }
	void SetDeathDespawnDelay(float InDelay) { DeathDespawnDelay = FMath::Max(0.0f, InDelay); }

	float GetInitialMaxHealth() const { return InitialMaxHealth; }
	void SetInitialMaxHealth(float InHealth) { InitialMaxHealth = FMath::Max(1.0f, InHealth); }

	float GetHealthRewardOnKill() const { return HealthRewardOnKill; }
	void SetHealthRewardOnKill(float InReward) { HealthRewardOnKill = FMath::Max(0.0f, InReward); }

	TSubclassOf<AActor> GetDropChestClass() const { return DropChestClass; }
	void SetDropChestClass(TSubclassOf<AActor> InClass) { DropChestClass = InClass; }

	float GetDropChestChance() const { return DropChestChance; }
	void SetDropChestChance(float InChance) { DropChestChance = FMath::Clamp(InChance, 0.0f, 1.0f); }

	/** Rolls chance and spawns drop chest if successful (Server authority only) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Mob|Drop")
	AActor* AttemptDropChest();

	/** Spawns configured drop chest actor in the world (Server authority only) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Mob|Drop")
	AActor* SpawnDropChest();

	/** Multicast delegate fired when a chest is successfully dropped upon death */
	UPROPERTY(BlueprintAssignable, Category = "FC|Mob|Drop")
	FFCOnChestDroppedSignature OnChestDropped;

	/** Attempts to activate the mob's configured attack gameplay ability */
	UFUNCTION(BlueprintCallable, Category = "FC|Mob|Combat")
	bool TryActivateAttackAbility();

	float GetAttackRange() const { return AttackRange; }
	float GetAttackCooldown() const { return AttackCooldown; }
	TSubclassOf<class UGameplayAbility> GetAttackAbilityClass() const { return AttackAbilityClass; }
	void SetAttackAbilityClass(TSubclassOf<class UGameplayAbility> InAbilityClass) { AttackAbilityClass = InAbilityClass; }
	UAnimMontage* GetAttackMontage() const { return AttackMontage; }
	void SetAttackMontage(UAnimMontage* InMontage) { AttackMontage = InMontage; }

	TSubclassOf<AFCProjectileBase> GetProjectileClassOverride() const { return ProjectileClassOverride; }
	void SetProjectileClassOverride(TSubclassOf<AFCProjectileBase> InClass) { ProjectileClassOverride = InClass; }

	UFCProjectileDataAsset* GetProjectileDataAssetOverride() const { return ProjectileDataAssetOverride; }
	void SetProjectileDataAssetOverride(UFCProjectileDataAsset* InDataAsset) { ProjectileDataAssetOverride = InDataAsset; }

	/** Current Mob AI State */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Mob")
	EFCMobAIState CurrentAIState = EFCMobAIState::Idle;

	UFUNCTION(BlueprintCallable, Category = "FC|Mob")
	void SetAIState(EFCMobAIState NewState) { CurrentAIState = NewState; }

	UFUNCTION(BlueprintPure, Category = "FC|Mob")
	EFCMobAIState GetAIState() const { return CurrentAIState; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void InitAbilityActorInfo();

	/** Speed when casually patrolling / wandering */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Movement", meta = (ClampMin = "0.0"))
	float PatrolSpeed = 250.0f;

	/** Speed when chasing perceived targets */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Movement", meta = (ClampMin = "0.0"))
	float ChaseSpeed = 500.0f;

	/** Primary attack ability granted to this mob */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Mob|Combat")
	TSubclassOf<class UGameplayAbility> AttackAbilityClass;

	/** Optional Projectile Class override (e.g. BP_FCProjectile_Orb). If set, this BP projectile will be spawned instead of the C++ default */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Mob|Combat")
	TSubclassOf<AFCProjectileBase> ProjectileClassOverride;

	/** Optional Projectile Data Asset override (e.g. DA_Projectile_Fireball) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Mob|Combat")
	TObjectPtr<UFCProjectileDataAsset> ProjectileDataAssetOverride;

	/** Maximum distance in cm at which this mob can initiate its ranged/melee attack */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Combat", meta = (ClampMin = "50.0"))
	float AttackRange = 700.0f;

	/** Cooldown in seconds between attacks */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Combat", meta = (ClampMin = "0.1"))
	float AttackCooldown = 2.0f;

	/** Optional attack animation montage to play when casting / attacking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Mob|Combat")
	TObjectPtr<UAnimMontage> AttackMontage;

	/** Initial maximum health for this mob (applied to AttributeSet on BeginPlay) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Mob|Stats", meta = (ClampMin = "1.0"))
	float InitialMaxHealth = 100.0f;

	/** Delay in seconds before dead mob actor is destroyed (0.0 = immediate destroy) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Mob|Combat", meta = (ClampMin = "0.0"))
	float DeathDespawnDelay = 5.0f;

	/** Blueprint class of the treasure chest to spawn on death */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Mob|Drop")
	TSubclassOf<AActor> DropChestClass;

	/** Health restored to player killer upon this mob's death (default: 5.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Mob|Drop", meta = (ClampMin = "0.0"))
	float HealthRewardOnKill = 5.0f;

	/** Probability of dropping chest on death (0.0 = 0%, 0.5 = 50%, 1.0 = 100%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Mob|Drop", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DropChestChance = 1.0f;

	/** World offset applied to the spawned chest position */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Mob|Drop")
	FVector DropChestOffset = FVector::ZeroVector;

	/** If true, snaps chest spawn location to the ground beneath the mob */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Mob|Drop")
	bool bSnapChestToGround = true;

	/** Directional death animations from Character/Mannequins/Anims/Death */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Front;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Front_02;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Front_03;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Back;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Right;

	/** Optional hit animation montage override */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimMontage> HitMontage;

	/** Directional hit animations (defaults to Death animations from Character/Mannequins/Anims/Death) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimSequence> HitAnim_Front;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimSequence> HitAnim_Back;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimSequence> HitAnim_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimSequence> HitAnim_Right;

	/** Play rate for hit animation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation", meta = (ClampMin = "0.1"))
	float HitPlayRate = 1.5f;

	/** Minimum time in seconds between playing hit reaction animations */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Combat", meta = (ClampMin = "0.0"))
	float HitReactionCooldown = 0.25f;

	/** Timestamp of last played hit reaction */
	float LastHitReactTime = -100.0f;

	/** Weak reference to the spawner that produced this mob */
	UPROPERTY(Transient)
	TWeakObjectPtr<AFCMobSpawnerBase> OwningSpawner;
};

