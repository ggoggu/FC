#include "UI/ViewModel/FCHandViewModel.h"
#include "UI/ViewModel/FCCardViewModel.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardHandContainer.h"
#include "Data/Card/FCCardSubsystem.h"

void UFCHandViewModel::SyncFromHandContainer(const FFCCardHandContainer& Container, UFCCardSubsystem* DataSubsystem)
{
	TArray<TObjectPtr<UFCCardViewModel>> NewList;
	NewList.Reserve(Container.Items.Num());

	// Map existing ViewModels by Guid for efficient reuse
	TMap<FGuid, UFCCardViewModel*> ExistingVMMap;
	for (UFCCardViewModel* ExistingVM : CardsInHand)
	{
		if (ExistingVM && ExistingVM->CardGuid.IsValid())
		{
			ExistingVMMap.Add(ExistingVM->CardGuid, ExistingVM);
		}
	}

	for (const FFCCardItem& Item : Container.Items)
	{
		UFCCardDataAsset* DataAsset = DataSubsystem ? DataSubsystem->GetCardDataAsset(Item.CardId) : nullptr;
		UFCCardViewModel* CardVM = nullptr;

		if (UFCCardViewModel** FoundVM = ExistingVMMap.Find(Item.CardGuid))
		{
			CardVM = *FoundVM;
			CardVM->InitializeFromCardItem(Item, DataAsset);
		}
		else
		{
			CardVM = NewObject<UFCCardViewModel>(this);
			if (CardVM)
			{
				CardVM->InitializeFromCardItem(Item, DataAsset);
			}
		}

		if (CardVM)
		{
			NewList.Add(CardVM);
		}
	}

	CardsInHand = MoveTemp(NewList);
	UE_MVVM_SET_PROPERTY_VALUE(CurrentHandCount, CardsInHand.Num());

	// Revalidate selection
	if (SelectedCardIndex >= CardsInHand.Num() || SelectedCardIndex < 0)
	{
		ClearSelection();
	}
	else
	{
		SelectCardByIndex(SelectedCardIndex);
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CardsInHand);
}

void UFCHandViewModel::UpdateCardItem(const FFCCardItem& InItem, UFCCardSubsystem* DataSubsystem)
{
	for (UFCCardViewModel* CardVM : CardsInHand)
	{
		if (CardVM && CardVM->CardGuid == InItem.CardGuid)
		{
			UFCCardDataAsset* DataAsset = DataSubsystem ? DataSubsystem->GetCardDataAsset(InItem.CardId) : nullptr;
			CardVM->InitializeFromCardItem(InItem, DataAsset);
			break;
		}
	}
}

void UFCHandViewModel::SelectCardByGuid(const FGuid& InGuid)
{
	int32 FoundIndex = INDEX_NONE;
	for (int32 Index = 0; Index < CardsInHand.Num(); ++Index)
	{
		UFCCardViewModel* CardVM = CardsInHand[Index];
		if (CardVM)
		{
			bool bMatch = (CardVM->CardGuid == InGuid);
			CardVM->SetIsSelected(bMatch);
			if (bMatch)
			{
				FoundIndex = Index;
			}
		}
	}

	UE_MVVM_SET_PROPERTY_VALUE(SelectedCardIndex, FoundIndex);
	UE_MVVM_SET_PROPERTY_VALUE(bHasSelection, FoundIndex != INDEX_NONE);
}

void UFCHandViewModel::SelectCardByIndex(int32 InIndex)
{
	if (!CardsInHand.IsValidIndex(InIndex))
	{
		ClearSelection();
		return;
	}

	for (int32 Index = 0; Index < CardsInHand.Num(); ++Index)
	{
		if (UFCCardViewModel* CardVM = CardsInHand[Index])
		{
			CardVM->SetIsSelected(Index == InIndex);
		}
	}

	UE_MVVM_SET_PROPERTY_VALUE(SelectedCardIndex, InIndex);
	UE_MVVM_SET_PROPERTY_VALUE(bHasSelection, true);
}

void UFCHandViewModel::ClearSelection()
{
	for (UFCCardViewModel* CardVM : CardsInHand)
	{
		if (CardVM)
		{
			CardVM->SetIsSelected(false);
		}
	}

	UE_MVVM_SET_PROPERTY_VALUE(SelectedCardIndex, INDEX_NONE);
	UE_MVVM_SET_PROPERTY_VALUE(bHasSelection, false);
}

UFCCardViewModel* UFCHandViewModel::GetSelectedCard() const
{
	if (CardsInHand.IsValidIndex(SelectedCardIndex))
	{
		return CardsInHand[SelectedCardIndex];
	}
	return nullptr;
}

void UFCHandViewModel::UpdatePlayability(int32 CurrentPlayerMana)
{
	for (UFCCardViewModel* CardVM : CardsInHand)
	{
		if (CardVM)
		{
			CardVM->SetIsPlayable(CardVM->ManaCost <= CurrentPlayerMana);
		}
	}
}
