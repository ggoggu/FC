#include "UI/ViewModel/FCElementOverheadViewModel.h"
#include "Combat/Element/FCElementComponent.h"

void UFCElementOverheadViewModel::BindToElementComponent(UFCElementComponent* InComponent)
{
	if (BoundElementComponent.Get() == InComponent && InComponent != nullptr)
	{
		return;
	}

	UnbindFromElementComponent();

	BoundElementComponent = InComponent;

	if (InComponent)
	{
		InComponent->OnElementStacksChanged.AddDynamic(this, &UFCElementOverheadViewModel::HandleStacksChanged);
		SetMaxStacks(InComponent->GetMaxTotalStacks());
		RefreshFromComponent();
	}
	else
	{
		UpdateFromStacks(TArray<EFCElement>());
	}
}

void UFCElementOverheadViewModel::UnbindFromElementComponent()
{
	if (BoundElementComponent.IsValid())
	{
		BoundElementComponent->OnElementStacksChanged.RemoveDynamic(this, &UFCElementOverheadViewModel::HandleStacksChanged);
		BoundElementComponent.Reset();
	}
}

void UFCElementOverheadViewModel::RefreshFromComponent()
{
	if (BoundElementComponent.IsValid())
	{
		SetMaxStacks(BoundElementComponent->GetMaxTotalStacks());
		UpdateFromStacks(BoundElementComponent->GetAllElementStacks());
	}
	else
	{
		UpdateFromStacks(TArray<EFCElement>());
	}
}

void UFCElementOverheadViewModel::HandleStacksChanged(const TArray<EFCElement>& InStacks)
{
	UpdateFromStacks(InStacks);
}

void UFCElementOverheadViewModel::UpdateFromStacks(const TArray<EFCElement>& InStacks)
{
	int32 NewFire = 0;
	int32 NewEarth = 0;
	int32 NewWater = 0;
	int32 NewWind = 0;
	int32 NewLightning = 0;
	int32 NewHoly = 0;
	int32 NewDark = 0;

	for (const EFCElement Elem : InStacks)
	{
		switch (Elem)
		{
		case EFCElement::Fire:      NewFire++;      break;
		case EFCElement::Earth:     NewEarth++;     break;
		case EFCElement::Water:     NewWater++;     break;
		case EFCElement::Wind:      NewWind++;      break;
		case EFCElement::Lightning: NewLightning++; break;
		case EFCElement::Holy:      NewHoly++;      break;
		case EFCElement::Dark:      NewDark++;      break;
		default: break;
		}
	}

	CurrentStacks = InStacks;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CurrentStacks);

	SetFireCount(NewFire);
	SetEarthCount(NewEarth);
	SetWaterCount(NewWater);
	SetWindCount(NewWind);
	SetLightningCount(NewLightning);
	SetHolyCount(NewHoly);
	SetDarkCount(NewDark);

	SetTotalStacks(InStacks.Num());

	OnStacksUpdated.Broadcast(TotalStacks);
}

void UFCElementOverheadViewModel::SetTotalStacks(int32 InTotal)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(TotalStacks, InTotal))
	{
		SetHasAnyStack(InTotal > 0);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetStackFillPercent);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetStackSummaryText);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetVisibilityBasedOnStacks);
	}
}

void UFCElementOverheadViewModel::SetMaxStacks(int32 InMax)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(MaxStacks, InMax))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetStackFillPercent);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetStackSummaryText);
	}
}

float UFCElementOverheadViewModel::GetStackFillPercent() const
{
	return (MaxStacks > 0) ? FMath::Clamp(static_cast<float>(TotalStacks) / static_cast<float>(MaxStacks), 0.0f, 1.0f) : 0.0f;
}

FText UFCElementOverheadViewModel::GetStackSummaryText() const
{
	return FText::Format(NSLOCTEXT("FCUI", "StackSummaryFmt", "{0} / {1}"), FText::AsNumber(TotalStacks), FText::AsNumber(MaxStacks));
}

ESlateVisibility UFCElementOverheadViewModel::GetVisibilityBasedOnStacks() const
{
	return (TotalStacks > 0) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}
