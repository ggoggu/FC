#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "InputCoreTypes.h"
#include "FCCardRewardViewModel.generated.h"

class UFCCardViewModel;
class UFCCardSubsystem;

/**
 * UFCCardRewardViewModel
 * 
 * Presentation ViewModel for the Card Reward / Pickup Selection Modal.
 * Manages the card presentation sub-viewmodel, prompt texts, configurable shortcut hints,
 * and visibility state for zero-tick UMG MVVM bindings.
 */
UCLASS(BlueprintType)
class FC_API UFCCardRewardViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UFCCardRewardViewModel();

	// --- Sub-ViewModel & Presentation State ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Reward")
	TObjectPtr<UFCCardViewModel> CardViewModel;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Reward")
	FText PromptTitle;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Reward")
	FText PromptDescription;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Reward")
	FText AcquireActionText;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Reward")
	FText DiscardActionText;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Reward")
	bool bIsVisible = false;

public:
	// Property Setters utilizing UE_MVVM_SET_PROPERTY_VALUE
	void SetCardViewModel(UFCCardViewModel* InVM);
	void SetPromptTitle(const FText& InTitle) { UE_MVVM_SET_PROPERTY_VALUE(PromptTitle, InTitle); }
	void SetPromptDescription(const FText& InDesc) { UE_MVVM_SET_PROPERTY_VALUE(PromptDescription, InDesc); }
	void SetAcquireActionText(const FText& InText) { UE_MVVM_SET_PROPERTY_VALUE(AcquireActionText, InText); }
	void SetDiscardActionText(const FText& InText) { UE_MVVM_SET_PROPERTY_VALUE(DiscardActionText, InText); }
	void SetIsVisible(bool bInVisible) { UE_MVVM_SET_PROPERTY_VALUE(bIsVisible, bInVisible); }

	/** Initializes the reward modal with the selected card data and shortcut key hints */
	UFUNCTION(BlueprintCallable, Category = "FC|Reward")
	void SetupReward(FName InCardId, UFCCardSubsystem* Subsystem, const FKey& InAcquireKey, const FKey& InDiscardKey);

	/** Formats shortcut key string for UI display (e.g. "[Space] 획득", "[ESC] 버리기") */
	static FText FormatActionShortcut(const FKey& InKey, const FText& ActionName);
};
