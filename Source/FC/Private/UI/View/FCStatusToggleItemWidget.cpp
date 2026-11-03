#include "UI/View/FCStatusToggleItemWidget.h"
#include "UI/ViewModel/FCStatusToggleItemViewModel.h"
#include "View/MVVMView.h"

UFCStatusToggleItemWidget::UFCStatusToggleItemWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ItemViewModel(nullptr)
{
}

void UFCStatusToggleItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ItemViewModel)
	{
		if (UMVVMView* View = GetExtension<UMVVMView>())
		{
			if (!View->SetViewModel(TEXT("FCStatusToggleItemViewModel"), ItemViewModel))
			{
				View->SetViewModelByClass(ItemViewModel);
			}

			if (View->IsConstructed())
			{
				View->ExecuteViewModelBindings(TEXT("FCStatusToggleItemViewModel"));
			}
		}
	}
}

void UFCStatusToggleItemWidget::SetItemViewModel(UFCStatusToggleItemViewModel* InViewModel)
{
	ItemViewModel = InViewModel;

	if (UMVVMView* View = GetExtension<UMVVMView>())
	{
		if (!View->SetViewModel(TEXT("FCStatusToggleItemViewModel"), InViewModel))
		{
			View->SetViewModelByClass(InViewModel);
		}

		if (View->IsConstructed())
		{
			View->ExecuteViewModelBindings(TEXT("FCStatusToggleItemViewModel"));
		}
	}

	OnItemViewModelAssigned(InViewModel);
}
