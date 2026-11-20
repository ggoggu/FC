#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "UI/ClassSelect/FCClassSelectTypes.h"
#include "FCClassSelectWidget.generated.h"

class UFCClassSelectViewModel;
class UFCClassSelectItemWidget;
class UPanelWidget;
class UTextBlock;
class UButton;

/**
 * UFCClassSelectWidget
 * 
 * Master Widget for Class Selection Screen (WBP_ClassSelect).
 * Resolves starting decks from optional per-tag Blueprint overrides or UFCClassSubsystem (SSOT).
 */
UCLASS(Abstract, BlueprintType)
class FC_API UFCClassSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Binds this widget to the master class selection ViewModel */
	UFUNCTION(BlueprintCallable, Category = "FC|ClassSelect")
	void InitializeWidget(UFCClassSelectViewModel* InViewModel);

	/** Returns current ViewModel */
	UFUNCTION(BlueprintPure, Category = "FC|ClassSelect")
	UFCClassSelectViewModel* GetViewModel() const { return ViewModel; }

	/** Retrieves the starting deck configured for the given character class GameplayTag */
	UFUNCTION(BlueprintCallable, Category = "FC|ClassSelect|StartingDeck")
	TArray<FName> GetStartingDeckForClass(FGameplayTag InClassTag) const;

	/** Optional per-class Blueprint starting deck overrides keyed by Class.* GameplayTag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|ClassSelect|StartingDeck")
	TMap<FGameplayTag, FFCClassStartingDeckConfig> StartingDeckOverrides;

	/** Blueprint hook when class selection changes (e.g. for SFX or VFX triggers) */
	UFUNCTION(BlueprintImplementableEvent, Category = "FC|ClassSelect|Events")
	void BP_OnClassSelected(FGameplayTag SelectedClassTag);

	/** Blueprint hook when class is confirmed */
	UFUNCTION(BlueprintImplementableEvent, Category = "FC|ClassSelect|Events")
	void BP_OnClassConfirmed(FGameplayTag ConfirmedClassTag);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "FC|ClassSelect")
	TObjectPtr<UFCClassSelectViewModel> ViewModel;

	/** WBP child widget class used to spawn class cards */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|ClassSelect|UI")
	TSubclassOf<UFCClassSelectItemWidget> ItemWidgetClass;

	// --- Optional UMG Bindings for Design Flexibility ---
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|ClassSelect|Bindings")
	TObjectPtr<UPanelWidget> ClassListContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|ClassSelect|Bindings")
	TObjectPtr<UTextBlock> Text_DetailName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|ClassSelect|Bindings")
	TObjectPtr<UTextBlock> Text_DetailDesc;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|ClassSelect|Bindings")
	TObjectPtr<UTextBlock> Text_DetailStats;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|ClassSelect|Bindings")
	TObjectPtr<UTextBlock> Text_DetailElements;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|ClassSelect|Bindings")
	TObjectPtr<UButton> Btn_Confirm;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UFCClassSelectItemWidget>> SpawnedItemWidgets;

	UFUNCTION()
	void HandleClassItemClicked(FGameplayTag ClickedClassTag);

	UFUNCTION()
	void HandleConfirmButtonClicked();

	UFUNCTION()
	void HandleViewModelClassSelected(FGameplayTag SelectedClassTag);

	UFUNCTION()
	void HandleViewModelClassConfirmed(FGameplayTag ConfirmedClassTag, const TArray<FName>& StartingDeck);

	void RefreshDetailPanel();
	void BuildClassItems();
};
