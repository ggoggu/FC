#include "Combat/Projectile/Behaviors/FCProjectileBehaviors.h"
#include "Combat/Projectile/FCProjectileBase.h"
#include "Combat/FCCombatUtils.h"
#include "Combat/Element/FCElementComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Pawn.h"

void FFCProjectileBehavior_RadialExplosion::OnProjectileImpact(
	AFCProjectileBase* Projectile,
	AActor* OtherActor,
	const FHitResult& HitResult,
	bool& bOutContinueFlight) const
{
	if (!Projectile || !Projectile->HasAuthority() || ExplosionRadius <= 0.0f)
	{
		return;
	}

	UWorld* World = Projectile->GetWorld();
	if (!World)
	{
		return;
	}

	const FVector ImpactLocation = HitResult.ImpactPoint.IsZero() ? Projectile->GetActorLocation() : FVector(HitResult.ImpactPoint);
	TArray<FOverlapResult> OverlapResults;
	const FCollisionShape SphereShape = FCollisionShape::MakeSphere(ExplosionRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Projectile);
	QueryParams.AddIgnoredActor(Projectile->GetInstigator());

	TSet<AActor*> DamagedActors;
	if (World->OverlapMultiByChannel(OverlapResults, ImpactLocation, FQuat::Identity, ECC_Pawn, SphereShape, QueryParams))
	{
		for (const FOverlapResult& Overlap : OverlapResults)
		{
			if (AActor* Target = Overlap.GetActor())
			{
				if (!DamagedActors.Contains(Target))
				{
					DamagedActors.Add(Target);
					Projectile->ApplyDamageToActor(Target, HitResult);
				}
			}
		}
	}

	if (OtherActor && !DamagedActors.Contains(OtherActor))
	{
		Projectile->ApplyDamageToActor(OtherActor, HitResult);
	}
}

void FFCProjectileBehavior_Pierce::OnProjectileImpact(
	AFCProjectileBase* Projectile,
	AActor* OtherActor,
	const FHitResult& HitResult,
	bool& bOutContinueFlight) const
{
	if (!Projectile || !Projectile->HasAuthority())
	{
		return;
	}

	if (Projectile->GetCurrentPierceCount() < MaxPierceCount)
	{
		Projectile->IncrementPierceCount();
		bOutContinueFlight = true;
	}
}

void FFCProjectileBehavior_SpawnActorOnImpact::OnProjectileImpact(
	AFCProjectileBase* Projectile,
	AActor* OtherActor,
	const FHitResult& HitResult,
	bool& bOutContinueFlight) const
{
	if (!Projectile || !Projectile->HasAuthority() || ActorClass.IsNull())
	{
		return;
	}

	UWorld* World = Projectile->GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<AActor> LoadedClass = ActorClass.IsValid() ? ActorClass.Get() : ActorClass.LoadSynchronous();
	if (!LoadedClass)
	{
		return;
	}

	const FVector ImpactLocation = HitResult.ImpactPoint.IsZero() ? Projectile->GetActorLocation() : FVector(HitResult.ImpactPoint);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Projectile->GetOwner();
	SpawnParams.Instigator = Projectile->GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<AActor>(LoadedClass, ImpactLocation, Projectile->GetActorRotation(), SpawnParams);
}

void FFCProjectileBehavior_SpawnActorOnImpact::GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	if (!ActorClass.IsNull())
	{
		OutPaths.AddUnique(ActorClass.ToSoftObjectPath());
	}
}

void FFCProjectileBehavior_ApplyElementTags::OnProjectileHitActor(
	AFCProjectileBase* Projectile,
	AActor* TargetActor,
	const FHitResult& HitResult) const
{
	if (!Projectile || !Projectile->HasAuthority() || !TargetActor || ElementTags.Num() == 0)
	{
		return;
	}

	if (UFCElementComponent* ElemComp = UFCCombatUtils::GetElementComponent(TargetActor))
	{
		AActor* InstigatorActor = Projectile->GetInstigator() ? Cast<AActor>(Projectile->GetInstigator()) : Cast<AActor>(Projectile);
		ElemComp->AddElementStacks(ElementTags, InstigatorActor);
	}
}
