#include "UI/ViewModel/FCStatusToggleItemViewModel.h"

void UFCStatusToggleItemViewModel::InitializeFromData(const FFCStatusToggleData& InData)
{
	SetStatusTag(InData.StatusTag);
	SetStatusId(InData.StatusId);
	SetDisplayName(InData.DisplayName);
	SetIcon(InData.Icon);
	SetBorderColor(InData.BorderColor);
	SetIsActive(InData.bIsActive);
	SetStackCount(InData.StackCount);
	SetDuration(InData.RemainingDuration, InData.TotalDuration);
	SetTooltipText(InData.Description);
}

void UFCStatusToggleItemViewModel::SetStackCount(int32 InCount)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(StackCount, InCount))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HasStackCount);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetStackDisplayText);
	}
}

void UFCStatusToggleItemViewModel::SetDuration(float InRemaining, float InTotal)
{
	const float SafeRemaining = FMath::Max(0.0f, InRemaining);
	const float SafeTotal = FMath::Max(0.0f, InTotal);
	const float NewRatio = (SafeTotal > 0.0f) ? FMath::Clamp(SafeRemaining / SafeTotal, 0.0f, 1.0f) : 0.0f;

	const bool bRemainingChanged = UE_MVVM_SET_PROPERTY_VALUE(RemainingDuration, SafeRemaining);
	const bool bTotalChanged = UE_MVVM_SET_PROPERTY_VALUE(TotalDuration, SafeTotal);
	const bool bRatioChanged = UE_MVVM_SET_PROPERTY_VALUE(DurationRatio, NewRatio);

	if (bRemainingChanged || bTotalChanged || bRatioChanged)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HasDuration);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetDurationPercent);
	}
}

bool UFCStatusToggleItemViewModel::HasStackCount() const
{
	return StackCount > 0;
}

bool UFCStatusToggleItemViewModel::HasDuration() const
{
	return TotalDuration > 0.0f && RemainingDuration > 0.0f;
}

FText UFCStatusToggleItemViewModel::GetStackDisplayText() const
{
	return FText::AsNumber(StackCount);
}

float UFCStatusToggleItemViewModel::GetDurationPercent() const
{
	return DurationRatio;
}
