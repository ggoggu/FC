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

	InitializeWidgetPool();

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

	PooledStackWidgets.Empty();

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

void UFCElementOverheadWidget::InitializeWidgetPool()
{
	if (PooledStackWidgets.Num() > 0)
	{
		return;
	}

	if (!StackListContainer || !StackItemWidgetClass)
	{
		return;
	}

	StackListContainer->ClearChildren();
	PooledStackWidgets.Empty(MaxPooledSlots);

	for (int32 SlotIndex = 0; SlotIndex < MaxPooledSlots; ++SlotIndex)
	{
		UFCElementStackItemWidget* ItemWidget = CreateWidget<UFCElementStackItemWidget>(this, StackItemWidgetClass);
		if (ItemWidget)
		{
			ItemWidget->ClearElement();
			ItemWidget->SetVisibility(ESlateVisibility::Collapsed);
			StackListContainer->AddChild(ItemWidget);
			PooledStackWidgets.Add(ItemWidget);
		}
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
	if (PooledStackWidgets.Num() == 0)
	{
		InitializeWidgetPool();
	}

	const TArray<EFCElement>& CurrentStacks = ViewModel ? ViewModel->GetCurrentStacks() : TArray<EFCElement>();
	const int32 ActiveCount = CurrentStacks.Num();

	for (int32 Index = 0; Index < PooledStackWidgets.Num(); ++Index)
	{
		UFCElementStackItemWidget* SlotWidget = PooledStackWidgets[Index];
		if (!SlotWidget)
		{
			continue;
		}

		if (Index < ActiveCount)
		{
			SlotWidget->SetElement(CurrentStacks[Index], Index);
			SlotWidget->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			SlotWidget->ClearElement();
			SlotWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
