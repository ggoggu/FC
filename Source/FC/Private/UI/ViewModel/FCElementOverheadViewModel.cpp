#include "UI/ViewModel/FCElementOverheadViewModel.h"
#include "Combat/Element/FCElementComponent.h"

int32 UFCElementOverheadViewModel::GetElementCount(FGameplayTag ElementTag) const
{
	if (const int32* Found = ElementCounts.Find(ElementTag))
	{
		return *Found;
	}
	return 0;
}

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
		UpdateFromStacks(TArray<FGameplayTag>());
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
		UpdateFromStacks(TArray<FGameplayTag>());
	}
}

void UFCElementOverheadViewModel::HandleStacksChanged(const TArray<FGameplayTag>& InStacks)
{
	UpdateFromStacks(InStacks);
}

void UFCElementOverheadViewModel::UpdateFromStacks(const TArray<FGameplayTag>& InStacks)
{
	TMap<FGameplayTag, int32> NewCounts;
	for (const FGameplayTag& ElemTag : InStacks)
	{
		if (ElemTag.IsValid())
		{
			NewCounts.FindOrAdd(ElemTag)++;
		}
	}

	CurrentStacks = InStacks;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CurrentStacks);

	ElementCounts = MoveTemp(NewCounts);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ElementCounts);

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
