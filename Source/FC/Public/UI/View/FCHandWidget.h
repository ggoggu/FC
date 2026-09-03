#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FCHandWidget.generated.h"

class UFCHandViewModel;
class UFCCardWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHandCardSelectedSignature, UFCCardWidget*, SelectedCardWidget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHandCardDragStartedSignature, UFCCardWidget*, DraggedCardWidget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHandCardDraggedSignature, UFCCardWidget*, DraggedCardWidget, const FVector2D&, DragTranslation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHandCardDragEndedSignature, UFCCardWidget*, DraggedCardWidget, bool, bWasDragged);

/**
 * UFCCardWidget
 * 
 * UMG UserWidget managing the player's card hand container,
 * fan layout organization, card slot generation, selection, and mouse drag movement.
 */
UCLASS(BlueprintType, Blueprintable)
class FC_API UFCHandWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFCHandWidget(const FObjectInitializer& ObjectInitializer);

	/** Assigns the hand presentation ViewModel to this widget */
	UFUNCTION(BlueprintCallable, Category = "Hand|ViewModel")
	virtual void SetHandViewModel(UFCHandViewModel* InViewModel);

	/** Gets the active hand presentation ViewModel */
	UFUNCTION(BlueprintPure, Category = "Hand|ViewModel")
	UFCHandViewModel* GetHandViewModel() const { return HandViewModel; }

	/** Callback when a child card widget is clicked */
	UFUNCTION(BlueprintCallable, Category = "Hand|Interaction")
	virtual void HandleCardClicked(UFCCardWidget* ClickedCardWidget);

	/** Callback when a child card widget hover state changes */
	UFUNCTION(BlueprintCallable, Category = "Hand|Interaction")
	virtual void HandleCardHovered(UFCCardWidget* HoveredCardWidget, bool bIsHovered);

	/** Callback when a child card widget begins drag movement */
	UFUNCTION(BlueprintCallable, Category = "Hand|Interaction")
	virtual void HandleCardDragStarted(UFCCardWidget* DraggedCard);

	/** Callback when a child card widget is being dragged */
	UFUNCTION(BlueprintCallable, Category = "Hand|Interaction")
	virtual void HandleCardDragged(UFCCardWidget* DraggedCard, const FVector2D& DragTranslation);

	/** Callback when a child card widget ends drag movement */
	UFUNCTION(BlueprintCallable, Category = "Hand|Interaction")
	virtual void HandleCardDragEnded(UFCCardWidget* DraggedCard, bool bWasDragged);

	/** Helper to instantiate and populate child card widgets inside a UMG panel (e.g. HorizontalBox or Overlay) */
	UFUNCTION(BlueprintCallable, Category = "Hand|View")
	void RefreshCardWidgets(class UPanelWidget* TargetPanel);

	/** Triggers card activation/play, moving the card from hand to discard pile (묘지) */
	UFUNCTION(BlueprintCallable, Category = "Hand|Actions")
	virtual void PlayCardFromHand(UFCCardWidget* CardWidget);

	/** Selects and holds a card by slot index (0..9), moving it to follow the mouse cursor */
	UFUNCTION(BlueprintCallable, Category = "Hand|Interaction")
	virtual bool SelectAndHoldCardByIndex(int32 Index);

	/** Selects and holds a card by GUID, moving it to follow the mouse cursor */
	UFUNCTION(BlueprintCallable, Category = "Hand|Interaction")
	virtual bool SelectAndHoldCardByGuid(const FGuid& CardGuid);

	/** Cancels holding the card and returns it to its position in the hand fan layout */
	UFUNCTION(BlueprintCallable, Category = "Hand|Interaction")
	virtual void ClearHeldCard();

	/** Plays the currently held card (if any) and removes it from hand */
	UFUNCTION(BlueprintCallable, Category = "Hand|Actions")
	virtual bool PlayHeldCard();

	/** Gets the currently held card widget */
	UFUNCTION(BlueprintPure, Category = "Hand|Interaction")
	UFCCardWidget* GetHeldCardWidget() const { return HeldCardWidget.Get(); }

	/** Checks if a card is currently held following the mouse */
	UFUNCTION(BlueprintPure, Category = "Hand|Interaction")
	bool HasHeldCard() const { return HeldCardWidget.IsValid(); }

	/** Recalculates and applies fan layout transforms across all active card widgets */
	UFUNCTION(BlueprintCallable, Category = "Hand|FanLayout")
	void UpdateFanLayout(bool bImmediate = false);

	/** Computes fan layout transform for a card at a given index */
	UFUNCTION(BlueprintPure, Category = "Hand|FanLayout")
	void CalculateFanTransformForIndex(
		int32 CardIndex,
		int32 TotalCards,
		bool bIsCardHovered,
		bool bIsCardSelected,
		FVector2D& OutTranslation,
		float& OutAngle,
		FVector2D& OutScale,
		int32& OutZOrder) const;

	// --- Hand Interaction Delegates ---
	UPROPERTY(BlueprintAssignable, Category = "Hand|Interaction")
	FOnHandCardSelectedSignature OnHandCardSelected;

	UPROPERTY(BlueprintAssignable, Category = "Hand|Interaction")
	FOnHandCardDragStartedSignature OnHandCardDragStarted;

	UPROPERTY(BlueprintAssignable, Category = "Hand|Interaction")
	FOnHandCardDraggedSignature OnHandCardDragged;

	UPROPERTY(BlueprintAssignable, Category = "Hand|Interaction")
	FOnHandCardDragEndedSignature OnHandCardDragEnded;

	// --- Fan Layout Configuration Properties ---
	/** Base horizontal spacing between cards (in pixels) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|FanLayout")
	float CardSpacing = 110.0f;

	/** Maximum total width of the hand. Spacing compresses dynamically when cards exceed this width */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|FanLayout")
	float MaxHandWidth = 800.0f;

	/** Height of the vertical arc drop curve for outer edge cards (in pixels) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|FanLayout")
	float ArcHeight = 35.0f;

	/** Maximum total fan rotation spread in degrees across the whole hand */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|FanLayout")
	float MaxFanAngle = 30.0f;

	/** Base rotation angle step per card in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|FanLayout")
	float AngleStep = 4.0f;

	/** Pivot point for card rotation and scaling (default is bottom-center: 0.5, 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|FanLayout")
	FVector2D RenderPivot = FVector2D(0.5f, 1.0f);

	/** Vertical lift (in pixels) applied to a card when hovered (default 0: hover only enlarges) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|FanLayout")
	float HoverLiftY = 0.0f;

	/** Scale multiplier applied to a card when hovered */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|FanLayout")
	float HoverScale = 1.15f;

	/** Scale multiplier applied to a card when being dragged */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|FanLayout")
	float DragScale = 1.15f;

	/** Vertical lift (in pixels) applied to a card when selected (default 0: no vertical pop-up on click) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|FanLayout")
	float SelectedLiftY = 0.0f;

	/** Scale multiplier applied to a card when selected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|FanLayout")
	float SelectedScale = 1.0f;

	/** Upward drag threshold in pixels to trigger card play on drop (e.g. -30px upwards) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|DragDrop")
	float PlayDragDropThresholdY = -30.0f;

	/** Minimum total drag distance in pixels to trigger card play on drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|DragDrop")
	float MinDragDropDistance = 50.0f;

	/** Whether dropping a dragged card automatically triggers card play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|DragDrop")
	bool bAutoPlayOnDragDrop = true;

	/** Whether hovering over a card straightens its angle to 0 degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|FanLayout")
	bool bStraightenOnHover = false;

	/** Whether to smoothly interpolate card transforms toward their fan layout targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|FanLayout")
	bool bEnableInterp = true;

	/** Interpolation speed for smooth card layout animations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|FanLayout")
	float LayoutInterpSpeed = 15.0f;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Optional UMG Panel variable (named CardContainer in Blueprint) that will automatically host the card widgets */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Hand|View")
	TObjectPtr<class UPanelWidget> CardContainer;

	/** Widget blueprint class used to instantiate individual card views */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand|Config")
	TSubclassOf<UFCCardWidget> CardWidgetClass;

	/** List of instantiated active card widgets currently displayed */
	UPROPERTY(BlueprintReadOnly, Category = "Hand|View")
	TArray<TObjectPtr<UFCCardWidget>> ActiveCardWidgets;

	/** Weak reference to currently hovered card widget */
	UPROPERTY(BlueprintReadOnly, Category = "Hand|View")
	TWeakObjectPtr<UFCCardWidget> HoveredCardWidget;

	/** Weak reference to currently dragged card widget */
	UPROPERTY(BlueprintReadOnly, Category = "Hand|View")
	TWeakObjectPtr<UFCCardWidget> DraggedCardWidget;

	/** Weak reference to currently held card widget (following mouse via number key selection) */
	UPROPERTY(BlueprintReadOnly, Category = "Hand|View")
	TWeakObjectPtr<UFCCardWidget> HeldCardWidget;

	/** Blueprint hook triggered whenever a new ViewModel is bound */
	UFUNCTION(BlueprintImplementableEvent, Category = "Hand|Events")
	void OnHandViewModelAssigned(UFCHandViewModel* NewViewModel);

	/** Blueprint hook triggered whenever cards in hand change */
	UFUNCTION(BlueprintImplementableEvent, Category = "Hand|Events")
	void OnHandCardsChanged();

	UFUNCTION()
	virtual void OnHandCardsUpdated();

	UPROPERTY(BlueprintReadOnly, Category = "Hand|ViewModel")
	TObjectPtr<UFCHandViewModel> HandViewModel;
};
