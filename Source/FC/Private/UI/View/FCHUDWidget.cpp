#include "UI/View/FCHUDWidget.h"
#include "UI/ViewModel/FCHUDViewModel.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Card/FCCardDeckComponent.h"

UFCHUDWidget::UFCHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, HUDViewModel(nullptr)
{
}

void UFCHUDWidget::SetHUDViewModel(UFCHUDViewModel* InViewModel)
{
	HUDViewModel = InViewModel;
	OnHUDViewModelAssigned(InViewModel);
}
