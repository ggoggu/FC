#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "FCBTTask_ClearBlackboardKey.generated.h"

/**
 * Behavior Tree task that clears a specified Blackboard key value
 */
UCLASS()
class FC_API UFCBTTask_ClearBlackboardKey : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UFCBTTask_ClearBlackboardKey();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	/** The blackboard key to clear */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI")
	FBlackboardKeySelector KeyToClear;
};
