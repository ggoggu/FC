#include "Data/Class/Traits/FCClassTraitBehaviors.h"
#include "Combat/FCCombatUtils.h"
#include "Combat/Element/FCElementComponent.h"
#include "Data/Class/FCClassSubsystem.h"
#include "Data/FCGameplayTags.h"

void FFCClassTrait_Mage::OnAttackHit(
	AActor* SourceActor,
	AActor* TargetActor,
	const FGameplayTagContainer& AttackTags,
	const TArray<FGameplayTag>& ElementTags) const
{
	if (!TargetActor || !TargetActor->HasAuthority() || ElementTags.Num() == 0)
	{
		return;
	}

	if (!AttackTags.HasTag(FCGameplayTags::Card_Type_Attack))
	{
		return;
	}

	if (UFCElementComponent* ElementComp = UFCCombatUtils::GetElementComponent(TargetActor))
	{
		ElementComp->AddElementStacks(ElementTags, SourceActor);
	}
}

FText FFCClassTrait_Mage::FormatCardTraitText(const TArray<FGameplayTag>& ElementTags, const UObject* WorldContextObject) const
{
	return FCClassTraitUtils::FormatElementTagsText(ElementTags, WorldContextObject);
}
