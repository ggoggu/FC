#include "AbilitySystem/Effects/FCGE_AttackBuff.h"
#include "AbilitySystem/FCAttributeSet.h"

UFCGE_AttackBuff::UFCGE_AttackBuff()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(60.0f));

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UFCAttributeSet::GetAttackPowerAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;

	FScalableFloat ScalableMagnitude;
	ScalableMagnitude.SetValue(1.0f);
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(ScalableMagnitude);

	Modifiers.Add(ModInfo);
}
