#include "AbilitySystem/Effects/FCGE_AttackBuff.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Data/FCGameplayTags.h"

UFCGE_AttackBuff::UFCGE_AttackBuff()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(60.0f));

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UFCAttributeSet::GetAttackPowerAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = FCGameplayTags::Data_Magnitude;
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(ModInfo);
}
