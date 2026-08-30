#include "Combat/Projectile/FCFireballProjectile.h"
#include "AbilitySystem/Effects/FCGE_Damage.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"

AFCFireballProjectile::AFCFireballProjectile()
{
	Damage = 1.0f;
	DamageEffectClass = UFCGE_Damage::StaticClass();
	ExplosionRadius = 0.0f;
	bPiercing = false;
	MaxPierceCount = 0;

	if (CollisionComponent)
	{
		CollisionComponent->InitSphereRadius(20.0f);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = 2500.0f;
		ProjectileMovement->MaxSpeed = 2500.0f;
		ProjectileMovement->ProjectileGravityScale = 0.0f;
		ProjectileMovement->bRotationFollowsVelocity = true;
		ProjectileMovement->bShouldBounce = false;
	}

	InitialLifeSpan = 5.0f;
}
