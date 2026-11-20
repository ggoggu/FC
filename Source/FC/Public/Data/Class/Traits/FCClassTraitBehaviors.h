#pragma once

#include "CoreMinimal.h"
#include "Data/Class/Traits/FCClassTraitBehaviorBase.h"
#include "FCClassTraitBehaviors.generated.h"

/**
 * Mage Class Trait: Applies elemental GameplayTag stacks on attack card hit
 * and formats localized elemental affinity strings from DT_ElementCatalog.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Class Trait: Mage Elements"))
struct FC_API FFCClassTrait_Mage : public FFCClassTraitBehaviorBase
{
	GENERATED_BODY()

	virtual void OnAttackHit(
		AActor* SourceActor,
		AActor* TargetActor,
		const FGameplayTagContainer& AttackTags,
		const TArray<FGameplayTag>& ElementTags) const override;

	virtual FText FormatCardTraitText(const TArray<FGameplayTag>& ElementTags, const UObject* WorldContextObject) const override;

	virtual bool SupportsElements() const override { return true; }
};

/**
 * Default/Neutral Class Trait (no elemental stack application).
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Class Trait: Default"))
struct FC_API FFCClassTrait_Default : public FFCClassTraitBehaviorBase
{
	GENERATED_BODY()
};
