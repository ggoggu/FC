#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FCElementStackItemWidget.generated.h"

class UFCElementStackItemViewModel;
class UImage;
class UBorder;
class UTextBlock;

/**
 * UFCElementStackItemWidget
 * 
 * UMG UserWidget representing an individual element stack token or gem.
 * Displays element color, icon/initial, and slot index.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class FC_API UFCElementStackItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFCElementStackItemWidget(const FObjectInitializer& ObjectInitializer);

	/** Assigns the item presentation ViewModel to this widget */
	UFUNCTION(BlueprintCallable, Category = "FC|Element|ViewModel")
	virtual void SetItemViewModel(UFCElementStackItemViewModel* InViewModel);

	/** Gets the active item presentation ViewModel */
	UFUNCTION(BlueprintPure, Category = "FC|Element|ViewModel")
	UFCElementStackItemViewModel* GetItemViewModel() const { return ItemViewModel; }

protected:
	virtual void NativeConstruct() override;

	/** Blueprint event hook triggered whenever a new ViewModel is bound */
	UFUNCTION(BlueprintImplementableEvent, Category = "FC|Element|Events")
	void OnItemViewModelAssigned(UFCElementStackItemViewModel* NewViewModel);

	/** Blueprint event hook triggered when stack added animation should play */
	UFUNCTION(BlueprintImplementableEvent, Category = "FC|Element|Events")
	void OnPlayStackAddedAnimation();

	UPROPERTY(BlueprintReadOnly, Category = "FC|Element|ViewModel")
	TObjectPtr<UFCElementStackItemViewModel> ItemViewModel;

	/** Optional element icon image */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|Element|View")
	TObjectPtr<UImage> Img_ElementIcon;

	/** Optional colored background or ring border */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|Element|View")
	TObjectPtr<UBorder> Border_Background;

	/** Optional short text initial (e.g. F, E, W) displayed when icon is absent */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|Element|View")
	TObjectPtr<UTextBlock> Text_ElementInitial;
};
