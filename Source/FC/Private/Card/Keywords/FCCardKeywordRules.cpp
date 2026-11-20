#include "Card/Keywords/FCCardKeywordRules.h"
#include "Data/Card/FCCardTypes.h"

bool FFCCardKeywordRule_Exhaust::OverridePostPlayZone(const FFCCardExecutionContext& Context, EFCCardZone& OutZone) const
{
	OutZone = EFCCardZone::ExhaustPile;
	return true;
}

bool FFCCardKeywordRule_Retain::OverrideCycleEndZone(const UFCCardDeckComponent* Deck, const FFCCardItem& Item, EFCCardZone& OutZone) const
{
	OutZone = EFCCardZone::Hand;
	return true;
}

bool FFCCardKeywordRule_Ethereal::OverrideCycleEndZone(const UFCCardDeckComponent* Deck, const FFCCardItem& Item, EFCCardZone& OutZone) const
{
	OutZone = EFCCardZone::ExhaustPile;
	return true;
}
