#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FCStatusToggleContainerWidget.generated.h"

class UFCStatusToggleContainerViewModel;
class UFCStatusToggleItemWidget;
class UPanelWidget;

/**
 * UFCStatusToggleContainerWidget
 * 
 * Container UMG widget housing active circular status toggle items.
 * Placed inside PlayerInfoContainer in WBP_HUD.
 * Features automatic widget pooling to eliminate garbage generation on status changes.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class FC_API UFCStatusToggleContainerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFCStatusToggleContainerWidget(const FObjectInitializer& ObjectInitializer);

	/** Assigns the presentation container ViewModel to this widget */
	UFUNCTION(BlueprintCallable, Category = "Status|ViewModel")
	virtual void SetContainerViewModel(UFCStatusToggleContainerViewModel* InViewModel);

	/** Gets the active container presentation ViewModel */
	UFUNCTION(BlueprintPure, Category = "Status|ViewModel")
	UFCStatusToggleContainerViewModel* GetContainerViewModel() const { return ContainerViewModel; }

	/** Re-evaluates active status items and synchronizes child item widgets with pooling */
	UFUNCTION(BlueprintCallable, Category = "Status|View")
	virtual void RefreshWidgets();

	/** Gets active instantiated child item widgets */
	const TArray<TObjectPtr<UFCStatusToggleItemWidget>>& GetActiveItemWidgets() const { return ActiveItemWidgets; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Target layout panel (e.g. HorizontalBox or WrapBox) inside the blueprint */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Status|Widgets")
	TObjectPtr<UPanelWidget> StatusPanel;

	/** Widget class used to instantiate status toggle circular items */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status|Config")
	TSubclassOf<UFCStatusToggleItemWidget> ItemWidgetClass;

	/** Blueprint hook triggered whenever a new ViewModel is bound */
	UFUNCTION(BlueprintImplementableEvent, Category = "Status|Events")
	void OnContainerViewModelAssigned(UFCStatusToggleContainerViewModel* NewViewModel);

	/** Pre-allocated or recycled item widgets */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UFCStatusToggleItemWidget>> WidgetPool;

	/** Currently visible status item widgets */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UFCStatusToggleItemWidget>> ActiveItemWidgets;

	UPROPERTY(BlueprintReadOnly, Category = "Status|ViewModel")
	TObjectPtr<UFCStatusToggleContainerViewModel> ContainerViewModel;

private:
	UFUNCTION()
	void HandleStatusListUpdated();
};
