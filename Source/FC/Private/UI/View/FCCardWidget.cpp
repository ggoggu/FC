#include "UI/View/FCCardWidget.h"
#include "UI/ViewModel/FCCardViewModel.h"

UFCCardWidget::UFCCardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CardViewModel(nullptr)
{
	SetIsFocusable(true);
}

void UFCCardWidget::SetCardViewModel(UFCCardViewModel* InViewModel)
{
	CardViewModel = InViewModel;
	OnCardViewModelAssigned(InViewModel);
}

void UFCCardWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (CardViewModel)
	{
		CardViewModel->SetIsHovered(true);
	}

	OnCardHovered.Broadcast(this, true);
	OnCardHoverStateChanged(true);
}

void UFCCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (CardViewModel)
	{
		CardViewModel->SetIsHovered(false);
	}

	OnCardHovered.Broadcast(this, false);
	OnCardHoverStateChanged(false);
}

FReply UFCCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnCardClicked.Broadcast(this);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
