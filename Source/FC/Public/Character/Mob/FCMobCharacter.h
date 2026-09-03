#pragma once

#include "CoreMinimal.h"
#include "Character/FCCharacterBase.h"
#include "FCMobCharacter.generated.h"

class AFCMobAIController;

/**
 * Humanoid Mob Character with automated AI navigation and replication
 */
UCLASS()
class FC_API AFCMobCharacter : public AFCCharacterBase
{
	GENERATED_BODY()

public:
	AFCMobCharacter();

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

protected:
	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo();

	/** Speed when casually patrolling / wandering */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Movement", meta = (ClampMin = "0.0"))
	float PatrolSpeed = 250.0f;

	/** Speed when chasing perceived targets */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Mob|Movement", meta = (ClampMin = "0.0"))
	float ChaseSpeed = 500.0f;
};
