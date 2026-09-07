#include "Data/Card/FCCardTypes.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "Combat/Projectile/FCProjectileDataAsset.h"

TSubclassOf<UGameplayAbility> FFCCardGameplayData::GetCardAbilityClass() const
{
	if (CardAbilityClass.IsNull())
	{
		return nullptr;
	}
	if (CardAbilityClass.IsValid())
	{
		return CardAbilityClass.Get();
	}
	return CardAbilityClass.LoadSynchronous();
}

TArray<TSubclassOf<UGameplayEffect>> FFCCardGameplayData::GetCardEffectClasses() const
{
	TArray<TSubclassOf<UGameplayEffect>> ResolvedEffects;
	ResolvedEffects.Reserve(CardEffectClasses.Num());
	for (const TSoftClassPtr<UGameplayEffect>& SoftEffect : CardEffectClasses)
	{
		if (SoftEffect.IsNull())
		{
			continue;
		}
		TSubclassOf<UGameplayEffect> Loaded = SoftEffect.IsValid() ? SoftEffect.Get() : SoftEffect.LoadSynchronous();
		if (Loaded)
		{
			ResolvedEffects.Add(Loaded);
		}
	}
	return ResolvedEffects;
}

UFCProjectileDataAsset* FFCCardGameplayData::GetProjectileDataAsset() const
{
	if (ProjectileDataAsset.IsNull())
	{
		return nullptr;
	}
	if (ProjectileDataAsset.IsValid())
	{
		return ProjectileDataAsset.Get();
	}
	return ProjectileDataAsset.LoadSynchronous();
}
