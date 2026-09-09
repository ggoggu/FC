#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Data/Class/FCClassTypes.h"
#include "Components/SlateWrapperTypes.h"
#include "UI/ViewModel/FCElementStackItemViewModel.h"
#include "FCElementOverheadViewModel.generated.h"

class UFCElementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnElementOverheadStacksUpdatedSignature, int32, TotalStacks);

/**
 * UFCElementOverheadViewModel
 * 
 * Presentation ViewModel managing the elemental stack display above an enemy's head.
 * Observes UFCElementComponent on client instances and drives UMG via zero-tick FieldNotify bindings.
 */
UCLASS(BlueprintType)
class FC_API UFCElementOverheadViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** Broadcast whenever stacks are refreshed */
	UPROPERTY(BlueprintAssignable, Category = "FC|Element")
	FOnElementOverheadStacksUpdatedSignature OnStacksUpdated;

	// --- Stack State ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element")
	int32 TotalStacks = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element")
	int32 MaxStacks = 7;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element")
	bool bHasAnyStack = false;

	/** Ordered collection of active element stack tokens */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element")
	TArray<TObjectPtr<UFCElementStackItemViewModel>> StackList;

	// --- Element Count Breakdowns ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element|Counts")
	int32 FireCount = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element|Counts")
	int32 EarthCount = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element|Counts")
	int32 WaterCount = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element|Counts")
	int32 WindCount = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element|Counts")
	int32 LightningCount = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element|Counts")
	int32 HolyCount = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element|Counts")
	int32 DarkCount = 0;

public:
	// --- Component Binding & Lifecycle ---
	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	void BindToElementComponent(UFCElementComponent* InComponent);

	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	void UnbindFromElementComponent();

	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	void RefreshFromComponent();

	/** Applies an array of element stacks directly to the ViewModel */
	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	void UpdateFromStacks(const TArray<EFCElement>& CurrentStacks);

	// --- Setters utilizing UE_MVVM_SET_PROPERTY_VALUE ---
	void SetTotalStacks(int32 InTotal);
	void SetMaxStacks(int32 InMax);
	void SetHasAnyStack(bool bInHasAny) { UE_MVVM_SET_PROPERTY_VALUE(bHasAnyStack, bInHasAny); }

	void SetFireCount(int32 InCount) { UE_MVVM_SET_PROPERTY_VALUE(FireCount, InCount); }
	void SetEarthCount(int32 InCount) { UE_MVVM_SET_PROPERTY_VALUE(EarthCount, InCount); }
	void SetWaterCount(int32 InCount) { UE_MVVM_SET_PROPERTY_VALUE(WaterCount, InCount); }
	void SetWindCount(int32 InCount) { UE_MVVM_SET_PROPERTY_VALUE(WindCount, InCount); }
	void SetLightningCount(int32 InCount) { UE_MVVM_SET_PROPERTY_VALUE(LightningCount, InCount); }
	void SetHolyCount(int32 InCount) { UE_MVVM_SET_PROPERTY_VALUE(HolyCount, InCount); }
	void SetDarkCount(int32 InCount) { UE_MVVM_SET_PROPERTY_VALUE(DarkCount, InCount); }

	// --- Computed FieldNotify Getters ---
	UFUNCTION(BlueprintPure, FieldNotify)
	float GetStackFillPercent() const;

	UFUNCTION(BlueprintPure, FieldNotify)
	FText GetStackSummaryText() const;

	UFUNCTION(BlueprintPure, FieldNotify)
	ESlateVisibility GetVisibilityBasedOnStacks() const;

	UFUNCTION(BlueprintPure, Category = "FC|Element")
	UFCElementComponent* GetBoundElementComponent() const { return BoundElementComponent.Get(); }

protected:
	UFUNCTION()
	void HandleStacksChanged(const TArray<EFCElement>& CurrentStacks);

	UPROPERTY(Transient)
	TWeakObjectPtr<UFCElementComponent> BoundElementComponent;
};
