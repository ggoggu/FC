#include "UI/ViewModel/FCElementStackItemViewModel.h"
#include "Engine/Texture2D.h"

void UFCElementStackItemViewModel::SetElement(EFCElement InElement)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(Element, InElement))
	{
		SetDisplayName(GetDisplayNameForElement(InElement));
		SetElementColor(GetColorForElement(InElement));
	}
}

void UFCElementStackItemViewModel::InitializeFromElement(EFCElement InElement, int32 InSlotIndex)
{
	SetElement(InElement);
	SetSlotIndex(InSlotIndex);
	SetDisplayName(GetDisplayNameForElement(InElement));
	SetElementColor(GetColorForElement(InElement));
}

FLinearColor UFCElementStackItemViewModel::GetColorForElement(EFCElement InElement)
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

FText UFCElementStackItemViewModel::GetDisplayNameForElement(EFCElement InElement)
{
	return FCClassTraitUtils::GetElementDisplayName(InElement);
}

FString UFCElementStackItemViewModel::GetShortNameForElement(EFCElement InElement)
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
