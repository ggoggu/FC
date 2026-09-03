#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FCCardWidget.generated.h"

class UFCCardViewModel;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardWidgetClickedSignature, UFCCardWidget*, ClickedWidget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardWidgetHoveredSignature, UFCCardWidget*, HoveredWidget, bool, bIsHovered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardWidgetDragStartedSignature, UFCCardWidget*, DraggedWidget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardWidgetDraggedSignature, UFCCardWidget*, DraggedWidget, const FVector2D&, DragTranslation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardWidgetDragEndedSignature, UFCCardWidget*, DraggedWidget, bool, bWasDragged);

/**
 * UFCCardWidget
 * 
 * UMG UserWidget representing an individual card view.
 * Bound to UFCCardViewModel via UE5 MVVM plugin or C++ event forwarding.
 * Supports mouse hover zoom, click selection, and interactive drag-and-move.
 */
UCLASS(BlueprintType, Blueprintable)
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

	UPROPERTY(BlueprintAssignable, Category = "Card|Interaction")
	FOnCardWidgetDragStartedSignature OnCardDragStarted;

	UPROPERTY(BlueprintAssignable, Category = "Card|Interaction")
	FOnCardWidgetDraggedSignature OnCardDragged;

	UPROPERTY(BlueprintAssignable, Category = "Card|Interaction")
	FOnCardWidgetDragEndedSignature OnCardDragEnded;

	// --- Layout & Transform Management ---
	/** Sets the target layout transform (translation, angle, scale, pivot, z-order) */
	UFUNCTION(BlueprintCallable, Category = "Card|Layout")
	virtual void SetCardTargetTransform(
		const FVector2D& InTranslation,
		float InAngle,
		const FVector2D& InScale,
		const FVector2D& InPivot,
		int32 InZOrder,
		bool bImmediate = false);

	/** Smoothly interpolates current render transform towards target transform */
	UFUNCTION(BlueprintCallable, Category = "Card|Layout")
	virtual void UpdateCardTransform(float DeltaTime, float InterpSpeed);

	/** Immediately applies the current render transform and slot z-order */
	UFUNCTION(BlueprintCallable, Category = "Card|Layout")
	virtual void ApplyCurrentTransform();

	UFUNCTION(BlueprintPure, Category = "Card|Layout")
	FVector2D GetCurrentTranslation() const { return CurrentTranslation; }

	UFUNCTION(BlueprintPure, Category = "Card|Layout")
	float GetCurrentAngle() const { return CurrentAngle; }

	UFUNCTION(BlueprintPure, Category = "Card|Layout")
	FVector2D GetCurrentScale() const { return CurrentScale; }

	UFUNCTION(BlueprintPure, Category = "Card|Layout")
	FVector2D GetTargetTranslation() const { return TargetTranslation; }

	UFUNCTION(BlueprintPure, Category = "Card|Layout")
	float GetTargetAngle() const { return TargetAngle; }

	UFUNCTION(BlueprintPure, Category = "Card|Layout")
	FVector2D GetTargetScale() const { return TargetScale; }

	UFUNCTION(BlueprintPure, Category = "Card|Layout")
	int32 GetTargetZOrder() const { return TargetZOrder; }

	UFUNCTION(BlueprintPure, Category = "Card|Interaction")
	bool IsDragging() const { return bIsDragging; }

	UFUNCTION(BlueprintPure, Category = "Card|Interaction")
	bool IsHeldByHotKey() const { return bIsHeldByHotKey; }

	UFUNCTION(BlueprintCallable, Category = "Card|Interaction")
	void SetIsHeldByHotKey(bool bInHeld);

	UFUNCTION(BlueprintPure, Category = "Card|Interaction")
	FVector2D GetDragStartScreenPosition() const { return DragStartScreenPos; }

	UFUNCTION(BlueprintPure, Category = "Card|Interaction")
	FVector2D GetDragStartCardTranslation() const { return DragStartCardTranslation; }

	// --- Description Text Wrapping & Layout Protection ---
	/** Updates the description text wrapping settings */
	UFUNCTION(BlueprintCallable, Category = "Card|Layout")
	void ApplyDescriptionWrapping();

	/** Gets the description text block widget (if bound in Blueprint) */
	UFUNCTION(BlueprintPure, Category = "Card|View")
	UTextBlock* GetDescriptionTextBlock() const { return Text_Description; }

	/** Sets whether the description text should automatically wrap to the next line */
	UFUNCTION(BlueprintCallable, Category = "Card|Layout")
	void SetAutoWrapDescription(bool bInAutoWrap);

	/** Sets the wrapping threshold width for the description text (in Slate units/pixels) */
	UFUNCTION(BlueprintCallable, Category = "Card|Layout")
	void SetDescriptionWrapWidth(float InWrapWidth);

	UFUNCTION(BlueprintPure, Category = "Card|Layout")
	bool GetAutoWrapDescription() const { return bAutoWrapDescription; }

	/** Gets wrapping threshold in pixels. Returns DescriptionWrapWidth if > 0, otherwise reads from Text_Description widget */
	UFUNCTION(BlueprintPure, Category = "Card|Layout")
	float GetDescriptionWrapWidth() const;

protected:
	virtual void NativeConstruct() override;
	virtual void SynchronizeProperties() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

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

	/** Card description text block. Automatically bound if named Text_Description in Blueprint */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Card|View")
	TObjectPtr<UTextBlock> Text_Description;

	/** Whether to automatically wrap card description text across multiple lines to prevent horizontal stretching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Layout")
	bool bAutoWrapDescription = true;

	/** Text wrapping threshold in pixels for card description.
	 * Default is 0.0f, which respects whatever 'Wrap Text At' value is set directly on Text_Description in the UMG editor (e.g. 200).
	 * If set > 0.0f, it programmatically overrides Text_Description's wrap width with this value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Layout")
	float DescriptionWrapWidth = 0.0f;

	// --- Internal Transform Tracking ---
	UPROPERTY(BlueprintReadOnly, Category = "Card|Layout")
	FVector2D CurrentTranslation = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Card|Layout")
	float CurrentAngle = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Card|Layout")
	FVector2D CurrentScale = FVector2D(1.0f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Card|Layout")
	FVector2D TargetTranslation = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Card|Layout")
	float TargetAngle = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Card|Layout")
	FVector2D TargetScale = FVector2D(1.0f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Card|Layout")
	FVector2D TargetPivot = FVector2D(0.5f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Card|Layout")
	int32 TargetZOrder = 0;

	// --- Internal Drag & Hold State Tracking ---
	UPROPERTY(BlueprintReadOnly, Category = "Card|Interaction")
	bool bIsDragging = false;

	UPROPERTY(BlueprintReadOnly, Category = "Card|Interaction")
	bool bIsHeldByHotKey = false;

	UPROPERTY(BlueprintReadOnly, Category = "Card|Interaction")
	bool bHasMovedDuringDrag = false;

	UPROPERTY(BlueprintReadOnly, Category = "Card|Interaction")
	FVector2D DragStartScreenPos = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Card|Interaction")
	FVector2D DragStartCardTranslation = FVector2D::ZeroVector;
};
