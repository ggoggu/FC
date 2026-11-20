#include "Data/Card/FCCardPileContainer.h"
#include "Card/FCCardDeckComponent.h"
#include "Components/ActorComponent.h"

void FFCCardPileEntry::PostReplicatedAdd(const FFCCardPileContainer& InArraySerializer)
{
	if (UFCCardDeckComponent* DeckComp = Cast<UFCCardDeckComponent>(InArraySerializer.OwnerComponent))
	{
		DeckComp->NotifyPileSummariesChanged();
	}
}

void FFCCardPileEntry::PostReplicatedChange(const FFCCardPileContainer& InArraySerializer)
{
	if (UFCCardDeckComponent* DeckComp = Cast<UFCCardDeckComponent>(InArraySerializer.OwnerComponent))
	{
		DeckComp->NotifyPileSummariesChanged();
	}
}

void FFCCardPileEntry::PreReplicatedRemove(const FFCCardPileContainer& InArraySerializer)
{
	if (UFCCardDeckComponent* DeckComp = Cast<UFCCardDeckComponent>(InArraySerializer.OwnerComponent))
	{
		DeckComp->NotifyPileSummariesChanged();
	}
}

bool FFCCardPileContainer::SyncFromCardIds(const TArray<FName>& RawPileIds)
{
	TMap<FName, int32> CountsByCardId;
	for (const FName& CardId : RawPileIds)
	{
		if (!CardId.IsNone())
		{
			CountsByCardId.FindOrAdd(CardId) += 1;
		}
	}

	TArray<FName> SortedIds;
	CountsByCardId.GetKeys(SortedIds);
	SortedIds.Sort([](const FName& A, const FName& B)
	{
		return A.LexicalLess(B);
	});

	if (SortedIds.Num() == Items.Num())
	{
		bool bSameKeys = true;
		bool bAnyCountChanged = false;
		for (int32 Index = 0; Index < SortedIds.Num(); ++Index)
		{
			if (Items[Index].CardId != SortedIds[Index])
			{
				bSameKeys = false;
				break;
			}
			const int32 NewCount = CountsByCardId[SortedIds[Index]];
			if (Items[Index].Count != NewCount)
			{
				bAnyCountChanged = true;
			}
		}

		if (bSameKeys)
		{
			if (!bAnyCountChanged)
			{
				return false;
			}

			for (int32 Index = 0; Index < SortedIds.Num(); ++Index)
			{
				const int32 NewCount = CountsByCardId[SortedIds[Index]];
				if (Items[Index].Count != NewCount)
				{
					Items[Index].Count = NewCount;
					MarkItemDirty(Items[Index]);
				}
			}
			return true;
		}
	}

	Items.Reset(SortedIds.Num());
	for (const FName& CardId : SortedIds)
	{
		Items.Emplace(CardId, CountsByCardId[CardId]);
	}
	MarkArrayDirty();
	return true;
}

int32 FFCCardPileContainer::GetTotalCardCount() const
{
	int32 Total = 0;
	for (const FFCCardPileEntry& Entry : Items)
	{
		Total += Entry.Count;
	}
	return Total;
}
