#pragma once

#include "CoreMinimal.h"
#include "Character/FCCharacterBase.h"
#include "AI/FCAITypes.h"
#include "FCMobCharacter.generated.h"

class AFCMobAIController;
class AFCMobSpawnerBase;

UENUM(BlueprintType)
enum class EFCDeathDirection : uint8
{
	Front UMETA(DisplayName = "Front"),
	Back UMETA(DisplayName = "Back"),
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right")
};

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

	/** Calculates relative hit direction from an instigator actor (Front, Back, Left, Right) */
	UFUNCTION(BlueprintCallable, Category = "FC|Mob|Combat")
	EFCDeathDirection CalculateHitDirection(AActor* InstigatorActor) const;

	/** Plays directional death animation on local mesh */
	UFUNCTION(BlueprintCallable, Category = "FC|Mob|Combat")
	void PlayDeathAnimation(EFCDeathDirection Direction);

	/** Replicated multicast RPC to play cosmetic death animation across network */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDeathAnimation(EFCDeathDirection Direction);

	UAnimSequence* GetDeathAnimationForDirection(EFCDeathDirection Direction) const;

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

	/** Attempts to activate the mob's configured attack gameplay ability */
	UFUNCTION(BlueprintCallable, Category = "FC|Mob|Combat")
	bool TryActivateAttackAbility();

	float GetAttackRange() const { return AttackRange; }
	float GetAttackCooldown() const { return AttackCooldown; }
	TSubclassOf<class UGameplayAbility> GetAttackAbilityClass() const { return AttackAbilityClass; }
	void SetAttackAbilityClass(TSubclassOf<class UGameplayAbility> InAbilityClass) { AttackAbilityClass = InAbilityClass; }
	UAnimMontage* GetAttackMontage() const { return AttackMontage; }
	void SetAttackMontage(UAnimMontage* InMontage) { AttackMontage = InMontage; }

	/** Sets the current combat target actor for 3D pitch aiming and focus */
	UFUNCTION(BlueprintCallable, Category = "FC|Mob|Combat")
	void SetCombatTarget(AActor* InTarget) { CombatTarget = InTarget; }

	UFUNCTION(BlueprintPure, Category = "FC|Mob|Combat")
	AActor* GetCombatTarget() const { return CombatTarget.Get(); }

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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Mob|Combat")
	TSubclassOf<class UGameplayAbility> AttackAbilityClass;

	/** Maximum distance in cm at which this mob can initiate its ranged/melee attack */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Combat", meta = (ClampMin = "50.0"))
	float AttackRange = 700.0f;

	/** Cooldown in seconds between attacks */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Combat", meta = (ClampMin = "0.1"))
	float AttackCooldown = 2.0f;

	/** Optional attack animation montage to play when casting / attacking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Mob|Combat")
	TObjectPtr<UAnimMontage> AttackMontage;

	/** Delay in seconds before dead mob actor is destroyed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Combat", meta = (ClampMin = "0.0"))
	float DeathDespawnDelay = 5.0f;

	/** Directional death animations (Front, Back, Left, Right) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Front;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Back;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Right;

	/** Weak reference to the spawner that produced this mob */
	UPROPERTY(Transient)
	TWeakObjectPtr<AFCMobSpawnerBase> OwningSpawner;

	/** Transient reference to active combat target for 3D aim direction calculation */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CombatTarget;
};

