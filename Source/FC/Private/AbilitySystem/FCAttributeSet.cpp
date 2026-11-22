#include "AbilitySystem/FCAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Character/FCCharacterBase.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"

UFCAttributeSet::UFCAttributeSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitMana(50.f);
	InitMaxMana(50.f);
	InitAttackPower(0.f);
	InitShield(0.f);
	InitMaxShield(100.f);
	InitIncomingDamage(0.f);
	InitIncomingHeal(0.f);
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
	InitIncomingDamage(0.0f);
	InitIncomingHeal(0.0f);
}

void UFCAttributeSet::RefreshMana()
{
	if (GetOwningAbilitySystemComponent() && GetOwningAbilitySystemComponent()->GetOwnerActor())
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
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, Mana, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, MaxMana, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
}

void UFCAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (!FMath::IsFinite(NewValue))
	{
		NewValue = 0.f;
		return;
	}

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
	else if (Attribute == GetIncomingDamageAttribute() || Attribute == GetIncomingHealAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UFCAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}

float UFCAttributeSet::ApplyIncomingDamageValue(float RawDamage, AActor* InstigatorActor, AActor* DamageCauser, const FHitResult* HitResult)
{
	if (!FMath::IsFinite(RawDamage) || RawDamage <= 0.0f)
	{
		return 0.0f;
	}

	UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();
	const bool bHasValidAscOwner = (TargetASC && TargetASC->GetOwnerActor() != nullptr && TargetASC->GetWorld() != nullptr);

	auto SafeSetHealth = [this, bHasValidAscOwner](float NewVal)
	{
		if (bHasValidAscOwner)
		{
			SetHealth(NewVal);
		}
		else
		{
			InitHealth(NewVal);
		}
	};

	auto SafeSetShield = [this, bHasValidAscOwner](float NewVal)
	{
		if (bHasValidAscOwner)
		{
			SetShield(NewVal);
		}
		else
		{
			InitShield(NewVal);
		}
	};

	float RemainingDamage = RawDamage;
	const float CurrentShield = GetShield();
	if (CurrentShield > 0.0f)
	{
		const float Absorbed = FMath::Min(CurrentShield, RemainingDamage);
		SafeSetShield(FMath::Clamp(CurrentShield - Absorbed, 0.0f, GetMaxShield()));
		RemainingDamage -= Absorbed;
	}

	if (RemainingDamage > 0.0f)
	{
		SafeSetHealth(FMath::Clamp(GetHealth() - RemainingDamage, 0.0f, GetMaxHealth()));
	}

	AActor* TargetActor = (TargetASC && TargetASC->AbilityActorInfo.IsValid())
		? TargetASC->AbilityActorInfo->AvatarActor.Get()
		: Cast<AActor>(GetOuter());

	AActor* EffectiveCauser = DamageCauser ? DamageCauser : InstigatorActor;
	AActor* EffectiveKiller = InstigatorActor ? InstigatorActor : DamageCauser;

	if (GetHealth() <= 0.0f)
	{
		if (AFCCharacterBase* Char = Cast<AFCCharacterBase>(TargetActor))
		{
			if (!Char->IsDead())
			{
				Char->Die(EffectiveKiller);
			}
		}
	}
	else if (TargetActor)
	{
		if (AFCCharacterBase* Char = Cast<AFCCharacterBase>(TargetActor))
		{
			if (!Char->IsDead())
			{
				Char->HandleDamageTaken(RawDamage, EffectiveCauser, HitResult ? *HitResult : FHitResult());
			}
		}
		else
		{
			AController* InstigatorController = nullptr;
			if (APawn* InstigatorPawn = Cast<APawn>(InstigatorActor))
			{
				InstigatorController = InstigatorPawn->GetController();
			}
			else if (AController* AsController = Cast<AController>(InstigatorActor))
			{
				InstigatorController = AsController;
			}
			FDamageEvent DamageEvent;
			TargetActor->TakeDamage(RawDamage, DamageEvent, InstigatorController, EffectiveCauser);
		}
	}

	return RawDamage;
}

void UFCAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();
	const bool bHasValidOwner = (TargetASC && TargetASC->GetOwnerActor() != nullptr && TargetASC->GetWorld() != nullptr);

	auto SafeSetFloat = [bHasValidOwner](auto SetFn, auto InitFn, float Val)
	{
		if (bHasValidOwner)
		{
			SetFn(Val);
		}
		else
		{
			InitFn(Val);
		}
	};

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalDamage = GetIncomingDamage();
		SafeSetFloat(
			[this](float V) { SetIncomingDamage(V); },
			[this](float V) { InitIncomingDamage(V); },
			0.0f);

		if (LocalDamage > 0.0f)
		{
			AActor* Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
			AActor* Causer = Data.EffectSpec.GetEffectContext().GetEffectCauser();
			const FHitResult* HitResult = Data.EffectSpec.GetEffectContext().GetHitResult();
			ApplyIncomingDamageValue(LocalDamage, Instigator, Causer, HitResult);
		}
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingHealAttribute())
	{
		const float LocalHeal = GetIncomingHeal();
		SafeSetFloat(
			[this](float V) { SetIncomingHeal(V); },
			[this](float V) { InitIncomingHeal(V); },
			0.0f);

		if (LocalHeal > 0.0f)
		{
			const float NewHealth = FMath::Clamp(GetHealth() + LocalHeal, 0.0f, GetMaxHealth());
			SafeSetFloat(
				[this](float V) { SetHealth(V); },
				[this](float V) { InitHealth(V); },
				NewHealth);
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		const float ClampedHealth = FMath::Clamp(GetHealth(), 0.f, GetMaxHealth());
		SafeSetFloat(
			[this](float V) { SetHealth(V); },
			[this](float V) { InitHealth(V); },
			ClampedHealth);

		if (GetHealth() <= 0.0f)
		{
			AActor* TargetActor = Data.Target.AbilityActorInfo.IsValid() ? Data.Target.AbilityActorInfo->AvatarActor.Get() : nullptr;
			AActor* Killer = Data.EffectSpec.GetEffectContext().GetInstigator();
			if (!Killer)
			{
				Killer = Data.EffectSpec.GetEffectContext().GetEffectCauser();
			}
			if (AFCCharacterBase* Char = Cast<AFCCharacterBase>(TargetActor))
			{
				if (!Char->IsDead())
				{
					Char->Die(Killer);
				}
			}
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
