#include "AI/Tasks/FCBTTask_LookAround.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"

UFCBTTask_LookAround::UFCBTTask_LookAround()
{
	NodeName = TEXT("Look Around");
	bNotifyTick = true;
}

uint16 UFCBTTask_LookAround::GetInstanceMemorySize() const
{
	return sizeof(FFCLookAroundTaskMemory);
}

EBTNodeResult::Type UFCBTTask_LookAround::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	FFCLookAroundTaskMemory* MyMemory = reinterpret_cast<FFCLookAroundTaskMemory*>(NodeMemory);
	MyMemory->ElapsedTime = 0.0f;
	MyMemory->InitialRotation = Pawn->GetActorRotation();
	MyMemory->DesiredRotation = FRotator(0.0f, FRotator::NormalizeAxis(MyMemory->InitialRotation.Yaw + LookAngle), 0.0f);
	MyMemory->StepIndex = 0;
	MyMemory->StepTimer = 0.0f;

	return EBTNodeResult::InProgress;
}

void UFCBTTask_LookAround::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FFCLookAroundTaskMemory* MyMemory = reinterpret_cast<FFCLookAroundTaskMemory*>(NodeMemory);
	MyMemory->ElapsedTime += DeltaSeconds;
	MyMemory->StepTimer += DeltaSeconds;

	if (MyMemory->ElapsedTime >= TotalDuration)
	{
		// Restore initial rotation and finish successfully
		Pawn->SetActorRotation(FRotator(0.0f, MyMemory->InitialRotation.Yaw, 0.0f));
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	const float StepDuration = FMath::Max(0.1f, TotalDuration / 3.0f);
	if (MyMemory->StepTimer >= StepDuration)
	{
		MyMemory->StepTimer = 0.0f;
		MyMemory->StepIndex = (MyMemory->StepIndex + 1);

		if (MyMemory->StepIndex == 1)
		{
			// Step 1: Turn opposite direction
			MyMemory->DesiredRotation = FRotator(0.0f, FRotator::NormalizeAxis(MyMemory->InitialRotation.Yaw - LookAngle), 0.0f);
		}
		else if (MyMemory->StepIndex >= 2)
		{
			// Step 2+: Return to initial center orientation
			MyMemory->DesiredRotation = FRotator(0.0f, MyMemory->InitialRotation.Yaw, 0.0f);
		}
	}

	const FRotator CurrentRot = Pawn->GetActorRotation();
	const FRotator NewRot = FMath::RInterpConstantTo(CurrentRot, MyMemory->DesiredRotation, DeltaSeconds, RotationSpeed);
	Pawn->SetActorRotation(FRotator(0.0f, NewRot.Yaw, 0.0f));
}

FString UFCBTTask_LookAround::GetStaticDescription() const
{
	return FString::Printf(TEXT("Look around +/-%.0f deg for %.1fs (Speed: %.0f deg/s)"), LookAngle, TotalDuration, RotationSpeed);
}
