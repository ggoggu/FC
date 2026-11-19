#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FCStatusToggleItemWidget.generated.h"

class UFCStatusToggleItemViewModel;
class UImage;
class UProgressBar;
class UTextBlock;

/**
 * UFCStatusToggleItemWidget
 * 
 * Base UMG widget for an individual circular status toggle icon.
 * Binds to UFCStatusToggleItemViewModel and reflects active state, stack, and cooldown.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class FC_API UFCStatusToggleItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFCStatusToggleItemWidget(const FObjectInitializer& ObjectInitializer);

	/** Assigns the presentation ViewModel to this status item widget */
	UFUNCTION(BlueprintCallable, Category = "Status|ViewModel")
	virtual void SetItemViewModel(UFCStatusToggleItemViewModel* InViewModel);

	/** Gets the active presentation ViewModel */
	UFUNCTION(BlueprintPure, Category = "Status|ViewModel")
	UFCStatusToggleItemViewModel* GetItemViewModel() const { return ItemViewModel; }

	/** Synchronizes visual components from the active ItemViewModel */
	UFUNCTION(BlueprintCallable, Category = "Status|View")
	virtual void UpdateVisuals();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Blueprint hook triggered whenever a new ViewModel is bound */
	UFUNCTION(BlueprintImplementableEvent, Category = "Status|Events")
	void OnItemViewModelAssigned(UFCStatusToggleItemViewModel* NewViewModel);

	UPROPERTY(BlueprintReadOnly, Category = "Status|ViewModel")
	TObjectPtr<UFCStatusToggleItemViewModel> ItemViewModel;

	/** Optional status icon image */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Status|View")
	TObjectPtr<UImage> IconImage;

	/** Optional circular ring / border glow image */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Status|View")
	TObjectPtr<UImage> BorderImage;

	/** Optional cooldown progress bar or duration gauge */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Status|View")
	TObjectPtr<UProgressBar> DurationBar;

	/** Optional stack count text */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Status|View")
	TObjectPtr<UTextBlock> StackText;
};
