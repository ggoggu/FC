#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FCHandWidget.generated.h"

class UFCHandViewModel;
class UFCCardWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHandCardSelectedSignature, UFCCardWidget*, SelectedCardWidget);

/**
 * UFCHandWidget
 * 
 * UMG UserWidget managing the player's card hand container,
 * fan layout organization, card slot generation, and selection dispatching.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
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

	/** Helper to instantiate and populate child card widgets inside a UMG panel (e.g. HorizontalBox or Overlay) */
	UFUNCTION(BlueprintCallable, Category = "Hand|View")
	void RefreshCardWidgets(class UPanelWidget* TargetPanel);

	// --- Hand Interaction Delegates ---
	UPROPERTY(BlueprintAssignable, Category = "Hand|Interaction")
	FOnHandCardSelectedSignature OnHandCardSelected;

protected:
	/** Widget blueprint class used to instantiate individual card views */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand|Config")
	TSubclassOf<UFCCardWidget> CardWidgetClass;

	/** Blueprint hook triggered whenever a new ViewModel is bound */
	UFUNCTION(BlueprintImplementableEvent, Category = "Hand|Events")
	void OnHandViewModelAssigned(UFCHandViewModel* NewViewModel);

	UPROPERTY(BlueprintReadOnly, Category = "Hand|ViewModel")
	TObjectPtr<UFCHandViewModel> HandViewModel;
};
