#include "UI/ClassSelect/FCClassSelectItemWidget.h"
#include "UI/ClassSelect/FCClassSelectItemViewModel.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UFCClassSelectItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Select)
	{
		Btn_Select->OnClicked.RemoveDynamic(this, &UFCClassSelectItemWidget::HandleSelectButtonClicked);
		Btn_Select->OnClicked.AddDynamic(this, &UFCClassSelectItemWidget::HandleSelectButtonClicked);
	}
}

void UFCClassSelectItemWidget::InitializeWidget(UFCClassSelectItemViewModel* InViewModel)
{
	ViewModel = InViewModel;

	if (ViewModel)
	{
		UpdateVisualState();
	}
}

void UFCClassSelectItemWidget::HandleSelectButtonClicked()
{
	if (ViewModel)
	{
		OnItemClicked.Broadcast(ViewModel->GetClassType());
	}
}

void UFCClassSelectItemWidget::UpdateVisualState()
{
	if (!ViewModel)
	{
		return;
	}

	if (Text_ClassName)
	{
		Text_ClassName->SetText(ViewModel->ClassDisplayName);
	}

	if (Img_ClassIcon && !ViewModel->ClassIcon.IsNull())
	{
		Img_ClassIcon->SetBrushFromSoftTexture(ViewModel->ClassIcon);
	}

	if (SelectionOverlay)
	{
		SelectionOverlay->SetVisibility(ViewModel->GetIsSelected() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (LockOverlay)
	{
		LockOverlay->SetVisibility(ViewModel->GetIsSelectable() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (Text_LockReason)
	{
		Text_LockReason->SetText(ViewModel->LockReasonText);
	}

	if (Btn_Select)
	{
		Btn_Select->SetIsEnabled(ViewModel->GetIsSelectable());
	}

	BP_OnSelectionChanged(ViewModel->GetIsSelected());
	BP_OnSelectableChanged(ViewModel->GetIsSelectable());
}
