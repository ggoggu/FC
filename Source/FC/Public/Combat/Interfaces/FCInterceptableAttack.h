#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "FCInterceptableAttack.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UFCInterceptableAttack : public UInterface
{
	GENERATED_BODY()
};

/**
 * IFCInterceptableAttack
 *
 * Contract for attacks (projectiles, beams, dashes) that can interact with tactical barriers
 * such as AFCSandWall without hardcoding IsA<AFCProjectileBase>() checks.
 */
class FC_API IFCInterceptableAttack
{
	GENERATED_BODY()

public:
	/** Returns gameplay tags describing this attack (e.g. Combat.Attack.Projectile, Combat.Attack.Unblockable) */
	virtual FGameplayTagContainer GetAttackTags() const = 0;

	/** Returns true if InterceptorActor is allowed to absorb/intercept this attack */
	virtual bool CanBeInterceptedBy(const AActor* InterceptorActor) const = 0;

	/** Authoritatively invoked on the server when this attack is intercepted */
	virtual void OnInterceptedBy(AActor* InterceptorActor, const FVector& ImpactLocation) = 0;
};
