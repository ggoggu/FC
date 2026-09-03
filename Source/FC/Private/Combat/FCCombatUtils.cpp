#include "Combat/FCCombatUtils.h"
#include "Combat/Element/FCElementComponent.h"
#include "Character/FCCharacterBase.h"

UFCElementComponent* UFCCombatUtils::GetElementComponent(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return nullptr;
	}

	// 1. Fast check if actor is AFCCharacterBase
	if (const AFCCharacterBase* Character = Cast<AFCCharacterBase>(TargetActor))
	{
		return Character->GetElementComponent();
	}

	// 2. Generic component search fallback (e.g. for destructibles, non-character pawns)
	return TargetActor->FindComponentByClass<UFCElementComponent>();
}

void UFCCombatUtils::ApplyAttackCardHitTraits(
	AActor* SourceActor,
	AActor* TargetActor,
	EFCCharacterClass CharacterClass,
	EFCCardType CardType,
	const TArray<EFCElement>& Elements)
{
	if (!TargetActor || CardType != EFCCardType::Attack)
	{
		return;
	}

	// Server-Authoritative hit trait application
	if (!TargetActor->HasAuthority())
	{
		return;
	}

	// 1. Mage Class Trait: Apply element stacks to target
	if (CharacterClass == EFCCharacterClass::Mage && Elements.Num() > 0)
	{
		if (UFCElementComponent* ElementComp = GetElementComponent(TargetActor))
		{
			ElementComp->AddElementStacks(Elements);
		}
	}

	// 2. Future Class Extension Hooks:
	// - Warrior: Apply Stance interactions / Wound / Bleed stacks
	// - Rogue: Combo point progression / Poison stacks
	// - Priest: Divinity / Holy brand stacks
}

bool UFCCombatUtils::TryConsumeTargetElements(AActor* TargetActor, const TArray<EFCElement>& RequiredElements)
{
	if (!TargetActor || RequiredElements.Num() == 0)
	{
		return false;
	}

	if (!TargetActor->HasAuthority())
	{
		return false;
	}

	if (UFCElementComponent* ElementComp = GetElementComponent(TargetActor))
	{
		return ElementComp->ConsumeElementStacks(RequiredElements);
	}

	return false;
}

TArray<AActor*> UFCCombatUtils::FilterAndConsumeElements(const TArray<AActor*>& TargetActors, const TArray<EFCElement>& RequiredElements)
{
	TArray<AActor*> SatisfiedTargets;

	if (RequiredElements.Num() == 0)
	{
		return SatisfiedTargets;
	}

	for (AActor* Target : TargetActors)
	{
		if (Target && TryConsumeTargetElements(Target, RequiredElements))
		{
			SatisfiedTargets.Add(Target);
		}
	}

	return SatisfiedTargets;
}

int32 UFCCombatUtils::GetTargetElementCount(AActor* TargetActor, EFCElement Element)
{
	if (UFCElementComponent* ElementComp = GetElementComponent(TargetActor))
	{
		return ElementComp->GetElementCount(Element);
	}
	return 0;
}

int32 UFCCombatUtils::GetTargetTotalElementStacks(AActor* TargetActor)
{
	if (UFCElementComponent* ElementComp = GetElementComponent(TargetActor))
	{
		return ElementComp->GetTotalElementStacks();
	}
	return 0;
}
