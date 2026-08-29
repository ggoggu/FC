#include "UI/ViewModel/FCHUDViewModel.h"
#include "UI/ViewModel/FCHandViewModel.h"

UFCHandViewModel* UFCHUDViewModel::GetOrCreateHandViewModel()
{
	if (!HandViewModel)
	{
		HandViewModel = NewObject<UFCHandViewModel>(this);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HandViewModel);
	}
	return HandViewModel;
}

void UFCHUDViewModel::SetCurrentHealth(int32 InHealth)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(CurrentHealth, InHealth))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthDisplayText);
	}
}

void UFCHUDViewModel::SetMaxHealth(int32 InMaxHealth)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, InMaxHealth))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthDisplayText);
	}
}

void UFCHUDViewModel::SetCurrentMana(int32 InMana)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(CurrentMana, InMana))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetManaPercent);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetManaDisplayText);

		if (HandViewModel)
		{
			HandViewModel->UpdatePlayability(CurrentMana);
		}
	}
}

void UFCHUDViewModel::SetMaxMana(int32 InMaxMana)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(MaxMana, InMaxMana))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetManaPercent);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetManaDisplayText);
	}
}

float UFCHUDViewModel::GetHealthPercent() const
{
	return (MaxHealth > 0) ? FMath::Clamp(static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth), 0.0f, 1.0f) : 0.0f;
}

float UFCHUDViewModel::GetManaPercent() const
{
	return (MaxMana > 0) ? FMath::Clamp(static_cast<float>(CurrentMana) / static_cast<float>(MaxMana), 0.0f, 1.0f) : 0.0f;
}

FText UFCHUDViewModel::GetHealthDisplayText() const
{
	return FText::Format(NSLOCTEXT("FC_HUD", "HealthFormat", "{0} / {1}"), FText::AsNumber(CurrentHealth), FText::AsNumber(MaxHealth));
}

FText UFCHUDViewModel::GetManaDisplayText() const
{
	return FText::Format(NSLOCTEXT("FC_HUD", "ManaFormat", "{0} / {1}"), FText::AsNumber(CurrentMana), FText::AsNumber(MaxMana));
}
