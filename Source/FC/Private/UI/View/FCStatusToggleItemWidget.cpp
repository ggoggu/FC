#include "UI/View/FCStatusToggleItemWidget.h"
#include "UI/ViewModel/FCStatusToggleItemViewModel.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "View/MVVMView.h"

UFCStatusToggleItemWidget::UFCStatusToggleItemWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ItemViewModel(nullptr)
	, IconImage(nullptr)
	, BorderImage(nullptr)
	, DurationBar(nullptr)
	, StackText(nullptr)
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

		ItemViewModel->OnItemDataChanged.RemoveDynamic(this, &UFCStatusToggleItemWidget::UpdateVisuals);
		ItemViewModel->OnItemDataChanged.AddDynamic(this, &UFCStatusToggleItemWidget::UpdateVisuals);

		UpdateVisuals();
	}
}

void UFCStatusToggleItemWidget::NativeDestruct()
{
	if (ItemViewModel)
	{
		ItemViewModel->OnItemDataChanged.RemoveDynamic(this, &UFCStatusToggleItemWidget::UpdateVisuals);
	}

	Super::NativeDestruct();
}

void UFCStatusToggleItemWidget::SetItemViewModel(UFCStatusToggleItemViewModel* InViewModel)
{
	if (ItemViewModel)
	{
		ItemViewModel->OnItemDataChanged.RemoveDynamic(this, &UFCStatusToggleItemWidget::UpdateVisuals);
	}

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

	if (ItemViewModel)
	{
		ItemViewModel->OnItemDataChanged.AddDynamic(this, &UFCStatusToggleItemWidget::UpdateVisuals);
	}

	UpdateVisuals();
	OnItemViewModelAssigned(InViewModel);
}

void UFCStatusToggleItemWidget::UpdateVisuals()
{
	if (!ItemViewModel)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ItemViewModel->bIsActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	// 1. Status Icon
	if (IconImage)
	{
		if (!ItemViewModel->Icon.IsNull())
		{
			if (UTexture2D* LoadedTex = ItemViewModel->Icon.LoadSynchronous())
			{
				IconImage->SetBrushFromTexture(LoadedTex, true);
				IconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
			else
			{
				IconImage->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 2. Outer Border / Glow Tint Color
	if (BorderImage)
	{
		BorderImage->SetColorAndOpacity(ItemViewModel->BorderColor);
	}

	// 3. Cooldown / Duration Progress
	if (DurationBar)
	{
		if (ItemViewModel->HasDuration())
		{
			DurationBar->SetPercent(ItemViewModel->GetDurationPercent());
			DurationBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			DurationBar->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 4. Stack Count
	if (StackText)
	{
		if (ItemViewModel->HasStackCount())
		{
			StackText->SetText(ItemViewModel->GetStackDisplayText());
			StackText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			StackText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 5. Tooltip
	if (!ItemViewModel->TooltipText.IsEmpty())
	{
		SetToolTipText(ItemViewModel->TooltipText);
	}
	else if (!ItemViewModel->DisplayName.IsEmpty())
	{
		SetToolTipText(ItemViewModel->DisplayName);
	}
}
