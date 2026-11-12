#include "AbilitySystem/Effects/FCGE_Damage.h"
#include "AbilitySystem/FCAttributeSet.h"

UFCGE_Damage::UFCGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UFCAttributeSet::GetHealthAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = FName("Data.Damage");
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(ModInfo);
}
