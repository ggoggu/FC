#include "UI/View/FCCardPileWidget.h"
#include "UI/View/FCCardWidget.h"
#include "UI/ViewModel/FCCardPileViewModel.h"
#include "UI/ViewModel/FCCardViewModel.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "View/MVVMView.h"

UFCCardPileWidget::UFCCardPileWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CardGridContainer(nullptr)
	, Text_PileTitle(nullptr)
	, Text_PileCount(nullptr)
	, Btn_Close(nullptr)
	, Btn_Backdrop(nullptr)
	, PileViewModel(nullptr)
{
	SetIsFocusable(true);
}

void UFCCardPileWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Close)
	{
		Btn_Close->OnClicked.RemoveDynamic(this, &UFCCardPileWidget::HandleCloseClicked);
		Btn_Close->OnClicked.AddDynamic(this, &UFCCardPileWidget::HandleCloseClicked);
	}

	if (Btn_Backdrop)
	{
		Btn_Backdrop->OnClicked.RemoveDynamic(this, &UFCCardPileWidget::HandleCloseClicked);
		Btn_Backdrop->OnClicked.AddDynamic(this, &UFCCardPileWidget::HandleCloseClicked);
	}

	if (PileViewModel)
	{
		if (UMVVMView* View = GetExtension<UMVVMView>())
		{
			if (!View->SetViewModel(TEXT("FCCardPileViewModel"), PileViewModel))
			{
				View->SetViewModelByClass(PileViewModel);
			}

			if (View->IsConstructed())
			{
				View->ExecuteViewModelBindings(TEXT("FCCardPileViewModel"));
			}
		}

		PileViewModel->OnPileCardsUpdated.RemoveDynamic(this, &UFCCardPileWidget::HandlePileCardsUpdated);
		PileViewModel->OnPileCardsUpdated.AddDynamic(this, &UFCCardPileWidget::HandlePileCardsUpdated);
		RefreshCardWidgets();
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFCCardPileWidget::NativeDestruct()
{
	if (PileViewModel)
	{
		PileViewModel->OnPileCardsUpdated.RemoveDynamic(this, &UFCCardPileWidget::HandlePileCardsUpdated);
	}

	Super::NativeDestruct();
}

void UFCCardPileWidget::SetPileViewModel(UFCCardPileViewModel* InViewModel)
{
	if (PileViewModel)
	{
		PileViewModel->OnPileCardsUpdated.RemoveDynamic(this, &UFCCardPileWidget::HandlePileCardsUpdated);
	}

	PileViewModel = InViewModel;

	if (UMVVMView* View = GetExtension<UMVVMView>())
	{
		if (!View->SetViewModel(TEXT("FCCardPileViewModel"), InViewModel))
		{
			View->SetViewModelByClass(InViewModel);
		}

		if (View->IsConstructed())
		{
			View->ExecuteViewModelBindings(TEXT("FCCardPileViewModel"));
		}
	}

	if (PileViewModel)
	{
		PileViewModel->OnPileCardsUpdated.AddDynamic(this, &UFCCardPileWidget::HandlePileCardsUpdated);
	}

	OnPileViewModelAssigned(InViewModel);
	RefreshCardWidgets();
}

void UFCCardPileWidget::HandlePileCardsUpdated()
{
	RefreshCardWidgets();
}

void UFCCardPileWidget::HandleCloseClicked()
{
	ClosePileViewer();
}

void UFCCardPileWidget::ClosePileViewer()
{
	if (PileViewModel)
	{
		PileViewModel->ClosePile();
	}
}

void UFCCardPileWidget::RefreshCardWidgets()
{
	if (!PileViewModel)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(PileViewModel->bIsOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (Text_PileTitle)
	{
		Text_PileTitle->SetText(PileViewModel->PileTitle);
	}

	if (Text_PileCount)
	{
		Text_PileCount->SetText(PileViewModel->GetCardCountDisplayText());
	}

	if (!PileViewModel->bIsOpen)
	{
		ActiveCardWidgets.Reset();
		for (UFCCardWidget* PooledWidget : CardWidgetPool)
		{
			if (PooledWidget)
			{
				PooledWidget->SetVisibility(ESlateVisibility::Collapsed);
				PooledWidget->SetCardViewModel(nullptr);
			}
		}
		return;
	}

	if (!CardGridContainer || !CardWidgetClass)
	{
		return;
	}

	const int32 TotalCards = PileViewModel->Cards.Num();

	while (CardWidgetPool.Num() < TotalCards)
	{
		UFCCardWidget* NewCard = CreateWidget<UFCCardWidget>(this, CardWidgetClass);
		if (!NewCard)
		{
			break;
		}

		NewCard->SetDisplayOnly(true);
		CardGridContainer->AddChild(NewCard);
		CardWidgetPool.Add(NewCard);
	}

	ActiveCardWidgets.Reset(TotalCards);

	for (int32 Index = 0; Index < TotalCards; ++Index)
	{
		if (CardWidgetPool.IsValidIndex(Index))
		{
			UFCCardWidget* CardWidget = CardWidgetPool[Index];
			UFCCardViewModel* CardVM = PileViewModel->Cards[Index];
			if (CardWidget && CardVM)
			{
				if (CardWidget->GetParent() != CardGridContainer)
				{
					CardGridContainer->AddChild(CardWidget);
				}

				CardWidget->SetDisplayOnly(true);
				CardWidget->SetCardViewModel(CardVM);
				CardWidget->SetVisibility(ESlateVisibility::Visible);
				ActiveCardWidgets.Add(CardWidget);
			}
		}
	}

	for (int32 Index = TotalCards; Index < CardWidgetPool.Num(); ++Index)
	{
		if (UFCCardWidget* InactiveWidget = CardWidgetPool[Index])
		{
			InactiveWidget->SetVisibility(ESlateVisibility::Collapsed);
			InactiveWidget->SetCardViewModel(nullptr);
		}
	}
}
