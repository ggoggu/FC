#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "FCBTTask_LookAround.generated.h"

struct FFCLookAroundTaskMemory
{
	float ElapsedTime = 0.0f;
	FRotator InitialRotation = FRotator::ZeroRotator;
	FRotator DesiredRotation = FRotator::ZeroRotator;
	int32 StepIndex = 0;
	float StepTimer = 0.0f;
};

/**
 * Behavior Tree task that pauses and naturally turns the character to look around
 */
UCLASS()
class FC_API UFCBTTask_LookAround : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UFCBTTask_LookAround();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual FString GetStaticDescription() const override;

protected:
	/** Total time spent looking around */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI", meta = (ClampMin = "1.0"))
	float TotalDuration = 3.0f;

	/** Maximum angle in degrees to turn left and right when looking around */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI", meta = (ClampMin = "10.0", ClampMax = "120.0"))
	float LookAngle = 60.0f;

	/** Angular rotation speed in degrees per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI", meta = (ClampMin = "30.0", ClampMax = "360.0"))
	float RotationSpeed = 120.0f;
};
