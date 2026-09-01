#include "UI/View/FCHandWidget.h"
#include "UI/View/FCCardWidget.h"
#include "UI/ViewModel/FCHandViewModel.h"
#include "UI/ViewModel/FCCardViewModel.h"
#include "Components/PanelWidget.h"

UFCHandWidget::UFCHandWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, HandViewModel(nullptr)
{
}

void UFCHandWidget::SetHandViewModel(UFCHandViewModel* InViewModel)
{
	HandViewModel = InViewModel;
	OnHandViewModelAssigned(InViewModel);
}

void UFCHandWidget::HandleCardClicked(UFCCardWidget* ClickedCardWidget)
{
	if (!ClickedCardWidget)
	{
		return;
	}

	if (UFCCardViewModel* CardVM = ClickedCardWidget->GetCardViewModel())
	{
		if (HandViewModel)
		{
			HandViewModel->SelectCardByGuid(CardVM->CardGuid);
		}
	}

	OnHandCardSelected.Broadcast(ClickedCardWidget);
}

void UFCHandWidget::RefreshCardWidgets(UPanelWidget* TargetPanel)
{
	if (!TargetPanel || !HandViewModel || !CardWidgetClass)
	{
		return;
	}

	TargetPanel->ClearChildren();
	for (UFCCardViewModel* CardVM : HandViewModel->CardsInHand)
	{
		if (CardVM)
		{
			UFCCardWidget* CardWidget = CreateWidget<UFCCardWidget>(this, CardWidgetClass);
			if (CardWidget)
			{
				CardWidget->SetCardViewModel(CardVM);
				CardWidget->OnCardClicked.AddDynamic(this, &UFCHandWidget::HandleCardClicked);
				TargetPanel->AddChild(CardWidget);
			}
		}
	}
}
