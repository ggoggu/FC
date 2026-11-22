#include "UI/ViewModel/FCCardPileViewModel.h"
#include "UI/ViewModel/FCCardViewModel.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardHandContainer.h"
#include "Data/Card/FCCardSubsystem.h"

void UFCCardPileViewModel::UpdatePileSummaries(
	const FFCCardPileContainer& InDrawSummary,
	const FFCCardPileContainer& InDiscardSummary,
	UFCCardSubsystem* InCardSubsystem)
{
	CachedDrawEntries = InDrawSummary.Items;
	CachedDiscardEntries = InDiscardSummary.Items;

	if (InCardSubsystem)
	{
		CachedCardSubsystem = InCardSubsystem;
	}

	if (bIsOpen)
	{
		RebuildCards(CachedCardSubsystem.Get());
	}
}

void UFCCardPileViewModel::OpenPile(EFCCardZone InZone, UFCCardSubsystem* InCardSubsystem)
{
	if (InZone != EFCCardZone::DrawPile && InZone != EFCCardZone::DiscardPile)
	{
		return;
	}

	if (InCardSubsystem)
	{
		CachedCardSubsystem = InCardSubsystem;
	}
	else if (!CachedCardSubsystem.IsValid())
	{
		CachedCardSubsystem = UFCCardSubsystem::GetCardSubsystem(this);
	}

	UE_MVVM_SET_PROPERTY_VALUE(ActiveZone, InZone);
	UE_MVVM_SET_PROPERTY_VALUE(PileTitle, GetTitleForZone(InZone));
	UE_MVVM_SET_PROPERTY_VALUE(bIsOpen, true);

	RebuildCards(CachedCardSubsystem.Get());
}

void UFCCardPileViewModel::TogglePile(EFCCardZone InZone, UFCCardSubsystem* InCardSubsystem)
{
	if (bIsOpen && ActiveZone == InZone)
	{
		ClosePile();
		return;
	}

	OpenPile(InZone, InCardSubsystem);
}

void UFCCardPileViewModel::ClosePile()
{
	if (UE_MVVM_SET_PROPERTY_VALUE(bIsOpen, false))
	{
		OnPileCardsUpdated.Broadcast();
	}
}

FText UFCCardPileViewModel::GetCardCountDisplayText() const
{
	return FText::Format(NSLOCTEXT("FC_PileViewer", "PileCountFormat", "{0}장"), FText::AsNumber(CardCount));
}

void UFCCardPileViewModel::RebuildCards(UFCCardSubsystem* InCardSubsystem)
{
	UFCCardSubsystem* Subsystem = InCardSubsystem ? InCardSubsystem : UFCCardSubsystem::GetCardSubsystem(this);
	const TArray<FFCCardPileEntry>& SourceEntries = (ActiveZone == EFCCardZone::DiscardPile)
		? CachedDiscardEntries
		: CachedDrawEntries;

	TArray<FName> ExpandedCardIds;
	for (const FFCCardPileEntry& Entry : SourceEntries)
	{
		if (Entry.CardId.IsNone() || Entry.Count <= 0)
		{
			continue;
		}
		for (int32 CopyIdx = 0; CopyIdx < Entry.Count; ++CopyIdx)
		{
			ExpandedCardIds.Add(Entry.CardId);
		}
	}

	if (Subsystem)
	{
		Subsystem->SortCardIdsForDisplay(ExpandedCardIds);
	}
	else
	{
		ExpandedCardIds.Sort([](const FName& A, const FName& B)
		{
			return A.LexicalLess(B);
		});
	}

	TArray<TObjectPtr<UFCCardViewModel>> NewList;
	NewList.Reserve(ExpandedCardIds.Num());

	for (int32 Index = 0; Index < ExpandedCardIds.Num(); ++Index)
	{
		const FName CardId = ExpandedCardIds[Index];
		UFCCardDataAsset* DataAsset = Subsystem ? Subsystem->GetCardDataAsset(CardId) : nullptr;

		UFCCardViewModel* CardVM = Cards.IsValidIndex(Index) ? Cards[Index].Get() : nullptr;
		if (!CardVM)
		{
			CardVM = NewObject<UFCCardViewModel>(this);
		}

		if (CardVM)
		{
			FFCCardItem DisplayItem(FGuid(), CardId, 0, false);
			CardVM->InitializeFromCardItem(DisplayItem, DataAsset);
			CardVM->SetIsPlayable(true);
			CardVM->SetIsHovered(false);
			CardVM->SetIsSelected(false);
			CardVM->SetIsHeld(false);
			NewList.Add(CardVM);
		}
	}

	Cards = MoveTemp(NewList);
	if (UE_MVVM_SET_PROPERTY_VALUE(CardCount, Cards.Num()))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCardCountDisplayText);
	}
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Cards);
	OnPileCardsUpdated.Broadcast();
}

FText UFCCardPileViewModel::GetTitleForZone(EFCCardZone InZone)
{
	if (InZone == EFCCardZone::DiscardPile)
	{
		return NSLOCTEXT("FC_PileViewer", "DiscardPileTitle", "버린 카드 (Discard)");
	}
	return NSLOCTEXT("FC_PileViewer", "DrawPileTitle", "남은 덱 (Deck)");
}
