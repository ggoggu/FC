#pragma once

#include "CoreMinimal.h"
#include "FCProjectileBehaviorBase.generated.h"

class AActor;
class AFCProjectileBase;
struct FHitResult;

/**
 * FFCProjectileBehaviorBase
 *
 * Stateless TInstancedStruct behavior fragment composed on projectile DataAssets
 * to handle radial explosions, piercing, spawning impact actors, element application, etc.
 */
USTRUCT(BlueprintType)
struct FC_API FFCProjectileBehaviorBase
{
	GENERATED_BODY()

	virtual ~FFCProjectileBehaviorBase() = default;

	/** Invoked on the server after the projectile is spawned and initialized */
	virtual void OnProjectileSpawned(AFCProjectileBase* Projectile) const {}

	/**
	 * Invoked on authoritative server impact.
	 * Set bOutContinueFlight = true to keep the projectile alive (e.g. piercing/ricochet).
	 */
	virtual void OnProjectileImpact(
		AFCProjectileBase* Projectile,
		AActor* OtherActor,
		const FHitResult& HitResult,
		bool& bOutContinueFlight) const {}

	/** Invoked on the server for every individual actor damaged by this projectile */
	virtual void OnProjectileHitActor(
		AFCProjectileBase* Projectile,
		AActor* TargetActor,
		const FHitResult& HitResult) const {}

	/** Collects soft asset paths for async deck/projectile preloading */
	virtual void GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const {}
};
