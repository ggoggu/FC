#include "Combat/Projectile/FCProjectileDataAsset.h"
#include "AbilitySystem/Effects/FCGE_Damage.h"
#include "Data/FCGameplayTags.h"

UFCProjectileDataAsset::UFCProjectileDataAsset()
{
	Damage = 1.0f;
	DamageEffectClass = UFCGE_Damage::StaticClass();
	ExplosionRadius = 0.0f;
	bPiercing = false;
	MaxPierceCount = 0;
	ProjectileElementTags = { FCGameplayTags::Element_Fire };
	SourceClassTag = FCGameplayTags::Class_Mage;
	SourceCardTypeTag = FCGameplayTags::Card_Type_Attack;

	LaunchSpeed = 2500.0f;
	MaxSpeed = 2500.0f;
	GravityScale = 0.0f;
	bRotationFollowsVelocity = true;
	bShouldBounce = false;
	LifeSpan = 5.0f;
	MuzzleOffset = FVector(100.0f, 0.0f, 40.0f);
}

FPrimaryAssetId UFCProjectileDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType("Projectile"), GetFName());
}
