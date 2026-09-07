#include "Combat/FCCombatUtils.h"
#include "Combat/Element/FCElementComponent.h"
#include "Character/FCCharacterBase.h"

#include "Character/Player/FCPlayerCharacter.h"
#include "Gameplay/FCChestActor.h"
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

	// 1. Fast check if actor is AFCCharacterBase
	if (const AFCCharacterBase* Character = Cast<AFCCharacterBase>(TargetActor))
	{
		return Character->GetElementComponent();
	}

	// 2. Generic component search fallback (e.g. for destructibles, non-character pawns)
	return TargetActor->FindComponentByClass<UFCElementComponent>();
}

void UFCCombatUtils::ApplyAttackCardHitTraits(
	AActor* SourceActor,
	AActor* TargetActor,
	EFCCharacterClass CharacterClass,
	EFCCardType CardType,
	const TArray<EFCElement>& Elements)
{
	if (!TargetActor || CardType != EFCCardType::Attack)
	{
		return;
	}

	// Server-Authoritative hit trait application
	if (!TargetActor->HasAuthority())
	{
		return;
	}

	// 1. Mage Class Trait: Apply element stacks to target
	if (CharacterClass == EFCCharacterClass::Mage && Elements.Num() > 0)
	{
		if (UFCElementComponent* ElementComp = GetElementComponent(TargetActor))
		{
			ElementComp->AddElementStacks(Elements);
		}
	}

	// 2. Future Class Extension Hooks:
	// - Warrior: Apply Stance interactions / Wound / Bleed stacks
	// - Rogue: Combo point progression / Poison stacks
	// - Priest: Divinity / Holy brand stacks
}

bool UFCCombatUtils::TryConsumeTargetElements(AActor* TargetActor, const TArray<EFCElement>& RequiredElements)
{
	if (!TargetActor || RequiredElements.Num() == 0)
	{
		return false;
	}

	if (!TargetActor->HasAuthority())
	{
		return false;
	}

	if (UFCElementComponent* ElementComp = GetElementComponent(TargetActor))
	{
		return ElementComp->ConsumeElementStacks(RequiredElements);
	}

	return false;
}

TArray<AActor*> UFCCombatUtils::FilterAndConsumeElements(const TArray<AActor*>& TargetActors, const TArray<EFCElement>& RequiredElements)
{
	TArray<AActor*> SatisfiedTargets;

	if (RequiredElements.Num() == 0)
	{
		return SatisfiedTargets;
	}

	for (AActor* Target : TargetActors)
	{
		if (Target && TryConsumeTargetElements(Target, RequiredElements))
		{
			SatisfiedTargets.Add(Target);
		}
	}

	return SatisfiedTargets;
}

int32 UFCCombatUtils::GetTargetElementCount(AActor* TargetActor, EFCElement Element)
{
	if (UFCElementComponent* ElementComp = GetElementComponent(TargetActor))
	{
		return ElementComp->GetElementCount(Element);
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

	// 1. If candidate is a Character
	if (const AFCCharacterBase* Char = Cast<AFCCharacterBase>(TargetCandidate))
	{
		if (Char->IsDead())
		{
			return false;
		}

		// Prevent player friendly fire: if source is player character, other player characters are not attackable
		if (TargetCandidate->IsA<AFCPlayerCharacter>() && SourceActor && SourceActor->IsA<AFCPlayerCharacter>())
		{
			return false;
		}

		return true;
	}

	// 2. If candidate is a Chest/Box
	if (const AFCChestActor* Chest = Cast<AFCChestActor>(TargetCandidate))
	{
		return !Chest->IsOpened();
	}

	// 3. Generic AbilitySystemInterface / AttributeSet check (damageable target)
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
