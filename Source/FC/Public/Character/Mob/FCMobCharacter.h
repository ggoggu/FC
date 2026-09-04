#pragma once

#include "CoreMinimal.h"
#include "Character/FCCharacterBase.h"
#include "AI/FCAITypes.h"
#include "FCMobCharacter.generated.h"

class AFCMobAIController;
class AFCMobSpawnerBase;

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

	/** Delay in seconds before dead mob actor is destroyed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Combat", meta = (ClampMin = "0.0"))
	float DeathDespawnDelay = 5.0f;

	/** Weak reference to the spawner that produced this mob */
	UPROPERTY(Transient)
	TWeakObjectPtr<AFCMobSpawnerBase> OwningSpawner;
};

