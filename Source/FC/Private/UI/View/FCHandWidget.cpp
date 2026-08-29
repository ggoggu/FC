#include "UI/View/FCHandWidget.h"
#include "UI/View/FCCardWidget.h"
#include "UI/ViewModel/FCHandViewModel.h"
#include "UI/ViewModel/FCCardViewModel.h"

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
