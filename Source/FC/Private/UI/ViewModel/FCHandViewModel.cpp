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

	// Update HandIndex and TotalCardsInHand for each child ViewModel
	for (int32 Index = 0; Index < CardsInHand.Num(); ++Index)
	{
		if (UFCCardViewModel* CardVM = CardsInHand[Index])
		{
			CardVM->SetHandIndex(Index);
			CardVM->SetTotalCardsInHand(CardsInHand.Num());
		}
	}

	// Revalidate selection
	if (SelectedCardIndex >= CardsInHand.Num() || SelectedCardIndex < 0)
	{
		ClearSelection();
	}
	else
	{
		SelectCardByIndex(SelectedCardIndex);
	}

	// Revalidate hover
	if (HoveredCardIndex >= CardsInHand.Num() || HoveredCardIndex < 0)
	{
		ClearHover();
	}
	else
	{
		HoverCardByIndex(HoveredCardIndex);
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CardsInHand);
	OnCardsUpdated.Broadcast();
}

void UFCHandViewModel::UpdateCardItem(const FFCCardItem& InItem, UFCCardSubsystem* DataSubsystem)
{
	for (UFCCardViewModel* CardVM : CardsInHand)
	{
		if (CardVM && CardVM->CardGuid == InItem.CardGuid)
		{
			UFCCardDataAsset* DataAsset = DataSubsystem ? DataSubsystem->GetCardDataAsset(InItem.CardId) : nullptr;
			CardVM->InitializeFromCardItem(InItem, DataAsset);
			OnCardsUpdated.Broadcast();
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

void UFCHandViewModel::HoverCardByGuid(const FGuid& InGuid)
{
	int32 FoundIndex = INDEX_NONE;
	for (int32 Index = 0; Index < CardsInHand.Num(); ++Index)
	{
		UFCCardViewModel* CardVM = CardsInHand[Index];
		if (CardVM)
		{
			bool bMatch = (CardVM->CardGuid == InGuid);
			CardVM->SetIsHovered(bMatch);
			if (bMatch)
			{
				FoundIndex = Index;
			}
		}
	}

	UE_MVVM_SET_PROPERTY_VALUE(HoveredCardIndex, FoundIndex);
	UE_MVVM_SET_PROPERTY_VALUE(bHasHover, FoundIndex != INDEX_NONE);
}

void UFCHandViewModel::HoverCardByIndex(int32 InIndex)
{
	if (!CardsInHand.IsValidIndex(InIndex))
	{
		ClearHover();
		return;
	}

	for (int32 Index = 0; Index < CardsInHand.Num(); ++Index)
	{
		if (UFCCardViewModel* CardVM = CardsInHand[Index])
		{
			CardVM->SetIsHovered(Index == InIndex);
		}
	}

	UE_MVVM_SET_PROPERTY_VALUE(HoveredCardIndex, InIndex);
	UE_MVVM_SET_PROPERTY_VALUE(bHasHover, true);
}

void UFCHandViewModel::ClearHover()
{
	for (UFCCardViewModel* CardVM : CardsInHand)
	{
		if (CardVM)
		{
			CardVM->SetIsHovered(false);
		}
	}

	UE_MVVM_SET_PROPERTY_VALUE(HoveredCardIndex, INDEX_NONE);
	UE_MVVM_SET_PROPERTY_VALUE(bHasHover, false);
}

UFCCardViewModel* UFCHandViewModel::GetHoveredCard() const
{
	if (CardsInHand.IsValidIndex(HoveredCardIndex))
	{
		return CardsInHand[HoveredCardIndex];
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

void UFCHandViewModel::CalculateCardFanTransform(
	int32 CardIndex,
	int32 TotalCards,
	float CardSpacing,
	float MaxHandWidth,
	float ArcHeight,
	float MaxFanAngle,
	float AngleStep,
	FVector2D& OutTranslation,
	float& OutAngle)
{
	OutTranslation = FVector2D::ZeroVector;
	OutAngle = 0.0f;

	if (TotalCards <= 0 || !FMath::IsWithinInclusive(CardIndex, 0, TotalCards - 1))
	{
		return;
	}

	if (TotalCards == 1)
	{
		OutTranslation = FVector2D::ZeroVector;
		OutAngle = 0.0f;
		return;
	}

	const float CenterIndex = (TotalCards - 1) * 0.5f;
	const float OffsetFromCenter = static_cast<float>(CardIndex) - CenterIndex;
	const float NormalizedOffset = (CenterIndex > 0.0f) ? (OffsetFromCenter / CenterIndex) : 0.0f;

	// 1. Horizontal Spacing (with automatic compression if total width exceeds MaxHandWidth)
	float EffectiveSpacing = CardSpacing;
	const float DesiredTotalWidth = (TotalCards - 1) * CardSpacing;
	if (MaxHandWidth > 0.0f && DesiredTotalWidth > MaxHandWidth && TotalCards > 1)
	{
		EffectiveSpacing = MaxHandWidth / static_cast<float>(TotalCards - 1);
	}
	OutTranslation.X = OffsetFromCenter * EffectiveSpacing;

	// 2. Vertical Arc Drop (parabolic curve: center card at Y=0, edge cards drop down by ArcHeight)
	OutTranslation.Y = ArcHeight * (NormalizedOffset * NormalizedOffset);

	// 3. Rotation Angle (with clamping to MaxFanAngle)
	float EffectiveAngleStep = AngleStep;
	const float DesiredTotalAngle = (TotalCards - 1) * AngleStep;
	if (MaxFanAngle > 0.0f && DesiredTotalAngle > MaxFanAngle && TotalCards > 1)
	{
		EffectiveAngleStep = MaxFanAngle / static_cast<float>(TotalCards - 1);
	}
	OutAngle = OffsetFromCenter * EffectiveAngleStep;
}
