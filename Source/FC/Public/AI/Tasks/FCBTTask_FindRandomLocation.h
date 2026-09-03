#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "FCBTTask_FindRandomLocation.generated.h"

/**
 * Behavior Tree task that finds a random reachable point on the NavMesh around an origin
 */
UCLASS()
class FC_API UFCBTTask_FindRandomLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UFCBTTask_FindRandomLocation();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	/** Radius around origin to find random reachable location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI", meta = (ClampMin = "50.0"))
	float Radius = 1000.0f;

	/** Optional origin key (defaults to HomeLocation or Pawn location if not set) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI")
	FBlackboardKeySelector OriginKey;

	/** Target blackboard vector key to store the resulting location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI")
	FBlackboardKeySelector TargetLocationKey;
};
