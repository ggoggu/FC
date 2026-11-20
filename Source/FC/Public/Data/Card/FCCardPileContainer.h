#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "FCCardPileContainer.generated.h"

struct FFCCardPileContainer;
class UActorComponent;

/**
 * FFCCardPileEntry
 *
 * Replicated canonical summary entry representing how many copies of a CardId
 * reside in a hidden pile (Draw or Discard), sorted by CardId so actual pile order
 * is never leaked across the network.
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardPileEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FFCCardPileEntry()
		: CardId(NAME_None)
		, Count(0)
	{
	}

	FFCCardPileEntry(FName InCardId, int32 InCount)
		: CardId(InCardId)
		, Count(InCount)
	{
	}

	/** Static card definition ID */
	UPROPERTY(BlueprintReadOnly, Category = "Card|Pile")
	FName CardId = NAME_None;

	/** Number of copies of this card currently in the pile */
	UPROPERTY(BlueprintReadOnly, Category = "Card|Pile")
	int32 Count = 0;

	void PostReplicatedAdd(const FFCCardPileContainer& InArraySerializer);
	void PostReplicatedChange(const FFCCardPileContainer& InArraySerializer);
	void PreReplicatedRemove(const FFCCardPileContainer& InArraySerializer);
};

/**
 * FFCCardPileContainer
 *
 * Fast Array Serializer container for a player's hidden card pile summary.
 * Replicated COND_OwnerOnly to preserve Fog-of-War draw order security.
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardPileContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	FFCCardPileContainer()
		: OwnerComponent(nullptr)
	{
	}

	UPROPERTY()
	TArray<FFCCardPileEntry> Items;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FFCCardPileEntry, FFCCardPileContainer>(Items, DeltaParms, *this);
	}

	/**
	 * Aggregates and canonically sorts RawPileIds by CardId into Items.
	 * Returns true if the summary changed.
	 */
	bool SyncFromCardIds(const TArray<FName>& RawPileIds);

	/** Returns total card count across all entries */
	int32 GetTotalCardCount() const;
};

template<>
struct TStructOpsTypeTraits<FFCCardPileContainer> : public TStructOpsTypeTraitsBase2<FFCCardPileContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
