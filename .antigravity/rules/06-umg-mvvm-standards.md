# UMG Model-View-ViewModel (MVVM) Standards & Architecture

This document defines the architectural standards, property notification conventions, and sub-viewmodel collection patterns for UMG MVVM implementation in Unreal Engine 5.8.

---

## 1. Core Principles & Tier Separation

MVVM enforces a strict separation between **Gameplay Models (Model)**, **UI Presentation Logic (ViewModel)**, and **Widget Blueprints (View)**:

1. **Client-Only Execution**:
   - `UMVVMViewModelBase` classes exist solely in the Presentation Tier on client instances (`Standalone`, `Listen Server Client`, `Autonomous Client`, `Simulated Client`).
   - ViewModels must **never** be replicated directly (`Replicated` macros are forbidden on ViewModels). They are populated via client-side delegates, Controller/HUD initializers, or GAS Attribute callbacks.
2. **Zero-Tick UI Binding**:
   - Eliminate legacy UMG `Tick`-based property bindings.
   - UI widgets observe ViewModel changes strictly via `FieldNotify` event broadcasts.
3. **Asset Soft Referencing**:
   - Texture and icon references in ViewModels must use `TSoftObjectPtr<UTexture2D>` to prevent memory bloat and synchronous blocking loads.

---

## 2. ViewModel Declaration & FieldNotify Conventions

### 1. Header Structure
All ViewModels must inherit from `UMVVMViewModelBase` (`#include "MVVMViewModelBase.h"`) and be decorated with `UCLASS(BlueprintType)`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "FCPlayerHUDViewModel.generated.h"

class UFCItemViewModel;

UCLASS(BlueprintType)
class FC_API UFCPlayerHUDViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    // --- System & State ---
    UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|HUD|System")
    FText TimerText;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|HUD|Player")
    int32 CurrentHealth = 100;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|HUD|Player")
    int32 MaxHealth = 100;

    // --- Sub-ViewModel Collections ---
    UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|HUD|Inventory")
    TArray<TObjectPtr<UFCItemViewModel>> ItemList;

public:
    // Setters utilizing UE_MVVM_SET_PROPERTY_VALUE
    void SetTimerText(const FText& InText) { UE_MVVM_SET_PROPERTY_VALUE(TimerText, InText); }
    void SetCurrentHealth(int32 InHealth);
    void SetMaxHealth(int32 InMaxHealth);

    // Collection Updates
    void UpdateItemList(const TArray<TObjectPtr<UFCItemViewModel>>& InItems);

    // Computed FieldNotify (Getter function depending on other properties)
    UFUNCTION(BlueprintPure, FieldNotify)
    float GetHealthPercent() const;
};
```

---

## 3. Value Mutation & Notification Macros

### 1. `UE_MVVM_SET_PROPERTY_VALUE`
- Always use `UE_MVVM_SET_PROPERTY_VALUE(PropertyName, NewValue)` inside property Setters.
- It automatically checks for value inequality before assignment and triggers the corresponding `FieldNotify` only when the value actually changes.

```cpp
void UFCPlayerHUDViewModel::SetCurrentHealth(int32 InHealth)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(CurrentHealth, InHealth))
    {
        // Broadcast dependent computed property updates
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
    }
}
```

### 2. `UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED`
- Use `UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(FieldOrGetter)` explicitly when:
  1. An entire `TArray` collection is modified, cleared, or populated.
  2. A computed getter function (`UFUNCTION(BlueprintPure, FieldNotify)`) needs to notify the UI because one of its input dependencies changed.

```cpp
void UFCPlayerHUDViewModel::UpdateItemList(const TArray<TObjectPtr<UFCItemViewModel>>& InItems)
{
    ItemList = InItems;
    // Broadcast whole array change to trigger list/grid widget refresh
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ItemList);
}

float UFCPlayerHUDViewModel::GetHealthPercent() const
{
    return (MaxHealth > 0) ? static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth) : 0.0f;
}
```

---

## 4. Sub-ViewModel & Item Collection Patterns

When designing lists, grids, inventories, or selectable items (e.g., Cards, Artifacts, Skills):

### 1. Item ViewModel Lifecycle (`NewObject` with Outer)
Always pass the parent ViewModel as the `Outer` (`this`) when instantiating child ViewModels so their lifecycle is garbage-collected cleanly:

```cpp
void UFCArtifactSelectionViewModel::SetupAvailableArtifacts(const TArray<FFCArtifactData>& InArtifacts)
{
    AvailableArtifacts.Empty();
    for (const FFCArtifactData& ArtifactData : InArtifacts)
    {
        UFCArtifactItemViewModel* NewItemVM = NewObject<UFCArtifactItemViewModel>(this);
        if (NewItemVM)
        {
            NewItemVM->InitializeFromData(ArtifactData);
            AvailableArtifacts.Add(NewItemVM);
        }
    }
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(AvailableArtifacts);
}
```

### 2. Selection Management (Single / Multi Select)
Selection states (`bIsSelected`) must be handled by the parent ViewModel or Item ViewModel via explicit Setters:

```cpp
void UFCArtifactSelectionViewModel::SelectArtifact(UFCArtifactItemViewModel* SelectedVM)
{
    if (!SelectedVM) return;

    for (UFCArtifactItemViewModel* ItemVM : AvailableArtifacts)
    {
        if (ItemVM)
        {
            ItemVM->SetIsSelected(ItemVM == SelectedVM);
        }
    }
}

FName UFCArtifactSelectionViewModel::GetSelectedArtifactID() const
{
    for (const UFCArtifactItemViewModel* ItemVM : AvailableArtifacts)
    {
        if (ItemVM && ItemVM->GetIsSelected())
        {
            return ItemVM->GetArtifactID();
        }
    }
    return NAME_None;
}
```

---

## 5. UI Asset Management (`TSoftObjectPtr`)

For icon images, portrait textures, and dynamic UI meshes, declare soft pointers to prevent hard-referencing large textures in memory:

```cpp
UCLASS(BlueprintType)
class FC_API UFCArtifactItemViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Artifact")
    FName ArtifactID;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Artifact")
    FText ArtifactName;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Artifact")
    FText EffectText;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Artifact")
    TSoftObjectPtr<UTexture2D> ArtifactIcon;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Artifact")
    bool bIsSelected = false;

public:
    void SetArtifactID(FName InID) { UE_MVVM_SET_PROPERTY_VALUE(ArtifactID, InID); }
    void SetArtifactName(const FText& InName) { UE_MVVM_SET_PROPERTY_VALUE(ArtifactName, InName); }
    void SetEffectText(const FText& InText) { UE_MVVM_SET_PROPERTY_VALUE(EffectText, InText); }
    void SetArtifactIcon(TSoftObjectPtr<UTexture2D> InIcon) { UE_MVVM_SET_PROPERTY_VALUE(ArtifactIcon, InIcon); }
    void SetIsSelected(bool bInSelected) { UE_MVVM_SET_PROPERTY_VALUE(bIsSelected, bInSelected); }

    bool GetIsSelected() const { return bIsSelected; }
    FName GetArtifactID() const { return ArtifactID; }
};
```

---

## 6. Integration with Gameplay & Networking

```
+-----------------------------------------------------------------------------------+
|                        AUTHORITATIVE MODEL (SERVER)                               |
|  - Replicated FastArray, Gameplay Attributes, PlayerState                         |
+----------------------------------------+------------------------------------------+
                                         | Replicated State / OnRep
                                         v
+-----------------------------------------------------------------------------------+
|                     PRESENTATION CONTROLLER / HUD (CLIENT)                        |
|  - Listens to OnRep / AttributeChange delegates                                   |
|  - Pushes updated data into local ViewModel instance                              |
+----------------------------------------+------------------------------------------+
                                         | SetProperty / BroadcastFieldChange
                                         v
+-----------------------------------------------------------------------------------+
|                            VIEWMODEL (CLIENT ONLY)                                |
|  - Holds formatted FText, clamped values, soft asset pointers                     |
|  - Dispatches FieldNotify events to View Bindings                                 |
+----------------------------------------+------------------------------------------+
                                         | Automatic MVVM Binding
                                         v
+-----------------------------------------------------------------------------------+
|                         WIDGET BLUEPRINT (VIEW / UMG)                             |
|  - One Way To Target / Two Way bindings                                           |
|  - Zero Tick Overhead                                                             |
+-----------------------------------------------------------------------------------+
```
