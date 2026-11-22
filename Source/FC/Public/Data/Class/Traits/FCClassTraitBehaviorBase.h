#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FCClassTraitBehaviorBase.generated.h"

class AActor;
class UObject;

/**
 * FFCClassTraitBehaviorBase
 *
 * Stateless TInstancedStruct strategy embedded in Class DataTable rows to encapsulate
 * class-specific on-hit combat hooks and card trait text formatting (OCP for Mage/Warrior/Rogue/Priest).
 */
USTRUCT(BlueprintType)
struct FC_API FFCClassTraitBehaviorBase
{
	GENERATED_BODY()

	virtual ~FFCClassTraitBehaviorBase() = default;

	/** Server-authoritative hook invoked when an attack card/projectile hits a target */
	virtual void OnAttackHit(
		AActor* SourceActor,
		AActor* TargetActor,
		const FGameplayTagContainer& AttackTags,
		const TArray<FGameplayTag>& ElementTags) const {}

	/** Formats the class-specific trait subtitle displayed on cards (e.g. Mage elements) */
	virtual FText FormatCardTraitText(const TArray<FGameplayTag>& ElementTags, const UObject* WorldContextObject) const
	{
		return FText::GetEmpty();
	}

	/** Returns true if this class uses elemental affinities */
	virtual bool SupportsElements() const
	{
		return false;
	}
};
