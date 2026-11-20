#include "UI/View/FCHUDWidget.h"
#include "UI/View/FCHandWidget.h"
#include "UI/View/FCStatusToggleContainerWidget.h"
#include "UI/View/FCCardPileWidget.h"
#include "UI/ViewModel/FCHUDViewModel.h"
#include "UI/ViewModel/FCHandViewModel.h"
#include "UI/ViewModel/FCStatusToggleContainerViewModel.h"
#include "UI/ViewModel/FCCardPileViewModel.h"
#include "Components/Button.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Card/FCCardDeckComponent.h"
#include "View/MVVMView.h"

UFCHUDWidget::UFCHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, HandWidget(nullptr)
	, StatusToggleContainerWidget(nullptr)
	, CardPileWidget(nullptr)
	, Btn_DrawPile(nullptr)
	, Btn_DiscardPile(nullptr)
	, HUDViewModel(nullptr)
{
}

void UFCHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_DrawPile)
	{
		Btn_DrawPile->OnClicked.RemoveDynamic(this, &UFCHUDWidget::HandleDrawPileButtonClicked);
		Btn_DrawPile->OnClicked.AddDynamic(this, &UFCHUDWidget::HandleDrawPileButtonClicked);
	}

	if (Btn_DiscardPile)
	{
		Btn_DiscardPile->OnClicked.RemoveDynamic(this, &UFCHUDWidget::HandleDiscardPileButtonClicked);
		Btn_DiscardPile->OnClicked.AddDynamic(this, &UFCHUDWidget::HandleDiscardPileButtonClicked);
	}

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

		if (HandWidget)
		{
			HandWidget->SetHandViewModel(HUDViewModel->GetOrCreateHandViewModel());
		}

		if (StatusToggleContainerWidget)
		{
			StatusToggleContainerWidget->SetContainerViewModel(HUDViewModel->GetOrCreateStatusContainerViewModel());
		}

		if (CardPileWidget)
		{
			CardPileWidget->SetPileViewModel(HUDViewModel->GetOrCreatePileViewerViewModel());
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

	if (HUDViewModel)
	{
		if (HandWidget)
		{
			HandWidget->SetHandViewModel(HUDViewModel->GetOrCreateHandViewModel());
		}

		if (StatusToggleContainerWidget)
		{
			StatusToggleContainerWidget->SetContainerViewModel(HUDViewModel->GetOrCreateStatusContainerViewModel());
		}

		if (CardPileWidget)
		{
			CardPileWidget->SetPileViewModel(HUDViewModel->GetOrCreatePileViewerViewModel());
		}
	}

	OnHUDViewModelAssigned(InViewModel);
}

bool UFCHUDWidget::IsPileViewerOpen() const
{
	return HUDViewModel && HUDViewModel->IsPileViewerOpen();
}

void UFCHUDWidget::HandleDrawPileButtonClicked()
{
	if (HandWidget && HandWidget->HasHeldCard())
	{
		HandWidget->ClearHeldCard();
	}

	if (HUDViewModel)
	{
		HUDViewModel->TogglePileViewer(EFCCardZone::DrawPile);
	}
}

void UFCHUDWidget::HandleDiscardPileButtonClicked()
{
	if (HandWidget && HandWidget->HasHeldCard())
	{
		HandWidget->ClearHeldCard();
	}

	if (HUDViewModel)
	{
		HUDViewModel->TogglePileViewer(EFCCardZone::DiscardPile);
	}
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
