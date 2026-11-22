#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeSet.h"
#include "FCHUDPresenterComponent.generated.h"

class UFCHUDWidget;
class UFCHUDViewModel;
class UFCStatusPresenterComponent;
class UAbilitySystemComponent;
class UFCCardDeckComponent;

/**
 * UFCHUDPresenterComponent
 *
 * Client-side Presentation Adapter Component responsible for spawning the player HUD,
 * creating/owning UFCHUDViewModel, binding UFCCardDeckComponent events, and subscribing
 * to GAS AttributeSet changes. Isolates UI presentation concerns from AFCPlayerController.
 */
UCLASS(ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class FC_API UFCHUDPresenterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFCHUDPresenterComponent();

	/** Spawns/binds HUD widget & ViewModel and attaches Deck & ASC listeners */
	UFUNCTION(BlueprintCallable, Category = "UI|HUD")
	void SetupHUD(TSubclassOf<UFCHUDWidget> InHUDWidgetClass, UFCStatusPresenterComponent* StatusPresenter = nullptr);

	/** Unbinds all active ASC attribute listeners and forwards unbind to StatusPresenter */
	UFUNCTION(BlueprintCallable, Category = "UI|HUD")
	void UnbindAttributeListeners(UFCStatusPresenterComponent* StatusPresenter = nullptr);

	/** Rebinds deck and ASC listeners (called after pawn possession or PlayerState replication) */
	UFUNCTION(BlueprintCallable, Category = "UI|HUD")
	void RefreshBindings(UFCStatusPresenterComponent* StatusPresenter = nullptr);

	UFUNCTION(BlueprintPure, Category = "UI|HUD")
	UFCHUDWidget* GetHUDWidget() const { return HUDWidget; }

	UFUNCTION(BlueprintPure, Category = "UI|HUD")
	UFCHUDViewModel* GetHUDViewModel() const { return HUDViewModel; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI|HUD")
	TObjectPtr<UFCHUDWidget> HUDWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI|HUD")
	TObjectPtr<UFCHUDViewModel> HUDViewModel;

	UFUNCTION()
	void SyncHandToViewModel();

	UFUNCTION()
	void SyncPileSummariesToViewModel();

	UFUNCTION()
	void OnDeckPileCountsChanged(int32 DrawCount, int32 DiscardCount, int32 ExhaustCount);

	UFUNCTION()
	void OnDeckCycleTriggered();

	UFUNCTION()
	void OnDeckCycleSettingsChanged(float NewInterval, int32 NewDrawCount);

	UFUNCTION()
	void UpdateCycleHUD();

private:
	void BindDeckComponentEvents();
	void UnbindDeckComponentEvents();
	void BindAttributeListeners(UFCStatusPresenterComponent* StatusPresenter);

	FTimerHandle CycleHUDUpdateTimerHandle;

	UPROPERTY(Transient)
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	UPROPERTY(Transient)
	TWeakObjectPtr<UFCCardDeckComponent> BoundDeckComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<UFCStatusPresenterComponent> CachedStatusPresenter;

	TArray<TTuple<FGameplayAttribute, FDelegateHandle>> BoundAttributeHandles;
};
