#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Card/Conditions/FCCardConditionBase.h"
#include "FCCardConditions.generated.h"

/**
 * Requires and atomically consumes elemental stacks (by GameplayTag) from target(s) or self.
 * Supports both hard-blocking card play and soft-fizzling (card plays & spends mana, conditional effects skipped).
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Condition: Consume Element Tags"))
struct FC_API FFCCardCondition_ConsumeElementTags : public FFCCardConditionBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Condition")
	TArray<FGameplayTag> RequiredElementTags;

	/** If true, consumes elements from caster instead of verified target(s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Condition")
	bool bCheckSelf = false;

	/** If false, card is still played and mana spent, but conditional effects fizzle when unmet */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Condition")
	bool bBlockCardPlayIfUnmet = false;

	virtual bool CanExecute(const FFCCardExecutionContext& Context) const override;
	virtual bool ConsumeCost(FFCCardExecutionContext& Context) const override;
	virtual bool BlocksCardPlayWhenUnmet() const override { return bBlockCardPlayIfUnmet; }
};

/**
 * Checks GameplayTag requirements on the caster and/or primary target ASC.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Condition: Require Gameplay Tags"))
struct FC_API FFCCardCondition_RequireTags : public FFCCardConditionBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Condition")
	FGameplayTagContainer RequiredCasterTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Condition")
	FGameplayTagContainer BlockedCasterTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Condition")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Condition")
	bool bBlockCardPlayIfUnmet = true;

	virtual bool CanExecute(const FFCCardExecutionContext& Context) const override;
	virtual bool ConsumeCost(FFCCardExecutionContext& Context) const override { return CanExecute(Context); }
	virtual bool BlocksCardPlayWhenUnmet() const override { return bBlockCardPlayIfUnmet; }
};
