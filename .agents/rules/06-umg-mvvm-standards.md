# 06. UMG Model-View-ViewModel (MVVM) Standards

Reference SSOT implementations: `Source/FC/Public/UI/ViewModel/FCHUDViewModel.h`, `Source/FC/Public/UI/ViewModel/FCStatusToggleContainerViewModel.h`, `Source/FC/Public/UI/Presenter/FCStatusPresenterComponent.h`.

## 1. Core Principles & Tier Separation
- **Client-Only Execution**: `UMVVMViewModelBase` exists exclusively on client instances. Never mark ViewModel properties `Replicated` (`MVVM_REPLICATED_FORBIDDEN` error) and never include `GameMode` headers in UI/ViewModel files (`UI_GAMEMODE_DEPENDENCY` error).
- **Zero-Tick UI Binding**: Eliminate UMG `Tick` polling bindings. Widgets observe ViewModel properties strictly through `FieldNotify`.
- **Soft Asset References**: Texture/icon fields on ViewModels must use `TSoftObjectPtr<UTexture2D>`, never hard `UTexture2D*` or `TObjectPtr<UTexture2D>` (`MVVM_HARD_TEXTURE_REF` error).

## 2. ViewModel Declaration & Notification Macros
- Inherit from `UMVVMViewModelBase` (`#include "MVVMViewModelBase.h"`) with `UCLASS(BlueprintType)`.
- Annotate bound state with `UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|...")` and computed getters with `UFUNCTION(BlueprintPure, FieldNotify)`.
- **Scalar Setters**: Always mutate through `UE_MVVM_SET_PROPERTY_VALUE(Prop, NewValue)` (auto-dedupes equal assignments). If `true`, call `UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DependentGetter)` for any computed `FieldNotify` getters.
- **Collection Setters**: After mutating `TArray<TObjectPtr<UItemVM>>`, explicitly call `UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CollectionProp)`.

## 3. Sub-ViewModel Lifecycle & Selection
- **Outer Ownership**: Instantiate child item ViewModels with `NewObject<UChildVM>(this)` so GC ties their lifetime cleanly to the parent ViewModel.
- **Selection State**: Manage single/multi-select (`bIsSelected`) via explicit ViewModel setters (`SetIsSelected`), never inside Widget Blueprint event graphs.

## 4. Unidirectional Control Flow & Input Consolidation
```
Server Model (Replicated FastArray / GAS / PlayerState)
  -> Client Controller / Presenter (OnRep / ASC AddWeakLambda)
    -> Client ViewModel (UE_MVVM_SET_PROPERTY_VALUE / FieldNotify)
      -> Widget Blueprint View (Passive display + surface clicks/drags only)
```
- **Passive Views**: `UUserWidget` classes must never own gameplay state (held cards, target resolution) or invoke Server RPCs directly—route intents through `APlayerController` / `ViewModel`.
- **Input Consolidation**: Never duplicate gameplay key/mouse listeners inside widgets (`NativeOnKeyDown`, `NativeOnMouseButtonDown`) to mirror PlayerController actions. Route gameplay inputs through Enhanced Input on `APlayerController`.
