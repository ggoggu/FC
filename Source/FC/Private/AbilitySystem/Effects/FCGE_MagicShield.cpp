#include "AbilitySystem/Effects/FCGE_MagicShield.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Data/FCGameplayTags.h"

UFCGE_MagicShield::UFCGE_MagicShield()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UFCAttributeSet::GetShieldAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = FCGameplayTags::Data_Magnitude;
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(ModInfo);
}
