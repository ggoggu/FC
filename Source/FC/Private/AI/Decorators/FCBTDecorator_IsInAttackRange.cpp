#include "AI/Decorators/FCBTDecorator_IsInAttackRange.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Mob/FCMobCharacter.h"
#include "AI/FCAITypes.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UFCBTDecorator_IsInAttackRange::UFCBTDecorator_IsInAttackRange()
{
	NodeName = TEXT("Is In Attack Range");
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
	bCreateNodeInstance = false;

	// Filter blackboard key to accept Actor objects
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UFCBTDecorator_IsInAttackRange, TargetActorKey), AActor::StaticClass());
}

bool UFCBTDecorator_IsInAttackRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return false;
	}

	APawn* MobPawn = AIController->GetPawn();
	if (!MobPawn)
	{
		return false;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return false;
	}

	const FName KeyName = TargetActorKey.SelectedKeyName != NAME_None 
		? TargetActorKey.SelectedKeyName 
		: FFCMobBlackboardKeys::TargetActor;

	UObject* TargetObject = BlackboardComp->GetValueAsObject(KeyName);
	AActor* TargetActor = Cast<AActor>(TargetObject);
	if (!TargetActor)
	{
		return false;
	}

	// Determine effective attack range
	float EffectiveRange = AttackRangeOverride;
	if (EffectiveRange <= 0.0f)
	{
		if (const AFCMobCharacter* MobChar = Cast<AFCMobCharacter>(MobPawn))
		{
			EffectiveRange = MobChar->GetAttackRange();
		}
		else
		{
			EffectiveRange = 700.0f;
		}
	}

	const float DistSq = FVector::DistSquared(MobPawn->GetActorLocation(), TargetActor->GetActorLocation());
	if (DistSq > FMath::Square(EffectiveRange))
	{
		return false;
	}

	// Optional Line-of-Sight check
	if (bRequireLineOfSight)
	{
		UWorld* World = MobPawn->GetWorld();
		if (World)
		{
			FVector Start = MobPawn->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
			FVector End = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

			FHitResult HitResult;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(MobPawn);
			QueryParams.AddIgnoredActor(TargetActor);

			const bool bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);
			if (bHit && HitResult.bBlockingHit)
			{
				return false;
			}
		}
	}

	return true;
}

FString UFCBTDecorator_IsInAttackRange::GetStaticDescription() const
{
	const FString KeyDesc = TargetActorKey.SelectedKeyName != NAME_None 
		? TargetActorKey.SelectedKeyName.ToString() 
		: TEXT("TargetActor");

	if (AttackRangeOverride > 0.0f)
	{
		return FString::Printf(TEXT("%s within %.1f cm (LoS: %s)"), 
			*KeyDesc, AttackRangeOverride, bRequireLineOfSight ? TEXT("Yes") : TEXT("No"));
	}

	return FString::Printf(TEXT("%s within Mob AttackRange (LoS: %s)"), 
		*KeyDesc, bRequireLineOfSight ? TEXT("Yes") : TEXT("No"));
}
