#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "FCBTTask_SetMobSpeed.generated.h"

/**
 * Behavior Tree task that configures AFCMobCharacter movement speed (Patrol vs Chase)
 */
UCLASS()
class FC_API UFCBTTask_SetMobSpeed : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UFCBTTask_SetMobSpeed();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	/** If true, sets speed to Mob's ChaseSpeed. If false, sets to PatrolSpeed (unless CustomSpeed > 0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI")
	bool bChaseSpeed = false;

	/** Optional custom speed override. If <= 0, uses Mob's configured Patrol/Chase speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI", meta = (ClampMin = "0.0"))
	float CustomSpeed = 0.0f;
};
