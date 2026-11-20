#include "UI/View/FCElementStackItemWidget.h"
#include "Data/Class/FCClassSubsystem.h"
#include "Data/FCGameplayTags.h"
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

	if (CurrentElementTag.IsValid() && CurrentElementTag != FCGameplayTags::Element_None)
	{
		SetElement(CurrentElementTag, CurrentSlotIndex);
	}
}

void UFCElementStackItemWidget::SetElement(FGameplayTag InElementTag, int32 InSlotIndex)
{
	CurrentElementTag = InElementTag;
	CurrentSlotIndex = InSlotIndex;

	if (Border_Background)
	{
		Border_Background->SetBrushColor(GetColorForElement(InElementTag, this));
	}

	if (Text_ElementInitial)
	{
		Text_ElementInitial->SetText(FText::FromString(GetShortNameForElement(InElementTag, this)));
		Text_ElementInitial->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}

	if (Img_ElementIcon)
	{
		UTexture2D* IconTex = nullptr;
		if (const UFCClassSubsystem* ClassSub = UFCClassSubsystem::GetClassSubsystem(this))
		{
			if (const FFCElementDefinitionRow* Row = ClassSub->FindElementRowPtr(InElementTag))
			{
				if (!Row->Icon.IsNull())
				{
					IconTex = Row->Icon.LoadSynchronous();
				}
			}
		}

		if (IconTex)
		{
			Img_ElementIcon->SetBrushFromTexture(IconTex);
			Img_ElementIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			Img_ElementIcon->SetBrushFromTexture(nullptr);
			Img_ElementIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	OnElementUpdated(InElementTag, InSlotIndex);
	OnPlayStackAddedAnimation();
}

void UFCElementStackItemWidget::ClearElement()
{
	CurrentElementTag = FGameplayTag::EmptyTag;
	CurrentSlotIndex = 0;

	if (Border_Background)
	{
		Border_Background->SetBrushColor(GetColorForElement(FCGameplayTags::Element_None, this));
	}

	if (Text_ElementInitial)
	{
		Text_ElementInitial->SetText(FText::GetEmpty());
	}

	if (Img_ElementIcon)
	{
		Img_ElementIcon->SetBrushFromTexture(nullptr);
		Img_ElementIcon->SetVisibility(ESlateVisibility::Collapsed);
	}

	OnElementUpdated(FGameplayTag::EmptyTag, 0);
}

FLinearColor UFCElementStackItemWidget::GetColorForElement(FGameplayTag InElementTag, const UObject* WorldContextObject)
{
	return FCClassTraitUtils::GetElementColor(InElementTag, WorldContextObject);
}

FText UFCElementStackItemWidget::GetDisplayNameForElement(FGameplayTag InElementTag, const UObject* WorldContextObject)
{
	return FCClassTraitUtils::GetElementDisplayName(InElementTag, WorldContextObject);
}

FString UFCElementStackItemWidget::GetShortNameForElement(FGameplayTag InElementTag, const UObject* WorldContextObject)
{
	return FCClassTraitUtils::GetElementShortCode(InElementTag, WorldContextObject);
}
