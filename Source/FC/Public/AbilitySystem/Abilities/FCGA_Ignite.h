#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Data/Card/FCCardTypes.h"
#include "FCGA_Ignite.generated.h"

class USoundBase;
class UNiagaraSystem;
class UGameplayEffect;

/**
 * UFCGA_Ignite
 * 
 * Specialized Mage ability for the "Ignite" (점화) card.
 * Targets all nearby enemies within Radius, querying their Fire stack count,
 * and deals 10 damage per Fire stack.
 */
UCLASS()
class FC_API UFCGA_Ignite : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UFCGA_Ignite();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	/** Base damage multiplier dealt per 1 Fire stack on the target (Default: 10.0) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ignite|Combat", meta = (ClampMin = "0.0"))
	float DamagePerStack = 10.0f;

	/** Radius around caster in centimeters to search for nearby enemies (Default: 1000.0cm = 10m) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ignite|Combat", meta = (ClampMin = "0.0"))
	float Radius = 1000.0f;

	/**
	 * Whether to consume (remove) Fire stacks after dealing damage.
	 * Default is false (stacks are preserved according to card rules).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ignite|Combat")
	bool bConsumeFireStacks = false;

	/** Optional Gameplay Effect used to deliver damage to targets */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ignite|Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** Audio cue played on ignition cast */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ignite|Presentation")
	TObjectPtr<USoundBase> CastSound;

	/** Niagara visual effect spawned at caster location on ignition cast */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ignite|Presentation")
	TObjectPtr<UNiagaraSystem> CastVFX;

	/** Niagara visual effect spawned at each ignited enemy target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ignite|Presentation")
	TObjectPtr<UNiagaraSystem> TargetImpactVFX;

	/**
	 * Static modular combat helper:
	 * Deals (FireStacks * InDamagePerStack) to TargetActor from InstigatorActor.
	 * Returns the calculated damage applied, or 0 if no Fire stacks or invalid target.
	 */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat|Ignite")
	static float ApplyIgniteToTarget(
		AActor* InstigatorActor,
		AActor* TargetActor,
		float InDamagePerStack = 10.0f,
		bool bInConsumeStacks = false,
		TSubclassOf<UGameplayEffect> InDamageEffectClass = nullptr
	);

	/** Finds all viable nearby enemy actors around CenterActor within InRadius */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat|Ignite")
	static TArray<AActor*> FindNearbyEnemies(AActor* CenterActor, float InRadius);

	/** Helper to determine if an actor is considered a hostile enemy */
	UFUNCTION(BlueprintPure, Category = "FC|Combat|Ignite")
	static bool IsEnemyActor(AActor* SourceActor, AActor* CandidateActor);
};
