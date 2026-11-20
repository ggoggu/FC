#include "AbilitySystem/ExecCalc/FCExecCalc_Damage.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Data/FCGameplayTags.h"

namespace
{
	struct FFCDamageStatics
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);

		FFCDamageStatics()
		{
			DEFINE_ATTRIBUTE_CAPTUREDEF(UFCAttributeSet, AttackPower, Source, true);
		}
	};

	const FFCDamageStatics& DamageStatics()
	{
		static FFCDamageStatics Statics;
		return Statics;
	}
}

UFCExecCalc_Damage::UFCExecCalc_Damage()
{
	RelevantAttributesToCapture.Add(DamageStatics().AttackPowerDef);
}

void UFCExecCalc_Damage::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// Read SetByCaller base damage (supports both positive Data.Damage and negative legacy convention safely)
	const float RawDamageMag = Spec.GetSetByCallerMagnitude(FCGameplayTags::Data_Damage, false, 0.0f);
	float BaseDamage = FMath::Abs(RawDamageMag);
	if (!FMath::IsFinite(BaseDamage))
	{
		BaseDamage = 0.0f;
	}

	// Optional AttackPower scaling factor passed via Data.Magnitude (e.g. 1.0f for projectile attacks, 0.0f for flat stack detonations)
	float AttackPowerScale = Spec.GetSetByCallerMagnitude(FCGameplayTags::Data_Magnitude, false, 0.0f);
	if (!FMath::IsFinite(AttackPowerScale) || AttackPowerScale < 0.0f)
	{
		AttackPowerScale = 0.0f;
	}

	float SourceAttackPower = 0.0f;
	if (AttackPowerScale > 0.0f)
	{
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackPowerDef, EvalParams, SourceAttackPower);
		SourceAttackPower = FMath::Max(0.0f, SourceAttackPower);
	}

	const float FinalDamage = FMath::Max(0.0f, BaseDamage + (SourceAttackPower * AttackPowerScale));
	if (FinalDamage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UFCAttributeSet::GetIncomingDamageAttribute(),
			EGameplayModOp::Additive,
			FinalDamage));
	}
}
