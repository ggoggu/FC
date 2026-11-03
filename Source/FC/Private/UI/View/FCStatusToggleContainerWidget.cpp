#include "UI/View/FCStatusToggleContainerWidget.h"
#include "UI/View/FCStatusToggleItemWidget.h"
#include "UI/ViewModel/FCStatusToggleContainerViewModel.h"
#include "UI/ViewModel/FCStatusToggleItemViewModel.h"
#include "Components/PanelWidget.h"
#include "View/MVVMView.h"

UFCStatusToggleContainerWidget::UFCStatusToggleContainerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, StatusPanel(nullptr)
	, ItemWidgetClass(nullptr)
	, ContainerViewModel(nullptr)
{
}

void UFCStatusToggleContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ContainerViewModel)
	{
		if (UMVVMView* View = GetExtension<UMVVMView>())
		{
			if (!View->SetViewModel(TEXT("FCStatusToggleContainerViewModel"), ContainerViewModel))
			{
				View->SetViewModelByClass(ContainerViewModel);
			}

			if (View->IsConstructed())
			{
				View->ExecuteViewModelBindings(TEXT("FCStatusToggleContainerViewModel"));
			}
		}

		ContainerViewModel->OnStatusListUpdated.RemoveDynamic(this, &UFCStatusToggleContainerWidget::HandleStatusListUpdated);
		ContainerViewModel->OnStatusListUpdated.AddDynamic(this, &UFCStatusToggleContainerWidget::HandleStatusListUpdated);

		RefreshWidgets();
	}
}

void UFCStatusToggleContainerWidget::NativeDestruct()
{
	if (ContainerViewModel)
	{
		ContainerViewModel->OnStatusListUpdated.RemoveDynamic(this, &UFCStatusToggleContainerWidget::HandleStatusListUpdated);
	}

	Super::NativeDestruct();
}

void UFCStatusToggleContainerWidget::SetContainerViewModel(UFCStatusToggleContainerViewModel* InViewModel)
{
	if (ContainerViewModel)
	{
		ContainerViewModel->OnStatusListUpdated.RemoveDynamic(this, &UFCStatusToggleContainerWidget::HandleStatusListUpdated);
	}

	ContainerViewModel = InViewModel;

	if (UMVVMView* View = GetExtension<UMVVMView>())
	{
		if (!View->SetViewModel(TEXT("FCStatusToggleContainerViewModel"), InViewModel))
		{
			View->SetViewModelByClass(InViewModel);
		}

		if (View->IsConstructed())
		{
			View->ExecuteViewModelBindings(TEXT("FCStatusToggleContainerViewModel"));
		}
	}

	if (ContainerViewModel)
	{
		ContainerViewModel->OnStatusListUpdated.AddDynamic(this, &UFCStatusToggleContainerWidget::HandleStatusListUpdated);
	}

	OnContainerViewModelAssigned(InViewModel);
	RefreshWidgets();
}

void UFCStatusToggleContainerWidget::HandleStatusListUpdated()
{
	RefreshWidgets();
}

void UFCStatusToggleContainerWidget::RefreshWidgets()
{
	if (!StatusPanel || !ContainerViewModel)
	{
		return;
	}

	const TArray<TObjectPtr<UFCStatusToggleItemViewModel>>& Items = ContainerViewModel->StatusItems;
	const int32 DesiredCount = Items.Num();

	ActiveItemWidgets.Reset();

	// If no item class configured, we cannot instantiate UI items
	if (!ItemWidgetClass && WidgetPool.Num() == 0)
	{
		return;
	}

	for (int32 Index = 0; Index < DesiredCount; ++Index)
	{
		UFCStatusToggleItemViewModel* ItemVM = Items[Index].Get();
		if (!ItemVM)
		{
			continue;
		}

		UFCStatusToggleItemWidget* ItemWidget = nullptr;

		if (Index < WidgetPool.Num())
		{
			ItemWidget = WidgetPool[Index];
		}
		else if (ItemWidgetClass)
		{
			ItemWidget = CreateWidget<UFCStatusToggleItemWidget>(this, ItemWidgetClass);
			if (ItemWidget)
			{
				WidgetPool.Add(ItemWidget);
				StatusPanel->AddChild(ItemWidget);
			}
		}

		if (ItemWidget)
		{
			if (ItemWidget->GetParent() != StatusPanel)
			{
				StatusPanel->AddChild(ItemWidget);
			}

			ItemWidget->SetItemViewModel(ItemVM);
			ItemWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			ActiveItemWidgets.Add(ItemWidget);
		}
	}

	// Collapse any surplus pooled widgets
	for (int32 PoolIndex = DesiredCount; PoolIndex < WidgetPool.Num(); ++PoolIndex)
	{
		if (UFCStatusToggleItemWidget* SurplusWidget = WidgetPool[PoolIndex])
		{
			SurplusWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
