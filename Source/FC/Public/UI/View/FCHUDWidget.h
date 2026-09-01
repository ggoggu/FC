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

protected:
	/** Blueprint hook triggered whenever a new ViewModel is bound */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Events")
	void OnHUDViewModelAssigned(UFCHUDViewModel* NewViewModel);

	UPROPERTY(BlueprintReadOnly, Category = "HUD|ViewModel")
	TObjectPtr<UFCHUDViewModel> HUDViewModel;
};
