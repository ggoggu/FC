#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Data/Card/FCCardTypes.h"
#include "FCGA_SpawnProjectile.generated.h"

class AFCProjectileBase;
class USoundBase;
class UNiagaraSystem;
class UFCProjectileDataAsset;

/**
 * UFCGA_SpawnProjectile
 * 
 * Extensible Template Gameplay Ability that calculates straight-line launch transforms
 * from the character's forward facing direction and spawns an authoritative projectile.
 * Fully supports data-driven projectile instantiation via UFCProjectileDataAsset.
 */
UCLASS()
class FC_API UFCGA_SpawnProjectile : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UFCGA_SpawnProjectile();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	const UFCProjectileDataAsset* GetProjectileDataAsset() const { return ProjectileDataAsset; }
	void SetProjectileDataAsset(const UFCProjectileDataAsset* InDataAsset) { ProjectileDataAsset = InDataAsset; }

	TSubclassOf<AFCProjectileBase> GetProjectileClass() const { return ProjectileClass; }
	void SetProjectileClass(TSubclassOf<AFCProjectileBase> InClass) { ProjectileClass = InClass; }

protected:
	/** Optional data asset configuring the spawned projectile */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Projectile")
	TObjectPtr<const UFCProjectileDataAsset> ProjectileDataAsset;

	/** Projectile actor class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Projectile")
	TSubclassOf<AFCProjectileBase> ProjectileClass;

	/** Elemental GameplayTags carried by spawned projectiles (Element.*) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile")
	TArray<FGameplayTag> ProjectileElementTags;

	/** Character class GameplayTag associated with this ability (Class.*) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile")
	FGameplayTag ClassTag;

	/** Card functional type GameplayTag (Card.Type.*) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile")
	FGameplayTag CardTypeTag;

	/** Projectile launch speed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile", meta = (ClampMin = "0.0"))
	float LaunchSpeed = 2500.0f;

	/** Base numerical damage delivered to projectile */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile", meta = (ClampMin = "0.0"))
	float BaseDamage = 1.0f;

	/** Local relative offset from character center (Forward, Right, Up) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile")
	FVector MuzzleOffset = FVector(100.0f, 0.0f, 40.0f);

	/** Audio cue played on cast / launch */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Presentation")
	TObjectPtr<USoundBase> CastSound;

	/** Niagara visual effect spawned at muzzle on cast */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Presentation")
	TObjectPtr<UNiagaraSystem> CastVFX;

	/** Computes straight-line launch transform oriented along immutable TargetData or avatar forward vector */
	virtual FTransform GetLaunchTransform(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* TriggerEventData = nullptr) const;

	/** Helper to find the directional reference component (e.g. ArrowComponent) */
	virtual USceneComponent* GetLaunchReferenceComponent(AActor* Avatar) const;
};
