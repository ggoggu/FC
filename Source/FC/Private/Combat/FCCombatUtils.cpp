#include "Combat/FCCombatUtils.h"
#include "Combat/Element/FCElementComponent.h"
#include "Combat/Interfaces/FCCombatTargetInterface.h"
#include "Data/Class/FCClassSubsystem.h"
#include "Data/FCGameplayTags.h"
#include "Character/FCCharacterBase.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"

UFCElementComponent* UFCCombatUtils::GetElementComponent(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return nullptr;
	}

	if (const AFCCharacterBase* Character = Cast<AFCCharacterBase>(TargetActor))
	{
		return Character->GetElementComponent();
	}

	return TargetActor->FindComponentByClass<UFCElementComponent>();
}

void UFCCombatUtils::ApplyAttackCardHitTraits(
	AActor* SourceActor,
	AActor* TargetActor,
	FGameplayTag ClassTag,
	FGameplayTag CardTypeTag,
	const TArray<FGameplayTag>& ElementTags)
{
	if (!TargetActor || !TargetActor->HasAuthority() || !ClassTag.IsValid())
	{
		return;
	}

	const UFCClassSubsystem* ClassSubsystem = UFCClassSubsystem::GetClassSubsystem(TargetActor);
	const FFCCharacterClassData* ClassRow = ClassSubsystem
		? ClassSubsystem->FindClassRowPtr(ClassTag)
		: UFCClassSubsystem::FindBuiltinClassDefault(ClassTag);

	if (!ClassRow)
	{
		return;
	}

	if (const FFCClassTraitBehaviorBase* TraitBehavior = ClassRow->TraitBehavior.GetPtr())
	{
		FGameplayTagContainer AttackTags;
		if (CardTypeTag.IsValid())
		{
			AttackTags.AddTag(CardTypeTag);
		}
		TraitBehavior->OnAttackHit(SourceActor, TargetActor, AttackTags, ElementTags);
	}
}

bool UFCCombatUtils::TryConsumeTargetElements(AActor* TargetActor, const TArray<FGameplayTag>& RequiredElementTags)
{
	if (!TargetActor || RequiredElementTags.Num() == 0)
	{
		return false;
	}

	if (!TargetActor->HasAuthority())
	{
		return false;
	}

	if (UFCElementComponent* ElementComp = GetElementComponent(TargetActor))
	{
		return ElementComp->ConsumeElementStacks(RequiredElementTags);
	}

	return false;
}

TArray<AActor*> UFCCombatUtils::FilterAndConsumeElements(const TArray<AActor*>& TargetActors, const TArray<FGameplayTag>& RequiredElementTags)
{
	TArray<AActor*> SatisfiedTargets;

	if (RequiredElementTags.Num() == 0)
	{
		return SatisfiedTargets;
	}

	for (AActor* Target : TargetActors)
	{
		if (Target && TryConsumeTargetElements(Target, RequiredElementTags))
		{
			SatisfiedTargets.Add(Target);
		}
	}

	return SatisfiedTargets;
}

int32 UFCCombatUtils::GetTargetElementCount(AActor* TargetActor, FGameplayTag ElementTag)
{
	if (UFCElementComponent* ElementComp = GetElementComponent(TargetActor))
	{
		return ElementComp->GetElementCount(ElementTag);
	}
	return 0;
}

int32 UFCCombatUtils::GetTargetTotalElementStacks(AActor* TargetActor)
{
	if (UFCElementComponent* ElementComp = GetElementComponent(TargetActor))
	{
		return ElementComp->GetTotalElementStacks();
	}
	return 0;
}

bool UFCCombatUtils::IsAttackableTarget(const AActor* SourceActor, const AActor* TargetCandidate)
{
	if (!TargetCandidate || TargetCandidate == SourceActor)
	{
		return false;
	}

	// 1. Polymorphic OCP contract (characters, chests, destructible walls, custom combat targets)
	if (const IFCCombatTargetInterface* CombatTarget = Cast<IFCCombatTargetInterface>(TargetCandidate))
	{
		return CombatTarget->IsCombatTargetAlive() && CombatTarget->CanBeTargetedBy(SourceActor);
	}

	// 2. Generic AbilitySystemInterface / AttributeSet fallback (non-interface damageable target)
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetCandidate))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			if (const UFCAttributeSet* AttributeSet = ASC->GetSet<UFCAttributeSet>())
			{
				if (AttributeSet->GetHealth() <= 0.0f)
				{
					return false;
				}
			}
			return true;
		}
	}

	return false;
}

AActor* UFCCombatUtils::FindBestAttackableTargetInDirection(
	const AActor* SourceActor,
	const FVector& AimDirection,
	const FVector& AimLocation,
	float MaxRange,
	float HalfAngleDegrees,
	float ProximityRadius,
	bool bCheckLineOfSight)
{
	if (!SourceActor)
	{
		return nullptr;
	}

	UWorld* World = SourceActor->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector SourceLocation = SourceActor->GetActorLocation();
	const FVector NormalizedAimDir2D = AimDirection.GetSafeNormal2D();
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(HalfAngleDegrees, 1.0f, 89.0f)));

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FindBestAttackableTarget), false);
	QueryParams.AddIgnoredActor(SourceActor);

	// Collect candidate actors
	TSet<AActor*> Candidates;

	// 1. Proximity Check around AimLocation (e.g. mouse cursor dropped near an enemy or chest)
	if (!AimLocation.IsZero() && ProximityRadius > 0.0f)
	{
		TArray<FOverlapResult> ProximityOverlaps;
		FCollisionShape ProximitySphere = FCollisionShape::MakeSphere(ProximityRadius);
		World->OverlapMultiByChannel(
			ProximityOverlaps,
			AimLocation,
			FQuat::Identity,
			ECC_WorldDynamic,
			ProximitySphere,
			QueryParams
		);

		TArray<FOverlapResult> PawnOverlaps;
		World->OverlapMultiByChannel(
			PawnOverlaps,
			AimLocation,
			FQuat::Identity,
			ECC_Pawn,
			ProximitySphere,
			QueryParams
		);

		for (const FOverlapResult& Overlap : ProximityOverlaps)
		{
			if (AActor* Act = Overlap.GetActor())
			{
				if (IsAttackableTarget(SourceActor, Act))
				{
					Candidates.Add(Act);
				}
			}
		}
		for (const FOverlapResult& Overlap : PawnOverlaps)
		{
			if (AActor* Act = Overlap.GetActor())
			{
				if (IsAttackableTarget(SourceActor, Act))
				{
					Candidates.Add(Act);
				}
			}
		}
	}

	// 2. Directional Sweep / Overlap around SourceActor within MaxRange
	if (!NormalizedAimDir2D.IsNearlyZero() && MaxRange > 0.0f)
	{
		TArray<FOverlapResult> RangeOverlaps;
		FCollisionShape RangeSphere = FCollisionShape::MakeSphere(MaxRange);
		World->OverlapMultiByChannel(
			RangeOverlaps,
			SourceLocation,
			FQuat::Identity,
			ECC_Pawn,
			RangeSphere,
			QueryParams
		);

		TArray<FOverlapResult> DynamicOverlaps;
		World->OverlapMultiByChannel(
			DynamicOverlaps,
			SourceLocation,
			FQuat::Identity,
			ECC_WorldDynamic,
			RangeSphere,
			QueryParams
		);

		auto ProcessOverlapList = [&](const TArray<FOverlapResult>& Overlaps)
		{
			for (const FOverlapResult& Overlap : Overlaps)
			{
				AActor* Act = Overlap.GetActor();
				if (!Act || !IsAttackableTarget(SourceActor, Act))
				{
					continue;
				}

				FVector ToTarget2D = (Act->GetActorLocation() - SourceLocation).GetSafeNormal2D();
				if (ToTarget2D.IsNearlyZero())
				{
					Candidates.Add(Act);
					continue;
				}

				const float Dot = FVector::DotProduct(NormalizedAimDir2D, ToTarget2D);
				if (Dot >= CosHalfAngle)
				{
					Candidates.Add(Act);
				}
			}
		};

		ProcessOverlapList(RangeOverlaps);
		ProcessOverlapList(DynamicOverlaps);
	}

	if (Candidates.Num() == 0)
	{
		return nullptr;
	}

	// 3. Score candidates and check Line of Sight
	AActor* BestTarget = nullptr;
	float BestScore = MAX_FLT;

	const FVector EyeLocation = SourceLocation + FVector(0.0f, 0.0f, 50.0f);

	for (AActor* Candidate : Candidates)
	{
		if (!Candidate)
		{
			continue;
		}

		const FVector TargetPos = Candidate->GetActorLocation();

		// Line of Sight Check against WorldStatic blocking obstacles
		if (bCheckLineOfSight)
		{
			FHitResult LoSHit;
			FCollisionQueryParams LoSParams(SCENE_QUERY_STAT(TargetLoSCheck), true);
			LoSParams.AddIgnoredActor(SourceActor);
			LoSParams.AddIgnoredActor(Candidate);

			const bool bBlocked = World->LineTraceSingleByChannel(
				LoSHit,
				EyeLocation,
				TargetPos,
				ECC_WorldStatic,
				LoSParams
			);

			if (bBlocked)
			{
				continue; // Vision blocked by wall/obstacle
			}
		}

		// Scoring:
		// Factor A: Distance to AimLocation (if provided)
		float ProximityDist = !AimLocation.IsZero() ? FVector::Distance(AimLocation, TargetPos) : 0.0f;

		// Factor B: Lateral perpendicular distance from SourceLocation along NormalizedAimDir2D
		FVector ToTarget = TargetPos - SourceLocation;
		FVector ProjectedPoint = SourceLocation + NormalizedAimDir2D * FMath::Max(0.0f, FVector::DotProduct(ToTarget, NormalizedAimDir2D));
		float PerpendicularDist = FVector::Distance(TargetPos, ProjectedPoint);

		// Factor C: Forward distance from player
		float ForwardDist = ToTarget.Size();

		// Combined Score: Prioritize alignment to aim line, proximity to cursor, and close distance
		float Score = PerpendicularDist * 2.0f + ProximityDist * 0.5f + ForwardDist * 0.2f;

		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}
