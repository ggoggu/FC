#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "FCCardRewardWidget.generated.h"

class UFCCardRewardViewModel;
class AFCCardPickupActor;

/**
 * UFCCardRewardWidget
 * 
 * Presentation View Widget for the Card Reward / Pickup Selection Modal.
 * Displays card information via MVVM binding and handles Acquire/Discard actions
 * via mouse clicks or configurable keyboard shortcut keys (AcquireKey / DiscardKey).
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class FC_API UFCCardRewardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFCCardRewardWidget(const FObjectInitializer& ObjectInitializer);

	/** Initializes the reward widget with target Card ID and source pickup actor */
	UFUNCTION(BlueprintCallable, Category = "Card|Reward")
	virtual void SetupRewardWidget(FName InCardId, AFCCardPickupActor* InSourceActor);

	/** Handles Acquire button click / action */
	UFUNCTION(BlueprintCallable, Category = "Card|Reward")
	virtual void OnAcquireClicked();

	/** Handles Discard / Skip button click / action */
	UFUNCTION(BlueprintCallable, Category = "Card|Reward")
	virtual void OnDiscardClicked();

	/** Dismisses and removes the widget from the viewport */
	UFUNCTION(BlueprintCallable, Category = "Card|Reward")
	virtual void CloseRewardWidget();

	// --- Configurable Keyboard Shortcut Keys ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Input")
	FKey AcquireKey = EKeys::SpaceBar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Input")
	FKey DiscardKey = EKeys::Escape;

	UFUNCTION(BlueprintPure, Category = "Card|ViewModel")
	UFCCardRewardViewModel* GetRewardViewModel() const { return RewardViewModel; }

protected:
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Blueprint hook triggered whenever the ViewModel is initialized */
	UFUNCTION(BlueprintImplementableEvent, Category = "Card|Reward")
	void OnRewardViewModelAssigned(UFCCardRewardViewModel* NewViewModel);

	UPROPERTY(BlueprintReadOnly, Category = "Card|ViewModel")
	TObjectPtr<UFCCardRewardViewModel> RewardViewModel;

	UPROPERTY(BlueprintReadOnly, Category = "Card|Reward")
	TWeakObjectPtr<AFCCardPickupActor> SourcePickupActor;
};
