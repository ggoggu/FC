#pragma once

#include "CoreMinimal.h"
#include "Combat/Projectile/FCProjectileBase.h"
#include "FCFireballProjectile.generated.h"

/**
 * AFCFireballProjectile
 * 
 * Specialized Fireball projectile dealing 1 damage in a straight trajectory.
 */
UCLASS()
class FC_API AFCFireballProjectile : public AFCProjectileBase
{
	GENERATED_BODY()

public:
	AFCFireballProjectile();
};
