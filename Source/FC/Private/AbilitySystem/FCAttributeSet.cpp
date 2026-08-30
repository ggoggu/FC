#include "AbilitySystem/FCAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UFCAttributeSet::UFCAttributeSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitMana(50.f);
	InitMaxMana(50.f);
}

FFCPlayerStatSaveData UFCAttributeSet::ExportStatSaveData() const
{
	FFCPlayerStatSaveData StatData;
	StatData.Health = GetHealth();
	StatData.MaxHealth = GetMaxHealth();
	StatData.Mana = GetMana();
	StatData.MaxMana = GetMaxMana();
	return StatData;
}

void UFCAttributeSet::RestoreFromStatSaveData(const FFCPlayerStatSaveData& InStatData)
{
	InitMaxHealth(InStatData.MaxHealth > 0.0f ? InStatData.MaxHealth : 100.0f);
	InitHealth(FMath::Clamp(InStatData.Health, 0.0f, GetMaxHealth()));
	InitMaxMana(InStatData.MaxMana >= 0.0f ? InStatData.MaxMana : 50.0f);
	InitMana(FMath::Clamp(InStatData.Mana, 0.0f, GetMaxMana()));
}

void UFCAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
}

void UFCAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
	}
}

void UFCAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
}

void UFCAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFCAttributeSet, Health, OldHealth);
}

void UFCAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFCAttributeSet, MaxHealth, OldMaxHealth);
}

void UFCAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFCAttributeSet, Mana, OldMana);
}

void UFCAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFCAttributeSet, MaxMana, OldMaxMana);
}
