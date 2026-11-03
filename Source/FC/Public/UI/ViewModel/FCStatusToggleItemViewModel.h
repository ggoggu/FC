#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "GameplayTagContainer.h"
#include "UI/Data/FCStatusToggleTypes.h"
#include "FCStatusToggleItemViewModel.generated.h"

/**
 * UFCStatusToggleItemViewModel
 * 
 * Presentation ViewModel for a single circular status toggle indicator.
 * Dispatches FieldNotify events for zero-tick UI data binding.
 */
UCLASS(BlueprintType)
class FC_API UFCStatusToggleItemViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	// --- Identity ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Status")
	FGameplayTag StatusTag;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Status")
	FName StatusId = NAME_None;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Status")
	FText DisplayName;

	// --- Visuals ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Status")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Status")
	FLinearColor BorderColor = FLinearColor(0.0f, 0.8f, 1.0f, 1.0f);

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Status")
	bool bIsActive = true;

	// --- Stack & Duration ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Status")
	int32 StackCount = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Status")
	float RemainingDuration = 0.0f;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Status")
	float TotalDuration = 0.0f;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Status")
	float DurationRatio = 0.0f;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Status")
	FText TooltipText;

public:
	/** Initializes or synchronizes from data structure */
	void InitializeFromData(const FFCStatusToggleData& InData);

	// Property Setters utilizing UE_MVVM_SET_PROPERTY_VALUE
	void SetStatusTag(const FGameplayTag& InTag) { UE_MVVM_SET_PROPERTY_VALUE(StatusTag, InTag); }
	void SetStatusId(const FName& InId) { UE_MVVM_SET_PROPERTY_VALUE(StatusId, InId); }
	void SetDisplayName(const FText& InName) { UE_MVVM_SET_PROPERTY_VALUE(DisplayName, InName); }
	void SetIcon(const TSoftObjectPtr<UTexture2D>& InIcon) { UE_MVVM_SET_PROPERTY_VALUE(Icon, InIcon); }
	void SetBorderColor(const FLinearColor& InColor) { UE_MVVM_SET_PROPERTY_VALUE(BorderColor, InColor); }
	void SetIsActive(bool bInActive) { UE_MVVM_SET_PROPERTY_VALUE(bIsActive, bInActive); }
	void SetStackCount(int32 InCount);
	void SetDuration(float InRemaining, float InTotal);
	void SetTooltipText(const FText& InTooltip) { UE_MVVM_SET_PROPERTY_VALUE(TooltipText, InTooltip); }

	// --- Computed FieldNotify Getters ---
	UFUNCTION(BlueprintPure, FieldNotify)
	bool HasStackCount() const;

	UFUNCTION(BlueprintPure, FieldNotify)
	bool HasDuration() const;

	UFUNCTION(BlueprintPure, FieldNotify)
	FText GetStackDisplayText() const;

	UFUNCTION(BlueprintPure, FieldNotify)
	float GetDurationPercent() const;
};
