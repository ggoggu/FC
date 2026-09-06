#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "FCBTDecorator_IsInAttackRange.generated.h"

/**
 * Behavior Tree decorator that checks whether the target actor is within attack range
 * and optionally verifies line of sight.
 */
UCLASS()
class FC_API UFCBTDecorator_IsInAttackRange : public UBTDecorator
{
	GENERATED_BODY()

public:
	UFCBTDecorator_IsInAttackRange();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;

protected:
	/** Blackboard key selector for target actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI")
	FBlackboardKeySelector TargetActorKey;

	/** If > 0, overrides the Mob's configured AttackRange. If <= 0, queries AFCMobCharacter::GetAttackRange() */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI", meta = (ClampMin = "0.0"))
	float AttackRangeOverride = 0.0f;

	/** If true, verifies line of sight with a raycast so the mob doesn't attack through walls */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI")
	bool bRequireLineOfSight = true;
};
