#include "AI/Tasks/FCBTTask_Attack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Controller/AI/FCMobAIController.h"
#include "AI/FCAITypes.h"
#include "Animation/AnimMontage.h"

UFCBTTask_Attack::UFCBTTask_Attack()
{
	NodeName = TEXT("Mob Attack");
	bNotifyTick = true;

	// Filter blackboard key to accept Actor objects
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UFCBTTask_Attack, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UFCBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	AFCMobCharacter* MobChar = Cast<AFCMobCharacter>(AIController->GetPawn());
	if (!MobChar || MobChar->IsDead())
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	const FName KeyName = TargetActorKey.SelectedKeyName != NAME_None
		? TargetActorKey.SelectedKeyName
		: FFCMobBlackboardKeys::TargetActor;

	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(KeyName));
	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	FFCAttackTaskMemory* MyMemory = reinterpret_cast<FFCAttackTaskMemory*>(NodeMemory);
	MyMemory->TargetActor = TargetActor;

	// Orient Mob towards the target
	if (bFaceTargetOnExecute)
	{
		const FVector Dir = (TargetActor->GetActorLocation() - MobChar->GetActorLocation()).GetSafeNormal2D();
		if (!Dir.IsNearlyZero())
		{
			MobChar->SetActorRotation(FRotator(0.0f, Dir.Rotation().Yaw, 0.0f));
		}
		AIController->SetFocus(TargetActor);
	}

	// Set active combat target on Mob for 3D pitch/elevation aim correction
	MobChar->SetCombatTarget(TargetActor);

	// Set AI state to Attacking
	if (AFCMobAIController* MobAICont = Cast<AFCMobAIController>(AIController))
	{
		MobAICont->SetAIState(EFCMobAIState::Attacking);
	}

	// Play montage if available
	UAnimMontage* MontageToPlay = MontageOverride ? MontageOverride.Get() : MobChar->GetAttackMontage();
	float Duration = AttackDuration;
	if (MontageToPlay)
	{
		const float MontageLen = MobChar->PlayAnimMontage(MontageToPlay, PlayRate);
		if (MontageLen > 0.0f)
		{
			Duration = MontageLen / PlayRate;
		}
	}

	MyMemory->RemainingDuration = FMath::Max(0.1f, Duration);

	// Handle cast delay / animation windup
	if (CastDelay <= 0.0f || !MontageToPlay)
	{
		const bool bSuccess = MobChar->TryActivateAttackAbility();
		if (!bSuccess)
		{
			MobChar->SetCombatTarget(nullptr);
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
			if (AFCMobAIController* MobAICont = Cast<AFCMobAIController>(AIController))
			{
				MobAICont->SetAIState(EFCMobAIState::Chasing);
			}
			return EBTNodeResult::Failed;
		}
		MyMemory->bAbilityTriggered = true;
		MyMemory->RemainingCastDelay = 0.0f;
	}
	else
	{
		MyMemory->bAbilityTriggered = false;
		MyMemory->RemainingCastDelay = FMath::Min(CastDelay, Duration * 0.8f);
	}

	return EBTNodeResult::InProgress;
}

void UFCBTTask_Attack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FFCAttackTaskMemory* MyMemory = reinterpret_cast<FFCAttackTaskMemory*>(NodeMemory);
	MyMemory->RemainingDuration -= DeltaSeconds;

	AAIController* AIController = OwnerComp.GetAIOwner();
	AFCMobCharacter* MobChar = AIController ? Cast<AFCMobCharacter>(AIController->GetPawn()) : nullptr;

	if (!MobChar || MobChar->IsDead())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// Fire the ability once windup / cast delay completes
	if (!MyMemory->bAbilityTriggered)
	{
		MyMemory->RemainingCastDelay -= DeltaSeconds;
		if (MyMemory->RemainingCastDelay <= 0.0f)
		{
			MobChar->TryActivateAttackAbility();
			MyMemory->bAbilityTriggered = true;
		}
	}

	// Continuously track target during attack windup
	if (bTrackTargetDuringAttack && MyMemory->TargetActor.IsValid())
	{
		const FVector Dir = (MyMemory->TargetActor->GetActorLocation() - MobChar->GetActorLocation()).GetSafeNormal2D();
		if (!Dir.IsNearlyZero())
		{
			const FRotator TargetRot(0.0f, Dir.Rotation().Yaw, 0.0f);
			MobChar->SetActorRotation(FMath::RInterpTo(MobChar->GetActorRotation(), TargetRot, DeltaSeconds, 10.0f));
		}
	}

	if (MyMemory->RemainingDuration <= 0.0f)
	{
		MobChar->SetCombatTarget(nullptr);

		if (AIController)
		{
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
			if (AFCMobAIController* MobAICont = Cast<AFCMobAIController>(AIController))
			{
				MobAICont->SetAIState(EFCMobAIState::Chasing);
			}
		}

		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UFCBTTask_Attack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		if (AFCMobCharacter* MobChar = Cast<AFCMobCharacter>(AIController->GetPawn()))
		{
			MobChar->SetCombatTarget(nullptr);
			MobChar->StopAnimMontage();
		}
		if (AFCMobAIController* MobAICont = Cast<AFCMobAIController>(AIController))
		{
			MobAICont->SetAIState(EFCMobAIState::Chasing);
		}
	}

	return EBTNodeResult::Aborted;
}

uint16 UFCBTTask_Attack::GetInstanceMemorySize() const
{
	return sizeof(FFCAttackTaskMemory);
}

FString UFCBTTask_Attack::GetStaticDescription() const
{
	const FString TargetDesc = TargetActorKey.SelectedKeyName != NAME_None
		? TargetActorKey.SelectedKeyName.ToString()
		: TEXT("TargetActor");

	return FString::Printf(TEXT("Cast Attack Ability on %s (Duration: %.2fs)"), *TargetDesc, AttackDuration);
}
