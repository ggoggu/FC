#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Card/Keywords/FCCardKeywordRuleBase.h"
#include "FCCardKeywordRules.generated.h"

/**
 * Exhaust: Moves played card to ExhaustPile instead of DiscardPile.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Keyword Rule: Exhaust"))
struct FC_API FFCCardKeywordRule_Exhaust : public FFCCardKeywordRuleBase
{
	GENERATED_BODY()

	virtual bool OverridePostPlayZone(const FFCCardExecutionContext& Context, EFCCardZone& OutZone) const override;
};

/**
 * Retain: Keeps card in Hand across hand cycle transitions.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Keyword Rule: Retain"))
struct FC_API FFCCardKeywordRule_Retain : public FFCCardKeywordRuleBase
{
	GENERATED_BODY()

	virtual bool OverrideCycleEndZone(const UFCCardDeckComponent* Deck, const FFCCardItem& Item, EFCCardZone& OutZone) const override;
};

/**
 * Ethereal: Exhausts card if still in hand at end of turn cycle.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Keyword Rule: Ethereal"))
struct FC_API FFCCardKeywordRule_Ethereal : public FFCCardKeywordRuleBase
{
	GENERATED_BODY()

	virtual bool OverrideCycleEndZone(const UFCCardDeckComponent* Deck, const FFCCardItem& Item, EFCCardZone& OutZone) const override;
};

/**
 * Innate: Prioritizes card to the top of the draw pile when initializing the deck.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Keyword Rule: Innate"))
struct FC_API FFCCardKeywordRule_Innate : public FFCCardKeywordRuleBase
{
	GENERATED_BODY()

	virtual bool ShouldPrioritizeInStartingHand() const override { return true; }
};

/**
 * Unplayable: Blocks playing the card directly from hand.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Keyword Rule: Unplayable"))
struct FC_API FFCCardKeywordRule_Unplayable : public FFCCardKeywordRuleBase
{
	GENERATED_BODY()

	virtual bool CanPlayCard(const FFCCardExecutionContext& Context) const override { return false; }
};

/**
 * FFCCardKeywordRow
 *
 * DataTable row defining a card keyword's GameplayTag, localized display text,
 * tooltip description, and polymorphic TInstancedStruct lifecycle rule.
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardKeywordRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Keyword")
	FGameplayTag KeywordTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Keyword")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Keyword")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Keyword", meta = (ExcludeBaseStruct))
	TInstancedStruct<FFCCardKeywordRuleBase> Rule;
};
