#include "Card/Conditions/FCCardConditions.h"
#include "Card/FCCardExecutionContext.h"
#include "Combat/FCCombatUtils.h"
#include "Combat/Element/FCElementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

bool FFCCardCondition_ConsumeElementTags::CanExecute(const FFCCardExecutionContext& Context) const
{
	if (RequiredElementTags.Num() == 0)
	{
		return true;
	}

	if (bCheckSelf)
	{
		if (UFCElementComponent* ElemComp = UFCCombatUtils::GetElementComponent(Context.CasterActor.Get()))
		{
			return ElemComp->HasElementTagStacks(RequiredElementTags);
		}
		return false;
	}

	for (const TWeakObjectPtr<AActor>& TargetPtr : Context.VerifiedTargets)
	{
		if (UFCElementComponent* ElemComp = UFCCombatUtils::GetElementComponent(TargetPtr.Get()))
		{
			if (ElemComp->HasElementTagStacks(RequiredElementTags))
			{
				return true;
			}
		}
	}

	return false;
}

bool FFCCardCondition_ConsumeElementTags::ConsumeCost(FFCCardExecutionContext& Context) const
{
	if (RequiredElementTags.Num() == 0)
	{
		return true;
	}

	if (bCheckSelf)
	{
		if (UFCElementComponent* ElemComp = UFCCombatUtils::GetElementComponent(Context.CasterActor.Get()))
		{
			return ElemComp->ConsumeElementTagStacks(RequiredElementTags);
		}
		return false;
	}

	TArray<TWeakObjectPtr<AActor>> SatisfiedTargets;
	for (const TWeakObjectPtr<AActor>& TargetPtr : Context.VerifiedTargets)
	{
		if (AActor* Target = TargetPtr.Get())
		{
			if (UFCElementComponent* ElemComp = UFCCombatUtils::GetElementComponent(Target))
			{
				if (ElemComp->ConsumeElementTagStacks(RequiredElementTags))
				{
					SatisfiedTargets.Add(Target);
				}
			}
		}
	}

	if (SatisfiedTargets.Num() > 0)
	{
		Context.VerifiedTargets = MoveTemp(SatisfiedTargets);
		return true;
	}

	return false;
}

bool FFCCardCondition_RequireTags::CanExecute(const FFCCardExecutionContext& Context) const
{
	const UAbilitySystemComponent* CasterASC = Context.CasterASC.Get();
	if (!RequiredCasterTags.IsEmpty())
	{
		if (!CasterASC || !CasterASC->HasAllMatchingGameplayTags(RequiredCasterTags))
		{
			return false;
		}
	}

	if (!BlockedCasterTags.IsEmpty())
	{
		if (CasterASC && CasterASC->HasAnyMatchingGameplayTags(BlockedCasterTags))
		{
			return false;
		}
	}

	if (!RequiredTargetTags.IsEmpty())
	{
		AActor* PrimaryTarget = Context.GetPrimaryTarget();
		const IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(PrimaryTarget);
		const UAbilitySystemComponent* TargetASC = TargetASI ? TargetASI->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC || !TargetASC->HasAllMatchingGameplayTags(RequiredTargetTags))
		{
			return false;
		}
	}

	return true;
}
