#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "FCExecCalc_Damage.generated.h"

/**
 * UFCExecCalc_Damage
 *
 * Centralized GAS ExecutionCalculation pipe for all combat damage:
 * Reads SetByCaller Data.Damage + optional Source.AttackPower * Data.Magnitude scale
 * and outputs positive final damage into UFCAttributeSet::IncomingDamage (Meta-Attribute).
 */
UCLASS()
class FC_API UFCExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UFCExecCalc_Damage();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
