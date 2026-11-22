#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Combat/Projectile/Behaviors/FCProjectileBehaviorBase.h"
#include "FCProjectileBehaviors.generated.h"

class AActor;

/**
 * Deals radial AoE damage to all attackable actors within ExplosionRadius on impact.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Projectile: Radial Explosion"))
struct FC_API FFCProjectileBehavior_RadialExplosion : public FFCProjectileBehaviorBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Projectile|Behavior", meta = (ClampMin = "10.0"))
	float ExplosionRadius = 300.0f;

	virtual void OnProjectileImpact(AFCProjectileBase* Projectile, AActor* OtherActor, const FHitResult& HitResult, bool& bOutContinueFlight) const override;
};

/**
 * Allows the projectile to pierce up to MaxPierceCount impacts before terminating.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Projectile: Pierce"))
struct FC_API FFCProjectileBehavior_Pierce : public FFCProjectileBehaviorBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Projectile|Behavior", meta = (ClampMin = "1"))
	int32 MaxPierceCount = 1;

	virtual void OnProjectileImpact(AFCProjectileBase* Projectile, AActor* OtherActor, const FHitResult& HitResult, bool& bOutContinueFlight) const override;
};

/**
 * Spawns an authoritative actor (hazard, trap, summon) at the impact point.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Projectile: Spawn Actor On Impact"))
struct FC_API FFCProjectileBehavior_SpawnActorOnImpact : public FFCProjectileBehaviorBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Projectile|Behavior")
	TSoftClassPtr<AActor> ActorClass;

	virtual void OnProjectileImpact(AFCProjectileBase* Projectile, AActor* OtherActor, const FHitResult& HitResult, bool& bOutContinueFlight) const override;
	virtual void GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const override;
};

/**
 * Applies elemental GameplayTag stacks to each actor damaged by this projectile.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Projectile: Apply Element Tags"))
struct FC_API FFCProjectileBehavior_ApplyElementTags : public FFCProjectileBehaviorBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Projectile|Behavior")
	TArray<FGameplayTag> ElementTags;

	virtual void OnProjectileHitActor(AFCProjectileBase* Projectile, AActor* TargetActor, const FHitResult& HitResult) const override;
};
