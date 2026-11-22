#include "AbilitySystem/Effects/FCGE_Damage.h"
#include "AbilitySystem/ExecCalc/FCExecCalc_Damage.h"

UFCGE_Damage::UFCGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UFCExecCalc_Damage::StaticClass();
	Executions.Add(ExecDef);
}
