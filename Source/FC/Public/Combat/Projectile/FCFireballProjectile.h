#pragma once

#include "CoreMinimal.h"
#include "Combat/Projectile/FCProjectileBase.h"
#include "FCFireballProjectile.generated.h"

class UPointLightComponent;

/**
 * AFCFireballProjectile
 * 
 * Specialized Fireball projectile dealing 1 damage in a straight trajectory.
 * Features a glowing fiery spherical mesh and dynamic point light illumination.
 */
UCLASS()
class FC_API AFCFireballProjectile : public AFCProjectileBase
{
	GENERATED_BODY()

public:
	AFCFireballProjectile();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Projectile", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> FireLight;
};

