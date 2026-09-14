#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FCElementOverheadWidget.generated.h"

class UFCElementOverheadViewModel;
class UFCElementStackItemWidget;
class UFCElementComponent;
class UPanelWidget;
class UProgressBar;
class UTextBlock;

/**
 * UFCElementOverheadWidget
 * 
 * Top-level overhead UMG widget attached to enemy characters (via UWidgetComponent).
 * Displays active elemental stacks accumulated by Mage cards.
 * Subscribes to UFCElementOverheadViewModel for zero-tick MVVM updates.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class FC_API UFCElementOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFCElementOverheadWidget(const FObjectInitializer& ObjectInitializer);

	/** Assigns the overhead presentation ViewModel */
	UFUNCTION(BlueprintCallable, Category = "FC|Element|ViewModel")
	virtual void SetViewModel(UFCElementOverheadViewModel* InViewModel);

	/** Gets the active overhead presentation ViewModel */
	UFUNCTION(BlueprintPure, Category = "FC|Element|ViewModel")
	UFCElementOverheadViewModel* GetViewModel() const { return ViewModel; }

	/** Convenience initialization helper: searches InActor for UFCElementComponent and binds */
	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	virtual void InitializeForActor(AActor* InActor);

	/** Convenience initialization helper: binds directly to a UFCElementComponent */
	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	virtual void InitializeForElementComponent(UFCElementComponent* InElementComponent);

	/** Refreshes child token item widgets inside StackListContainer */
	UFUNCTION(BlueprintCallable, Category = "FC|Element|View")
	virtual void RefreshStackWidgets();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Callback when ViewModel broadcasts stack changes */
	UFUNCTION()
	virtual void HandleStacksUpdated(int32 NewTotalStacks);

	/** Blueprint hook triggered whenever a new ViewModel is bound */
	UFUNCTION(BlueprintImplementableEvent, Category = "FC|Element|Events")
	void OnViewModelAssigned(UFCElementOverheadViewModel* NewViewModel);

	/** Blueprint hook triggered whenever stacks are updated */
	UFUNCTION(BlueprintImplementableEvent, Category = "FC|Element|Events")
	void OnStacksUpdated(int32 NewTotalStacks);

	UPROPERTY(BlueprintReadOnly, Category = "FC|Element|ViewModel")
	TObjectPtr<UFCElementOverheadViewModel> ViewModel;

	/** Optional container panel (e.g. HorizontalBox) hosting child token widgets */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|Element|View")
	TObjectPtr<UPanelWidget> StackListContainer;

	/** Optional progress bar representing stack fullness */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|Element|View")
	TObjectPtr<UProgressBar> Bar_Stacks;

	/** Optional summary text block (e.g. "3 / 7") */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|Element|View")
	TObjectPtr<UTextBlock> Text_TotalStacks;

	/** Widget class to instantiate for each individual stack token inside StackListContainer */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Element|Config")
	TSubclassOf<UFCElementStackItemWidget> StackItemWidgetClass;

	/** If true, widget collapses when stack count is 0 and becomes visible when stacks > 0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Element|Config")
	bool bAutoHideWhenEmpty = true;
};
