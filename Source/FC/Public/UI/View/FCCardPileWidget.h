#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FCCardPileWidget.generated.h"

class UFCCardPileViewModel;
class UFCCardWidget;
class UPanelWidget;
class UTextBlock;
class UButton;

/**
 * UFCCardPileWidget
 *
 * Modal UMG widget displaying canonically sorted cards from the Draw Pile (Deck)
 * or Discard Pile using pooled display-only UFCCardWidget instances.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class FC_API UFCCardPileWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFCCardPileWidget(const FObjectInitializer& ObjectInitializer);

	/** Assigns the pile presentation ViewModel to this widget */
	UFUNCTION(BlueprintCallable, Category = "Pile|ViewModel")
	virtual void SetPileViewModel(UFCCardPileViewModel* InViewModel);

	/** Gets the active pile presentation ViewModel */
	UFUNCTION(BlueprintPure, Category = "Pile|ViewModel")
	UFCCardPileViewModel* GetPileViewModel() const { return PileViewModel; }

	/** Synchronizes pooled card widgets and modal visibility from PileViewModel */
	UFUNCTION(BlueprintCallable, Category = "Pile|View")
	virtual void RefreshCardWidgets();

	/** Closes the pile viewer modal via the ViewModel */
	UFUNCTION(BlueprintCallable, Category = "Pile|Actions")
	virtual void ClosePileViewer();

	/** Gets the list of currently active card widgets in the grid */
	const TArray<TObjectPtr<UFCCardWidget>>& GetActiveCardWidgets() const { return ActiveCardWidgets; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Target grid/wrap panel (e.g. WrapBox inside ScrollBox) hosting pooled card widgets */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Pile|Widgets")
	TObjectPtr<UPanelWidget> CardGridContainer;

	/** Optional header title text block */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Pile|Widgets")
	TObjectPtr<UTextBlock> Text_PileTitle;

	/** Optional total card count text block */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Pile|Widgets")
	TObjectPtr<UTextBlock> Text_PileCount;

	/** Optional close button */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Pile|Widgets")
	TObjectPtr<UButton> Btn_Close;

	/** Optional dimmed backdrop button that closes the modal when clicked */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Pile|Widgets")
	TObjectPtr<UButton> Btn_Backdrop;

	/** Widget blueprint class used to instantiate individual card views (e.g. WBP_CardWidget) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pile|Config")
	TSubclassOf<UFCCardWidget> CardWidgetClass;

	/** Reusable card widget pool */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Pile|Pool")
	TArray<TObjectPtr<UFCCardWidget>> CardWidgetPool;

	/** Currently visible card widgets */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Pile|View")
	TArray<TObjectPtr<UFCCardWidget>> ActiveCardWidgets;

	/** Blueprint hook triggered whenever a new ViewModel is bound */
	UFUNCTION(BlueprintImplementableEvent, Category = "Pile|Events")
	void OnPileViewModelAssigned(UFCCardPileViewModel* NewViewModel);

	UPROPERTY(BlueprintReadOnly, Category = "Pile|ViewModel")
	TObjectPtr<UFCCardPileViewModel> PileViewModel;

private:
	UFUNCTION()
	void HandlePileCardsUpdated();

	UFUNCTION()
	void HandleCloseClicked();
};
