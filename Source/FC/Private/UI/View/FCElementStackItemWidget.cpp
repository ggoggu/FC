#include "UI/View/FCElementStackItemWidget.h"
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

	if (CurrentElement != EFCElement::None)
	{
		SetElement(CurrentElement, CurrentSlotIndex);
	}
}

void UFCElementStackItemWidget::SetElement(EFCElement InElement, int32 InSlotIndex)
{
	CurrentElement = InElement;
	CurrentSlotIndex = InSlotIndex;

	if (Border_Background)
	{
		Border_Background->SetBrushColor(GetColorForElement(InElement));
	}

	if (Text_ElementInitial)
	{
		Text_ElementInitial->SetText(FText::FromString(GetShortNameForElement(InElement)));
		Text_ElementInitial->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}

	OnElementUpdated(InElement, InSlotIndex);
	OnPlayStackAddedAnimation();
}

void UFCElementStackItemWidget::ClearElement()
{
	CurrentElement = EFCElement::None;
	CurrentSlotIndex = 0;

	if (Border_Background)
	{
		Border_Background->SetBrushColor(GetColorForElement(EFCElement::None));
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

	OnElementUpdated(EFCElement::None, 0);
}

FLinearColor UFCElementStackItemWidget::GetColorForElement(EFCElement InElement)
{
	switch (InElement)
	{
	case EFCElement::Fire:
		return FLinearColor(1.0f, 0.28f, 0.08f, 1.0f); // Bright Fire Orange/Red
	case EFCElement::Earth:
		return FLinearColor(0.85f, 0.55f, 0.15f, 1.0f); // Earth Amber/Brown
	case EFCElement::Water:
		return FLinearColor(0.12f, 0.65f, 1.0f, 1.0f); // Cyan/Water Blue
	case EFCElement::Wind:
		return FLinearColor(0.2f, 0.9f, 0.5f, 1.0f); // Wind Emerald
	case EFCElement::Lightning:
		return FLinearColor(1.0f, 0.88f, 0.15f, 1.0f); // Lightning Yellow
	case EFCElement::Holy:
		return FLinearColor(1.0f, 0.95f, 0.65f, 1.0f); // Holy Radiant Gold
	case EFCElement::Dark:
		return FLinearColor(0.68f, 0.25f, 0.95f, 1.0f); // Dark Violet/Purple
	case EFCElement::None:
	default:
		return FLinearColor(0.5f, 0.5f, 0.5f, 0.5f); // Neutral Slate
	}
}

FText UFCElementStackItemWidget::GetDisplayNameForElement(EFCElement InElement)
{
	return FCClassTraitUtils::GetElementDisplayName(InElement);
}

FString UFCElementStackItemWidget::GetShortNameForElement(EFCElement InElement)
{
	switch (InElement)
	{
	case EFCElement::Fire:
		return TEXT("F");
	case EFCElement::Earth:
		return TEXT("E");
	case EFCElement::Water:
		return TEXT("W");
	case EFCElement::Wind:
		return TEXT("A");
	case EFCElement::Lightning:
		return TEXT("L");
	case EFCElement::Holy:
		return TEXT("H");
	case EFCElement::Dark:
		return TEXT("D");
	case EFCElement::None:
	default:
		return TEXT("-");
	}
}
