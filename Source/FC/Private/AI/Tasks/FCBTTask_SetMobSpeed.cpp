#include "AI/Tasks/FCBTTask_SetMobSpeed.h"
#include "AIController.h"
#include "Character/Mob/FCMobCharacter.h"

UFCBTTask_SetMobSpeed::UFCBTTask_SetMobSpeed()
{
	NodeName = TEXT("Set Mob Speed");
}

EBTNodeResult::Type UFCBTTask_SetMobSpeed::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	AFCMobCharacter* MobChar = Cast<AFCMobCharacter>(AIController->GetPawn());
	if (!MobChar)
	{
		return EBTNodeResult::Failed;
	}

	if (CustomSpeed > 0.0f)
	{
		MobChar->SetMovementSpeed(CustomSpeed);
	}
	else if (bChaseSpeed)
	{
		MobChar->SetChaseSpeed();
	}
	else
	{
		MobChar->SetPatrolSpeed();
	}

	return EBTNodeResult::Succeeded;
}

FString UFCBTTask_SetMobSpeed::GetStaticDescription() const
{
	if (CustomSpeed > 0.0f)
	{
		return FString::Printf(TEXT("Set Speed to Custom: %.0f"), CustomSpeed);
	}
	return FString::Printf(TEXT("Set Speed to %s"), bChaseSpeed ? TEXT("ChaseSpeed") : TEXT("PatrolSpeed"));
}
