#include "AbilitySystem/Abilities/FCGA_Fireball.h"
#include "Combat/Projectile/FCFireballProjectile.h"

UFCGA_Fireball::UFCGA_Fireball()
{
	ProjectileClass = AFCFireballProjectile::StaticClass();
	BaseDamage = 1.0f;
	LaunchSpeed = 2500.0f;
	MuzzleOffset = FVector(100.0f, 0.0f, 40.0f);
}
