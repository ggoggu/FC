#include "UI/View/FCElementStackItemWidget.h"
#include "UI/ViewModel/FCElementStackItemViewModel.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

UFCElementStackItemWidget::UFCElementStackItemWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFCElementStackItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ItemViewModel)
	{
		SetItemViewModel(ItemViewModel);
	}
}

void UFCElementStackItemWidget::SetItemViewModel(UFCElementStackItemViewModel* InViewModel)
{
	ItemViewModel = InViewModel;

	if (ItemViewModel)
	{
		if (Border_Background)
		{
			Border_Background->SetBrushColor(ItemViewModel->ElementColor);
		}

		if (Text_ElementInitial)
		{
			Text_ElementInitial->SetText(FText::FromString(UFCElementStackItemViewModel::GetShortNameForElement(ItemViewModel->Element)));
			Text_ElementInitial->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		}

		if (Img_ElementIcon)
		{
			if (ItemViewModel->ElementIcon.IsValid())
			{
				Img_ElementIcon->SetBrushFromTexture(ItemViewModel->ElementIcon.Get());
				Img_ElementIcon->SetVisibility(ESlateVisibility::Visible);
			}
			else if (!ItemViewModel->ElementIcon.IsNull())
			{
				// Asynchronously stream or load soft texture
				if (UTexture2D* LoadedTex = ItemViewModel->ElementIcon.LoadSynchronous())
				{
					Img_ElementIcon->SetBrushFromTexture(LoadedTex);
					Img_ElementIcon->SetVisibility(ESlateVisibility::Visible);
				}
			}
		}

		OnItemViewModelAssigned(ItemViewModel);
		OnPlayStackAddedAnimation();
	}
}
