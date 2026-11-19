#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Class/FCClassTypes.h"
#include "FCClassSelectItemWidget.generated.h"

class UFCClassSelectItemViewModel;
class UButton;
class UTextBlock;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClassItemClickedSignature, EFCCharacterClass, ClassType);

/**
 * UFCClassSelectItemWidget
 * 
 * Base C++ widget class for an individual class selection card (WBP_ClassSelectItem).
 * Provides BindWidgetOptional bindings and BlueprintNativeEvents for flexible visual customizability.
 */
UCLASS(Abstract, BlueprintType)
class FC_API UFCClassSelectItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Binds this widget to its presentation ViewModel and hooks into property events */
	UFUNCTION(BlueprintCallable, Category = "FC|ClassSelect")
	void InitializeWidget(UFCClassSelectItemViewModel* InViewModel);

	/** Returns the bound ViewModel */
	UFUNCTION(BlueprintPure, Category = "FC|ClassSelect")
	UFCClassSelectItemViewModel* GetViewModel() const { return ViewModel; }

	/** Delegate fired when user clicks the card */
	UPROPERTY(BlueprintAssignable, Category = "FC|ClassSelect|Events")
	FOnClassItemClickedSignature OnItemClicked;

	// Visual hooks for Blueprint styling & animation
	UFUNCTION(BlueprintImplementableEvent, Category = "FC|ClassSelect|Visual")
	void BP_OnSelectionChanged(bool bIsSelected);

	UFUNCTION(BlueprintImplementableEvent, Category = "FC|ClassSelect|Visual")
	void BP_OnSelectableChanged(bool bIsSelectable);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "FC|ClassSelect")
	TObjectPtr<UFCClassSelectItemViewModel> ViewModel;

	// --- Optional UMG Bindings for easy Designer Styling ---
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|ClassSelect|Bindings")
	TObjectPtr<UButton> Btn_Select;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|ClassSelect|Bindings")
	TObjectPtr<UTextBlock> Text_ClassName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|ClassSelect|Bindings")
	TObjectPtr<UImage> Img_ClassIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|ClassSelect|Bindings")
	TObjectPtr<UWidget> SelectionOverlay;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|ClassSelect|Bindings")
	TObjectPtr<UWidget> LockOverlay;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|ClassSelect|Bindings")
	TObjectPtr<UTextBlock> Text_LockReason;

	UFUNCTION()
	void HandleSelectButtonClicked();

	void UpdateVisualState();
};
