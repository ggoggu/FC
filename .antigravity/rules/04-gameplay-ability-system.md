# Gameplay Ability System (GAS) Standards & Architecture

When implementing combat abilities, status effects (buffs/debuffs), or resource attributes using Unreal Engine 5.8 Gameplay Ability System (GAS), strictly adhere to the following standards:

---

## 1. AttributeSet Definition & Macro Hygiene

### 1. Standard Attribute Accessor Macro
Every AttributeSet header must define and utilize the standard `ATTRIBUTE_ACCESSORS` macro for defining getter, setter, init, and capture accessors:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "FCAttributeSet.generated.h"

// Standard GAS Attribute Accessor Macro
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class FC_API UFCAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UFCAttributeSet();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    // Attributes
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "FC|Attributes")
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UFCAttributeSet, Health)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "FC|Attributes")
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UFCAttributeSet, MaxHealth)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "FC|Attributes")
    FGameplayAttributeData Stamina;
    ATTRIBUTE_ACCESSORS(UFCAttributeSet, Stamina)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "FC|Attributes")
    FGameplayAttributeData MaxStamina;
    ATTRIBUTE_ACCESSORS(UFCAttributeSet, MaxStamina)

protected:
    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

    UFUNCTION()
    virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

    UFUNCTION()
    virtual void OnRep_Stamina(const FGameplayAttributeData& OldStamina);

    UFUNCTION()
    virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina);
};
```

---

## 2. Replication Lifecycle & Clamping

### 1. `GetLifetimeReplicatedProps`
Attributes must be registered with `DOREPLIFETIME_CONDITION_NOTIFY` using `REPNOTIFY_Always`:

```cpp
#include "FCAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

void UFCAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UFCAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
}
```

### 2. RepNotify Implementation
Always wrap OnRep callbacks with `GAMEPLAYATTRIBUTE_REPNOTIFY`:

```cpp
void UFCAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UFCAttributeSet, Health, OldHealth);
}

void UFCAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UFCAttributeSet, MaxHealth, OldMaxHealth);
}

void UFCAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UFCAttributeSet, Stamina, OldStamina);
}

void UFCAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UFCAttributeSet, MaxStamina, OldMaxStamina);
}
```

### 3. Value Clamping (`PreAttributeChange` vs `PostGameplayEffectExecute`)
- **`PreAttributeChange`**: Used for pre-clamping current values before mutation occurs (e.g., clamping health between 0 and MaxHealth on base value changes).
- **`PostGameplayEffectExecute`**: Server-only authoritative resolution after a Gameplay Effect executes (e.g., translating incoming Damage into Health reduction, handling death triggers, and clamping current attributes).

```cpp
void UFCAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetStaminaAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
    }
}

void UFCAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
    }
}
```

---

## 3. Gameplay Tag Taxonomy & Conventions

All Gameplay Tags must follow a structured hierarchical taxonomy:

| Category | Format Pattern | Example | Purpose |
| :--- | :--- | :--- | :--- |
| **State Tags** | `State.<Category>.<Name>` | `State.Debuff.Stun`, `State.Combat.Attacking`, `State.Movement.Dodging` | Dynamic player states, action locks, status effects. |
| **Ability Tags** | `Ability.<Type>.<Name>` | `Ability.Skill.PrimarySlash`, `Ability.Passive.Regen` | Identifiers for triggering, canceling, or blocking abilities. |
| **Data Tags** | `Data.<Domain>.<Attribute>` | `Data.Damage.Physical`, `Data.Damage.Fire`, `Data.Cooldown.PrimarySlash` | Calculation tags, damage types, element associations. |
| **Event Tags** | `Event.Montage.<Name>` | `Event.Montage.HitImpact`, `Event.Montage.SpawnProjectile` | Animation montage notify events and gameplay payload triggers. |

---

## 4. Prediction, RPC Reduction & Gameplay Cues

1. **Client Prediction**:
   - Abilities triggered by user input must utilize `TryActivateAbility` with prediction key propagation.
   - Gameplay montages and weapon swing animations run locally predicted on the autonomous client.
2. **Gameplay Cues for Audio/Visuals**:
   - **Never** send Multicast RPCs for weapon hit impacts, blood splatters, swing trails, or sound effects.
   - Dispatch Gameplay Cues (`GameplayCue.Combat.MeleeImpact`, `GameplayCue.Weapon.MuzzleFlash`) through `UAbilitySystemComponent::ExecuteGameplayCue` or Gameplay Effects.
   - Gameplay Cues automatically execute locally on predicted clients and replicate seamlessly to simulated proxies.
3. **Strict UI Decoupling**:
   - `UGameplayAbility` classes and `UAttributeSet` classes must never reference UMG widgets or HUD classes.
   - UI widgets observe attribute changes by binding to `UAbilitySystemComponent::GetGameplayAttributeValueChangeDelegate`.
