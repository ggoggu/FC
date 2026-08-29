#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FCCardWidget.generated.h"

class UFCCardViewModel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardWidgetClickedSignature, UFCCardWidget*, ClickedWidget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardWidgetHoveredSignature, UFCCardWidget*, HoveredWidget, bool, bIsHovered);

/**
 * UFCCardWidget
 * 
 * UMG UserWidget representing an individual card view.
 * Bound to UFCCardViewModel via UE5 MVVM plugin or C++ event forwarding.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class FC_API UFCCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFCCardWidget(const FObjectInitializer& ObjectInitializer);

	/** Assigns the card presentation ViewModel to this widget */
	UFUNCTION(BlueprintCallable, Category = "Card|ViewModel")
	virtual void SetCardViewModel(UFCCardViewModel* InViewModel);

	/** Gets the active card presentation ViewModel */
	UFUNCTION(BlueprintPure, Category = "Card|ViewModel")
	UFCCardViewModel* GetCardViewModel() const { return CardViewModel; }

	// --- View Interaction Delegates ---
	UPROPERTY(BlueprintAssignable, Category = "Card|Interaction")
	FOnCardWidgetClickedSignature OnCardClicked;

	UPROPERTY(BlueprintAssignable, Category = "Card|Interaction")
	FOnCardWidgetHoveredSignature OnCardHovered;

protected:
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Blueprint hook triggered whenever a new ViewModel is bound */
	UFUNCTION(BlueprintImplementableEvent, Category = "Card|Events")
	void OnCardViewModelAssigned(UFCCardViewModel* NewViewModel);

	/** Blueprint hook for visual hover animations (e.g. slight raise/scale) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Card|Events")
	void OnCardHoverStateChanged(bool bIsHovered);

	/** Blueprint hook for visual selection animations (e.g. golden glow) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Card|Events")
	void OnCardSelectionStateChanged(bool bIsSelected);

	UPROPERTY(BlueprintReadOnly, Category = "Card|ViewModel")
	TObjectPtr<UFCCardViewModel> CardViewModel;
};
