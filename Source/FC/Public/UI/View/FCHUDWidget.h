#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FCHUDWidget.generated.h"

class UFCHUDViewModel;
class UFCHandWidget;

/**
 * UFCHUDWidget
 * 
 * Top-level UMG HUD Widget container.
 * Binds to UFCHUDViewModel and coordinates child widgets (HandWidget, HealthBar, ManaOrb, Deck counters).
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class FC_API UFCHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFCHUDWidget(const FObjectInitializer& ObjectInitializer);

	/** Assigns the HUD presentation ViewModel */
	UFUNCTION(BlueprintCallable, Category = "HUD|ViewModel")
	virtual void SetHUDViewModel(UFCHUDViewModel* InViewModel);

	/** Gets the active HUD presentation ViewModel */
	UFUNCTION(BlueprintPure, Category = "HUD|ViewModel")
	UFCHUDViewModel* GetHUDViewModel() const { return HUDViewModel; }

	/** Gets the child hand widget */
	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UFCHandWidget* GetHandWidget() const { return HandWidget; }

	/** Sets the child hand widget */
	UFUNCTION(BlueprintCallable, Category = "HUD|Widgets")
	void SetHandWidget(UFCHandWidget* InHandWidget) { HandWidget = InHandWidget; }

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Optional child hand widget (named HandWidget in Blueprint) automatically linked to HUDViewModel's HandViewModel */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HUD|Widgets")
	TObjectPtr<UFCHandWidget> HandWidget;

	/** Blueprint hook triggered whenever a new ViewModel is bound */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Events")
	void OnHUDViewModelAssigned(UFCHUDViewModel* NewViewModel);

	UPROPERTY(BlueprintReadOnly, Category = "HUD|ViewModel")
	TObjectPtr<UFCHUDViewModel> HUDViewModel;
};
