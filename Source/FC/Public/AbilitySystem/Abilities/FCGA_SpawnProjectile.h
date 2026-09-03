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

protected:
	/** Optional data asset configuring the spawned projectile */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile")
	TObjectPtr<const UFCProjectileDataAsset> ProjectileDataAsset;

	/** Projectile actor class to spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile")
	TSubclassOf<AFCProjectileBase> ProjectileClass;

	/** Elemental affinities carried by spawned projectiles */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile")
	TArray<EFCElement> ProjectileElements;

	/** Character class associated with this ability */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile")
	EFCCharacterClass CharacterClass = EFCCharacterClass::Mage;

	/** Card functional type */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile")
	EFCCardType CardType = EFCCardType::Attack;

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

	/** Computes straight-line launch transform oriented along character Arrow component or forward vector */
	virtual FTransform GetLaunchTransform(const FGameplayAbilityActorInfo* ActorInfo) const;

	/** Helper to find the directional reference component (e.g. ArrowComponent) */
	virtual USceneComponent* GetLaunchReferenceComponent(AActor* Avatar) const;
};
