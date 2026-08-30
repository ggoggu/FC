#include "AbilitySystem/Effects/FCGE_Damage.h"
#include "AbilitySystem/FCAttributeSet.h"

UFCGE_Damage::UFCGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UFCAttributeSet::GetHealthAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;

	FScalableFloat ScalableMagnitude;
	ScalableMagnitude.SetValue(-1.0f);
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(ScalableMagnitude);

	Modifiers.Add(ModInfo);
}
