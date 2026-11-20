#pragma once

#include "CoreMinimal.h"
#include "FCCardConditionBase.generated.h"

struct FFCCardExecutionContext;

/**
 * FFCCardConditionBase
 *
 * Stateless TInstancedStruct contract for card play preconditions and resource consumption
 * (e.g., target element consumption, required caster/target tags, stance/combo checks).
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardConditionBase
{
	GENERATED_BODY()

	virtual ~FFCCardConditionBase() = default;

	/** Evaluates whether the precondition is currently satisfied */
	virtual bool CanExecute(const FFCCardExecutionContext& Context) const
	{
		return true;
	}

	/** Authoritatively consumes required resources when executing the card */
	virtual bool ConsumeCost(FFCCardExecutionContext& Context) const
	{
		return true;
	}

	/**
	 * If true, failing this condition aborts card play before mana/hand mutation.
	 * If false, the card is still played and mana is spent, but conditional effects fizzle.
	 */
	virtual bool BlocksCardPlayWhenUnmet() const
	{
		return true;
	}

	/** Collects any soft asset references for async deck preloading */
	virtual void GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const {}
};
