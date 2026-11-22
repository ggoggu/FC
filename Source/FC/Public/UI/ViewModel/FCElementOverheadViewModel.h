#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "GameplayTagContainer.h"
#include "Data/Class/FCClassTypes.h"
#include "Components/SlateWrapperTypes.h"
#include "FCElementOverheadViewModel.generated.h"

class UFCElementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnElementOverheadStacksUpdatedSignature, int32, TotalStacks);

/**
 * UFCElementOverheadViewModel
 * 
 * Presentation ViewModel managing the elemental stack display above an enemy's head.
 * Uses open-ended FGameplayTag storage and map counts (zero hardcoded per-element properties).
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

	/** Ordered collection of active element GameplayTag stacks (Zero UObject allocation) */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element")
	TArray<FGameplayTag> CurrentStacks;

	/** Dynamic breakdown map of element tag -> stack count (open-ended for any DT_ElementCatalog tag) */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element|Counts")
	TMap<FGameplayTag, int32> ElementCounts;

	UFUNCTION(BlueprintPure, Category = "FC|Element")
	const TArray<FGameplayTag>& GetCurrentStacks() const { return CurrentStacks; }

	UFUNCTION(BlueprintPure, Category = "FC|Element")
	FGameplayTag GetStackAt(int32 Index) const { return CurrentStacks.IsValidIndex(Index) ? CurrentStacks[Index] : FGameplayTag::EmptyTag; }

	UFUNCTION(BlueprintPure, Category = "FC|Element|Counts")
	int32 GetElementCount(FGameplayTag ElementTag) const;

public:
	// --- Component Binding & Lifecycle ---
	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	void BindToElementComponent(UFCElementComponent* InComponent);

	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	void UnbindFromElementComponent();

	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	void RefreshFromComponent();

	/** Applies an array of element GameplayTag stacks directly to the ViewModel */
	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	void UpdateFromStacks(const TArray<FGameplayTag>& InStacks);

	// --- Setters utilizing UE_MVVM_SET_PROPERTY_VALUE ---
	void SetTotalStacks(int32 InTotal);
	void SetMaxStacks(int32 InMax);
	void SetHasAnyStack(bool bInHasAny) { UE_MVVM_SET_PROPERTY_VALUE(bHasAnyStack, bInHasAny); }

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
	void HandleStacksChanged(const TArray<FGameplayTag>& InStacks);

	UPROPERTY(Transient)
	TWeakObjectPtr<UFCElementComponent> BoundElementComponent;
};
