#pragma once

#include "CoreMinimal.h"
#include "FCCardKeywordRuleBase.generated.h"

enum class EFCCardZone : uint8;
struct FFCCardExecutionContext;
struct FFCCardItem;
class UFCCardDeckComponent;

/**
 * FFCCardKeywordRuleBase
 *
 * Stateless TInstancedStruct lifecycle hook contract for DataTable-driven card keywords
 * (e.g., Exhaust, Retain, Ethereal, Innate, Unplayable).
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardKeywordRuleBase
{
	GENERATED_BODY()

	virtual ~FFCCardKeywordRuleBase() = default;

	/** Returns false to block playing the card (e.g. Unplayable) */
	virtual bool CanPlayCard(const FFCCardExecutionContext& Context) const
	{
		return true;
	}

	/** Optionally overrides the destination zone after a card is played (e.g. Exhaust -> ExhaustPile) */
	virtual bool OverridePostPlayZone(const FFCCardExecutionContext& Context, EFCCardZone& OutZone) const
	{
		return false;
	}

	/** Optionally overrides where a hand card moves at the end of a turn cycle (e.g. Retain -> Hand, Ethereal -> ExhaustPile) */
	virtual bool OverrideCycleEndZone(const UFCCardDeckComponent* Deck, const FFCCardItem& Item, EFCCardZone& OutZone) const
	{
		return false;
	}

	/** Returns true if cards with this keyword should be prioritized at the top of the opening draw pile (e.g. Innate) */
	virtual bool ShouldPrioritizeInStartingHand() const
	{
		return false;
	}
};
