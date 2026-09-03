#include "UI/View/FCHUDWidget.h"
#include "UI/View/FCHandWidget.h"
#include "UI/ViewModel/FCHUDViewModel.h"
#include "UI/ViewModel/FCHandViewModel.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Card/FCCardDeckComponent.h"

UFCHUDWidget::UFCHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, HandWidget(nullptr)
	, HUDViewModel(nullptr)
{
}

void UFCHUDWidget::SetHUDViewModel(UFCHUDViewModel* InViewModel)
{
	HUDViewModel = InViewModel;

	if (HUDViewModel && HandWidget)
	{
		HandWidget->SetHandViewModel(HUDViewModel->GetOrCreateHandViewModel());
	}

	OnHUDViewModelAssigned(InViewModel);
}

FReply UFCHUDWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (HandWidget && HandWidget->HasHeldCard())
		{
			HandWidget->PlayHeldCard();
			return FReply::Handled();
		}
	}
	else if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (HandWidget && HandWidget->HasHeldCard())
		{
			HandWidget->ClearHeldCard();
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
