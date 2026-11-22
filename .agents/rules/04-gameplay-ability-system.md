# 04. Gameplay Ability System (GAS) Standards

Reference implementation SSOT: `Source/FC/Public/AbilitySystem/FCAttributeSet.h` (`Health`, `MaxHealth`, `Mana`, `MaxMana`, `AttackPower`, `Shield`, `MaxShield`) and `Source/FC/Private/AbilitySystem/FCAttributeSet.cpp`.

## 1. AttributeSet Macro & Replication Hygiene (Enforced by `verify_gas.py`)
- **Accessor Macro**: Define `#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)` and declare `ATTRIBUTE_ACCESSORS(Class, Prop)` for **every** `FGameplayAttributeData` (both replicated attributes and meta-attributes).
- **Replicated Attributes**:
  - Mark with `ReplicatedUsing = OnRep_<PropName>` and declare `UFUNCTION() virtual void OnRep_<PropName>(const FGameplayAttributeData& Old<PropName>);`.
  - Register in `GetLifetimeReplicatedProps` using `DOREPLIFETIME_CONDITION_NOTIFY(Class, Prop, COND_None | COND_OwnerOnly, REPNOTIFY_Always);` (use `COND_OwnerOnly` for private resources such as `Mana` / `MaxMana`).
  - Wrap every `OnRep_*` body with `GAMEPLAYATTRIBUTE_REPNOTIFY(Class, Prop, OldProp);`.
- **Un-Replicated Meta-Attributes (Damage / Healing Pattern)**:
  - For server-only intermediate calculations (e.g., absorbing `Shield` before reducing `Health` without adding back subtracted health), declare an un-replicated meta-attribute using `NotReplicated` or `Category = "Attributes|Meta"` (or standard names `Damage`, `IncomingDamage`, `IncomingHealing`, `Healing`):
    ```cpp
    UPROPERTY(BlueprintReadOnly, NotReplicated, Category = "Attributes|Meta")
    FGameplayAttributeData Damage;
    ATTRIBUTE_ACCESSORS(UFCAttributeSet, Damage)
    ```
  - `verify_gas.py` enforces `ATTRIBUTE_ACCESSORS` on meta-attributes while exempting them from `ReplicatedUsing` / `OnRep_*` / `DOREPLIFETIME_CONDITION_NOTIFY`.
  - Consume and reset `SetDamage(0.f)` inside server-authoritative `PostGameplayEffectExecute`.

## 2. Clamping & NaN Sanitization
- **`PreAttributeChange`**: Must check `!FMath::IsFinite(NewValue)` (or `IsNaN`) first (`GAS_PREATTRIBUTE_MISSING_NAN_CHECK` error) and clamp current values to valid bounds (`[0, MaxHealth]`, `[0, MaxMana]`, `[0, MaxShield]`, `>= 0`).
- **`PostGameplayEffectExecute`**: Server-authoritative resolution (damage/shield absorption, clamping base values, death/hit notifications).

## 3. Gameplay Tag Taxonomy & SSOT
| Category | Pattern | Examples |
| :--- | :--- | :--- |
| **State** | `State.<Category>.<Name>` | `State.Debuff.Stun`, `State.Combat.Attacking` |
| **Ability** | `Ability.<Type>.<Name>` | `Ability.Skill.Fireball`, `Ability.Skill.SandWall` |
| **Data** | `Data.<Domain>.<Attribute>` | `Data.Damage.Fire`, `Data.Cooldown.Primary` |
| **Event** | `Event.Montage.<Name>` | `Event.Montage.HitImpact`, `Event.Montage.SpawnProjectile` |

- **SSOT & Conflict Prevention (Enforced by `verify_gas.py`)**:
  - **Global Core Tags** (`Element.*`, `Class.*`, `Card.Type.*`, `Combat.Team.*`, `Data.*` shared across systems): Declare once in `FCGameplayTags.h` / `FCGameplayTags.cpp`.
  - **Single-File Native Tags**: Tags used by a single `.cpp` file should use `UE_DEFINE_GAMEPLAY_TAG_STATIC(TagName, "...")` inside that `.cpp` instead of modifying `FCGameplayTags.h/.cpp` (prevents merge conflicts and wide header recompilation).
  - **Feature / Data-Only Tags**: Put non-C++ / editor / data-driven tags in feature-scoped files under `Config/Tags/<Feature>.ini` instead of accumulating them in `Config/DefaultGameplayTags.ini`.
  - **Zero Native ↔ INI Duplication** (`TAG_DUPLICATE_NATIVE_INI`): Never duplicate a C++ `UE_DEFINE_GAMEPLAY_TAG*` tag in `DefaultGameplayTags.ini` or `Config/Tags/*.ini`.
  - **No Raw String Lookups** (`TAG_RAW_REQUEST_CALL`): Never call `FGameplayTag::RequestGameplayTag(FName(...))` in C++; use Native Tag symbols (`FCGameplayTags::*` or `UE_DEFINE_GAMEPLAY_TAG_STATIC`).

## 4. Prediction, Gameplay Cues & Architectural Decoupling
- **Client Prediction & Security**: Trigger player abilities via `TryActivateAbility` with prediction keys; enforce server execution policies (`ServerInitiated` / `ServerOnly`) and validate `TargetData`.
- **Gameplay Cues Over Multicasts**: Dispatch transient hit/impact/audio VFX via `GameplayCue.*` (`ExecuteGameplayCue` or Gameplay Effects), never `NetMulticast` RPCs.
- **Zero UI Coupling** (`GAS_UI_HEADER_IN_ABILITY`): `UGameplayAbility` and `UAttributeSet` must never include Slate, UMG, or ViewModel headers; client presenters (`UFCStatusPresenterComponent`) bind via `GetGameplayAttributeValueChangeDelegate` + `AddWeakLambda`.
- **Concrete Actor Decoupling (OCP)**: New GAS classes and AttributeSet extensions must avoid coupling to concrete gameplay actor classes—notify via `AActor::TakeDamage`, interfaces, or delegates.
