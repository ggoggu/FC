#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "FCGA_SandWall.generated.h"

class AFCSandWall;
class USoundBase;
class UNiagaraSystem;

/**
 * UFCGA_SandWall
 * 
 * Gameplay Ability that summons an authoritative Sand Wall slightly in front of the caster,
 * oriented towards the targeted enemy or aim direction.
 * The sand wall absorbs incoming hostile attacks for 1 second.
 */
UCLASS()
class FC_API UFCGA_SandWall : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UFCGA_SandWall();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	float GetSpawnForwardDistance() const { return SpawnForwardDistance; }
	float GetWallDuration() const { return WallDuration; }

protected:
	/** Actor class to spawn (defaults to AFCSandWall) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|SandWall")
	TSubclassOf<AFCSandWall> WallClass;

	/** Distance in front of caster to spawn the sand barrier (in cm, 150cm = slightly in front) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|SandWall", meta = (ClampMin = "50.0"))
	float SpawnForwardDistance = 150.0f;

	/** Duration the summoned sand wall persists (in seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|SandWall", meta = (ClampMin = "0.1"))
	float WallDuration = 1.0f;

	/** Optional audio cue played on cast */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Presentation")
	TObjectPtr<USoundBase> CastSound;

	/** Optional visual particle effect played at spawn location */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Presentation")
	TObjectPtr<UNiagaraSystem> CastVFX;

	/** Computes the spawn transform in front of the caster oriented along the target direction */
	virtual FTransform CalculateSpawnTransform(const AActor* Avatar) const;
};
