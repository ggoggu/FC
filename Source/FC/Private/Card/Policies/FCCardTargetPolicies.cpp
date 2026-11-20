#include "Card/Policies/FCCardTargetPolicies.h"
#include "Card/FCCardExecutionContext.h"
#include "Combat/FCCombatUtils.h"
#include "Character/Player/FCPlayerCharacter.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"

namespace
{
	FVector ClampAimLocation(const FVector& CasterLocation, const FVector& RawLocation, float MaxRange)
	{
		if (RawLocation.IsZero())
		{
			return FVector::ZeroVector;
		}
		const FVector Delta = RawLocation - CasterLocation;
		if (Delta.SizeSquared() > FMath::Square(MaxRange))
		{
			return CasterLocation + Delta.GetSafeNormal() * MaxRange;
		}
		return RawLocation;
	}

	bool IsCandidateAllowed(const AActor* Caster, const AActor* Candidate, bool bAllowFriendlyTargets)
	{
		if (!Caster || !Candidate)
		{
			return false;
		}
		if (UFCCombatUtils::IsAttackableTarget(Caster, Candidate))
		{
			return true;
		}
		if (bAllowFriendlyTargets)
		{
			return Candidate == Caster || Candidate->IsA<AFCPlayerCharacter>();
		}
		return false;
	}

	bool ResolveDirectionalClientHelper(const FFCCardClientTargetQuery& Query, bool bEnablePolicyAutoTarget, FFCCardTargetInfo& OutTargetInfo)
	{
		APawn* CasterPawn = Query.CasterPawn.Get();
		if (!CasterPawn)
		{
			return false;
		}

		OutTargetInfo.TargetLocation = Query.DropWorldLocation;
		OutTargetInfo.TargetHitComponent = Query.DirectHitComponent;

		if (AActor* DirectHit = Query.DirectHitActor.Get())
		{
			if (UFCCombatUtils::IsAttackableTarget(CasterPawn, DirectHit))
			{
				OutTargetInfo.TargetActor = DirectHit;
				OutTargetInfo.TargetLocation = DirectHit->GetActorLocation();
				return true;
			}
		}

		if (Query.bEnableAutoTargeting && bEnablePolicyAutoTarget)
		{
			const FVector AimDir2D = (Query.DropWorldLocation - CasterPawn->GetActorLocation()).GetSafeNormal2D();
			if (AActor* BestTarget = UFCCombatUtils::FindBestAttackableTargetInDirection(
				CasterPawn,
				AimDir2D,
				Query.DropWorldLocation,
				Query.MaxRange,
				Query.HalfAngleDegrees,
				Query.ProximityRadius,
				true))
			{
				OutTargetInfo.TargetActor = BestTarget;
				OutTargetInfo.TargetLocation = BestTarget->GetActorLocation();
			}
		}

		return true;
	}
}

bool FFCCardTargetPolicy_Self::ResolveClientTarget(const FFCCardClientTargetQuery& Query, const FFCCardGameplayData& CardData, FFCCardTargetInfo& OutTargetInfo) const
{
	if (APawn* CasterPawn = Query.CasterPawn.Get())
	{
		OutTargetInfo.TargetActor = CasterPawn;
		OutTargetInfo.TargetLocation = CasterPawn->GetActorLocation();
		return true;
	}
	return false;
}

bool FFCCardTargetPolicy_Self::VerifyServerTargets(const FFCCardExecutionContext& Context, TArray<TWeakObjectPtr<AActor>>& OutVerifiedTargets, FVector& OutVerifiedAimLocation) const
{
	if (AActor* Caster = Context.CasterActor.Get())
	{
		OutVerifiedTargets.Reset();
		OutVerifiedTargets.Add(Caster);
		OutVerifiedAimLocation = Caster->GetActorLocation();
		return true;
	}
	return false;
}

bool FFCCardTargetPolicy_SingleTarget::ResolveClientTarget(const FFCCardClientTargetQuery& Query, const FFCCardGameplayData& CardData, FFCCardTargetInfo& OutTargetInfo) const
{
	return ResolveDirectionalClientHelper(Query, bEnableClientAutoTarget, OutTargetInfo);
}

bool FFCCardTargetPolicy_SingleTarget::VerifyServerTargets(const FFCCardExecutionContext& Context, TArray<TWeakObjectPtr<AActor>>& OutVerifiedTargets, FVector& OutVerifiedAimLocation) const
{
	OutVerifiedTargets.Reset();
	AActor* Caster = Context.CasterActor.Get();
	if (!Caster)
	{
		return false;
	}

	const FVector CasterLocation = Caster->GetActorLocation();
	OutVerifiedAimLocation = ClampAimLocation(CasterLocation, Context.RawTargetInfo.TargetLocation, MaxRange);

	AActor* Candidate = Context.RawTargetInfo.TargetActor.Get();
	if (!Candidate)
	{
		return true;
	}

	const bool bWithinRange = FVector::DistSquared(CasterLocation, Candidate->GetActorLocation()) <= FMath::Square(MaxRange);
	if (!bWithinRange)
	{
		return true;
	}

	bool bHasLoS = true;
	if (bRequireLineOfSight && Candidate != Caster)
	{
		if (const UWorld* World = Caster->GetWorld())
		{
			FHitResult OcclusionHit;
			FCollisionQueryParams LoSParams(SCENE_QUERY_STAT(FCCardPolicySingleLoS), false, Caster);
			LoSParams.AddIgnoredActor(Candidate);
			bHasLoS = !World->LineTraceSingleByChannel(
				OcclusionHit,
				CasterLocation + FVector(0.0f, 0.0f, 50.0f),
				Candidate->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f),
				ECC_WorldStatic,
				LoSParams);
		}
	}

	if (bHasLoS && IsCandidateAllowed(Caster, Candidate, bAllowFriendlyTargets))
	{
		OutVerifiedTargets.Add(Candidate);
	}

	return true;
}

bool FFCCardTargetPolicy_Directional::ResolveClientTarget(const FFCCardClientTargetQuery& Query, const FFCCardGameplayData& CardData, FFCCardTargetInfo& OutTargetInfo) const
{
	return ResolveDirectionalClientHelper(Query, bEnableClientAutoTarget, OutTargetInfo);
}

bool FFCCardTargetPolicy_Directional::VerifyServerTargets(const FFCCardExecutionContext& Context, TArray<TWeakObjectPtr<AActor>>& OutVerifiedTargets, FVector& OutVerifiedAimLocation) const
{
	OutVerifiedTargets.Reset();
	AActor* Caster = Context.CasterActor.Get();
	if (!Caster)
	{
		return false;
	}

	const FVector CasterLocation = Caster->GetActorLocation();
	OutVerifiedAimLocation = ClampAimLocation(CasterLocation, Context.RawTargetInfo.TargetLocation, MaxRange);

	if (AActor* Candidate = Context.RawTargetInfo.TargetActor.Get())
	{
		const bool bWithinRange = FVector::DistSquared(CasterLocation, Candidate->GetActorLocation()) <= FMath::Square(MaxRange);
		if (bWithinRange && IsCandidateAllowed(Caster, Candidate, bAllowFriendlyTargets))
		{
			OutVerifiedTargets.Add(Candidate);
		}
	}

	return true;
}

bool FFCCardTargetPolicy_AllEnemiesInRadius::ResolveClientTarget(const FFCCardClientTargetQuery& Query, const FFCCardGameplayData& CardData, FFCCardTargetInfo& OutTargetInfo) const
{
	if (APawn* CasterPawn = Query.CasterPawn.Get())
	{
		OutTargetInfo.TargetLocation = CasterPawn->GetActorLocation();
		return true;
	}
	return false;
}

bool FFCCardTargetPolicy_AllEnemiesInRadius::VerifyServerTargets(const FFCCardExecutionContext& Context, TArray<TWeakObjectPtr<AActor>>& OutVerifiedTargets, FVector& OutVerifiedAimLocation) const
{
	OutVerifiedTargets.Reset();
	AActor* Caster = Context.CasterActor.Get();
	if (!Caster)
	{
		return false;
	}

	const FVector Origin = Caster->GetActorLocation();
	OutVerifiedAimLocation = Origin;

	const UWorld* World = Caster->GetWorld();
	if (!World || Radius <= 0.0f)
	{
		return true;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FCCardPolicyAllEnemies), false, Caster);
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

	TSet<AActor*> UniqueTargets;
	auto CollectChannel = [&](ECollisionChannel Channel)
	{
		TArray<FOverlapResult> Overlaps;
		if (World->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity, Channel, Sphere, QueryParams))
		{
			for (const FOverlapResult& Overlap : Overlaps)
			{
				if (AActor* Candidate = Overlap.GetActor())
				{
					if (UFCCombatUtils::IsAttackableTarget(Caster, Candidate))
					{
						UniqueTargets.Add(Candidate);
					}
				}
			}
		}
	};

	CollectChannel(ECC_Pawn);
	CollectChannel(ECC_WorldDynamic);

	for (AActor* Target : UniqueTargets)
	{
		OutVerifiedTargets.Add(Target);
	}

	return true;
}

bool FFCCardTargetPolicy_None::ResolveClientTarget(const FFCCardClientTargetQuery& Query, const FFCCardGameplayData& CardData, FFCCardTargetInfo& OutTargetInfo) const
{
	OutTargetInfo.TargetLocation = Query.DropWorldLocation;
	return true;
}

bool FFCCardTargetPolicy_None::VerifyServerTargets(const FFCCardExecutionContext& Context, TArray<TWeakObjectPtr<AActor>>& OutVerifiedTargets, FVector& OutVerifiedAimLocation) const
{
	OutVerifiedTargets.Reset();
	OutVerifiedAimLocation = Context.RawTargetInfo.TargetLocation;
	return true;
}
