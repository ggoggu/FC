#include "UI/View/FCElementOverheadWidget.h"
#include "UI/View/FCElementStackItemWidget.h"
#include "UI/ViewModel/FCElementOverheadViewModel.h"
#include "Combat/Element/FCElementComponent.h"
#include "Combat/FCCombatUtils.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"

UFCElementOverheadWidget::UFCElementOverheadWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoHideWhenEmpty = true;
}

void UFCElementOverheadWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ViewModel)
	{
		SetViewModel(NewObject<UFCElementOverheadViewModel>(this));

		// If embedded inside a UWidgetComponent on an Actor, automatically find UFCElementComponent
		if (const UWidgetComponent* WidgetComp = Cast<UWidgetComponent>(GetOuter()))
		{
			if (AActor* OwnerActor = WidgetComp->GetOwner())
			{
				InitializeForActor(OwnerActor);
			}
		}
	}
	else
	{
		HandleStacksUpdated(ViewModel->TotalStacks);
	}

	if (bAutoHideWhenEmpty && (!ViewModel || ViewModel->TotalStacks <= 0))
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFCElementOverheadWidget::NativeDestruct()
{
	if (ViewModel)
	{
		ViewModel->OnStacksUpdated.RemoveDynamic(this, &UFCElementOverheadWidget::HandleStacksUpdated);
		ViewModel->UnbindFromElementComponent();
	}

	Super::NativeDestruct();
}

void UFCElementOverheadWidget::SetViewModel(UFCElementOverheadViewModel* InViewModel)
{
	if (ViewModel)
	{
		ViewModel->OnStacksUpdated.RemoveDynamic(this, &UFCElementOverheadWidget::HandleStacksUpdated);
	}

	ViewModel = InViewModel;

	if (ViewModel)
	{
		ViewModel->OnStacksUpdated.AddUniqueDynamic(this, &UFCElementOverheadWidget::HandleStacksUpdated);
		HandleStacksUpdated(ViewModel->TotalStacks);
		OnViewModelAssigned(ViewModel);
	}
}

void UFCElementOverheadWidget::InitializeForActor(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}

	UFCElementComponent* ElementComp = UFCCombatUtils::GetElementComponent(InActor);
	InitializeForElementComponent(ElementComp);
}

void UFCElementOverheadWidget::InitializeForElementComponent(UFCElementComponent* InElementComponent)
{
	if (!ViewModel)
	{
		SetViewModel(NewObject<UFCElementOverheadViewModel>(this));
	}

	if (ViewModel)
	{
		ViewModel->BindToElementComponent(InElementComponent);
	}
}

void UFCElementOverheadWidget::HandleStacksUpdated(int32 NewTotalStacks)
{
	if (bAutoHideWhenEmpty)
	{
		SetVisibility((NewTotalStacks > 0) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (ViewModel)
	{
		if (Bar_Stacks)
		{
			Bar_Stacks->SetPercent(ViewModel->GetStackFillPercent());
		}

		if (Text_TotalStacks)
		{
			Text_TotalStacks->SetText(ViewModel->GetStackSummaryText());
		}
	}

	RefreshStackWidgets();

	OnStacksUpdated(NewTotalStacks);
}

void UFCElementOverheadWidget::RefreshStackWidgets()
{
	if (!StackListContainer)
	{
		return;
	}

	StackListContainer->ClearChildren();

	if (!ViewModel || !StackItemWidgetClass)
	{
		return;
	}

	for (UFCElementStackItemViewModel* ItemVM : ViewModel->StackList)
	{
		if (!ItemVM)
		{
			continue;
		}

		UFCElementStackItemWidget* ItemWidget = CreateWidget<UFCElementStackItemWidget>(this, StackItemWidgetClass);
		if (ItemWidget)
		{
			ItemWidget->SetItemViewModel(ItemVM);
			StackListContainer->AddChild(ItemWidget);
		}
	}
}
