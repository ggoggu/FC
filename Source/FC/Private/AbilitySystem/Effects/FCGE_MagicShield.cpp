#include "AbilitySystem/Effects/FCGE_MagicShield.h"
#include "AbilitySystem/FCAttributeSet.h"

UFCGE_MagicShield::UFCGE_MagicShield()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UFCAttributeSet::GetShieldAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;

	FScalableFloat ScalableMagnitude;
	ScalableMagnitude.SetValue(20.0f);
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(ScalableMagnitude);

	Modifiers.Add(ModInfo);
}
