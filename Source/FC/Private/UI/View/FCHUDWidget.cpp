#include "UI/View/FCHUDWidget.h"
#include "UI/View/FCHandWidget.h"
#include "UI/ViewModel/FCHUDViewModel.h"
#include "UI/ViewModel/FCHandViewModel.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Card/FCCardDeckComponent.h"
#include "View/MVVMView.h"

UFCHUDWidget::UFCHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, HandWidget(nullptr)
	, HUDViewModel(nullptr)
{
}

void UFCHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HUDViewModel)
	{
		if (UMVVMView* View = GetExtension<UMVVMView>())
		{
			if (!View->SetViewModel(TEXT("FCHUDViewModel"), HUDViewModel))
			{
				View->SetViewModelByClass(HUDViewModel);
			}

			if (View->IsConstructed())
			{
				View->ExecuteViewModelBindings(TEXT("FCHUDViewModel"));
			}
		}
	}
}

void UFCHUDWidget::SetHUDViewModel(UFCHUDViewModel* InViewModel)
{
	HUDViewModel = InViewModel;

	if (UMVVMView* View = GetExtension<UMVVMView>())
	{
		if (!View->SetViewModel(TEXT("FCHUDViewModel"), InViewModel))
		{
			View->SetViewModelByClass(InViewModel);
		}

		if (View->IsConstructed())
		{
			View->ExecuteViewModelBindings(TEXT("FCHUDViewModel"));
		}
	}

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
