#include "AbilitySystem/FCAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UFCAttributeSet::UFCAttributeSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitMana(50.f);
	InitMaxMana(50.f);
	InitAttackPower(0.f);
	InitShield(0.f);
	InitMaxShield(100.f);
}

FFCPlayerStatSaveData UFCAttributeSet::ExportStatSaveData() const
{
	FFCPlayerStatSaveData StatData;
	StatData.Health = GetHealth();
	StatData.MaxHealth = GetMaxHealth();
	StatData.Mana = GetMana();
	StatData.MaxMana = GetMaxMana();
	StatData.AttackPower = GetAttackPower();
	return StatData;
}

void UFCAttributeSet::RestoreFromStatSaveData(const FFCPlayerStatSaveData& InStatData)
{
	InitMaxHealth(InStatData.MaxHealth > 0.0f ? InStatData.MaxHealth : 100.0f);
	InitHealth(FMath::Clamp(InStatData.Health, 0.0f, GetMaxHealth()));
	InitMaxMana(InStatData.MaxMana >= 0.0f ? InStatData.MaxMana : 50.0f);
	InitMana(FMath::Clamp(InStatData.Mana, 0.0f, GetMaxMana()));
	InitAttackPower(FMath::Max(InStatData.AttackPower, 0.0f));
	InitShield(0.0f);
	InitMaxShield(100.0f);
}

void UFCAttributeSet::RefreshMana()
{
	if (GetOwningAbilitySystemComponent())
	{
		SetMana(GetMaxMana());
	}
	else
	{
		InitMana(GetMaxMana());
	}
}

void UFCAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
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
	else if (Attribute == GetAttackPowerAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxShield());
	}
	else if (Attribute == GetMaxShieldAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UFCAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		if (Data.EvaluatedData.Magnitude < 0.f)
		{
			const float IncomingDamage = -Data.EvaluatedData.Magnitude;
			const float CurrentShield = GetShield();

			if (CurrentShield > 0.f)
			{
				if (CurrentShield >= IncomingDamage)
				{
					// Shield completely absorbs damage
					SetShield(CurrentShield - IncomingDamage);
					SetHealth(FMath::Clamp(GetHealth() + IncomingDamage, 0.f, GetMaxHealth()));
				}
				else
				{
					// Shield partially absorbs damage
					const float Absorbed = CurrentShield;
					SetShield(0.f);
					SetHealth(FMath::Clamp(GetHealth() + Absorbed, 0.f, GetMaxHealth()));
				}
			}
			else
			{
				SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
			}
		}
		else
		{
			SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
		}
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
	else if (Data.EvaluatedData.Attribute == GetAttackPowerAttribute())
	{
		SetAttackPower(FMath::Max(GetAttackPower(), 0.f));
	}
	else if (Data.EvaluatedData.Attribute == GetShieldAttribute())
	{
		SetShield(FMath::Clamp(GetShield(), 0.f, GetMaxShield()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxShieldAttribute())
	{
		SetMaxShield(FMath::Max(GetMaxShield(), 0.f));
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

void UFCAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFCAttributeSet, AttackPower, OldAttackPower);
}

void UFCAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFCAttributeSet, Shield, OldShield);
}

void UFCAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFCAttributeSet, MaxShield, OldMaxShield);
}
