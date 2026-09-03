#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/Card/FCCardTypes.h"
#include "FCProjectileDataAsset.generated.h"

class AFCProjectileBase;
class UGameplayEffect;
class UNiagaraSystem;
class USoundBase;

/**
 * UFCProjectileDataAsset
 * 
 * Data Asset encapsulating gameplay rules and numerical parameters for projectiles,
 * referencing a visual/collider Blueprint template actor (e.g. BP_FCProjectile_Arrow).
 * Visual alignments (mesh rotation, scale, collision radius) are cleanly managed
 * in the Blueprint viewport, while this asset controls combat, speed, and element rules.
 */
UCLASS(BlueprintType)
class FC_API UFCProjectileDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFCProjectileDataAsset();

	// --- Visual / Collider Blueprint Template ---
	/** The Projectile Blueprint/Actor class to spawn (e.g. BP_FCProjectile_Arrow, BP_FCProjectile_Orb) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Template")
	TSubclassOf<AFCProjectileBase> ProjectileClass;

	// --- Combat & Impact Configuration ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float Damage = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float ExplosionRadius = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	bool bPiercing = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0"))
	int32 MaxPierceCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSubclassOf<AActor> SpawnActorOnImpact;

	/** Elemental affinities carried by this projectile to apply on hit */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TArray<EFCElement> ProjectileElements;

	/** Character class associated with this projectile */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	EFCCharacterClass SourceClass = EFCCharacterClass::Mage;

	/** Source card functional type */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	EFCCardType SourceCardType = EFCCardType::Attack;

	// --- Flight & Movement Parameters ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float LaunchSpeed = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float MaxSpeed = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float GravityScale = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	bool bRotationFollowsVelocity = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	bool bShouldBounce = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.1"))
	float LifeSpan = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	FVector MuzzleOffset = FVector(100.0f, 0.0f, 40.0f);

	// --- Presentation (Muzzle & Optional Impact Overrides) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Muzzle")
	TSoftObjectPtr<UNiagaraSystem> CastVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Muzzle")
	TSoftObjectPtr<USoundBase> CastSound;

	/** Optional override for impact particle effect (if null, uses BP template defaults) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Impact")
	TSoftObjectPtr<UNiagaraSystem> ImpactVFX;

	/** Optional override for impact sound effect (if null, uses BP template defaults) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Impact")
	TSoftObjectPtr<USoundBase> ImpactSound;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
