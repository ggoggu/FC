#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
 * Exposes starting deck configurations as editable Blueprint variables,
 * connects presentation ViewModel to UMG widgets, and routes selection/confirmation flows.
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

	/** Retrieves the starting deck configured for the given character class */
	UFUNCTION(BlueprintCallable, Category = "FC|ClassSelect|StartingDeck")
	TArray<FName> GetStartingDeckForClass(EFCCharacterClass InClass) const;

	// --- Blueprint Editable Starting Deck Settings (As Requested by User) ---
	/**
	 * Default Starting Deck for Mage (Mage is currently the selectable class).
	 * Fully editable in Blueprint Details panel and Event Graph!
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|ClassSelect|StartingDeck")
	TArray<FName> MageStartingDeck = {
		FName("Card_Fireball"),
		FName("Card_Fireball"),
		FName("Card_Fireball"),
		FName("Card_Fireball"),
		FName("Card_Fireball"),
		FName("Card_AttackBuff"),
		FName("Card_AttackBuff"),
		FName("Card_AttackBuff"),
		FName("Card_MagicShield"),
		FName("Card_MagicShield")
	};

	/**
	 * Customizable starting decks per class, easily expandable in BP.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|ClassSelect|StartingDeck")
	TMap<EFCCharacterClass, FFCClassStartingDeckConfig> StartingDeckOverrides;

	/** Blueprint hook when class selection changes (e.g. for SFX or VFX triggers) */
	UFUNCTION(BlueprintImplementableEvent, Category = "FC|ClassSelect|Events")
	void BP_OnClassSelected(EFCCharacterClass SelectedClass);

	/** Blueprint hook when class is confirmed */
	UFUNCTION(BlueprintImplementableEvent, Category = "FC|ClassSelect|Events")
	void BP_OnClassConfirmed(EFCCharacterClass ConfirmedClass);

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
	void HandleClassItemClicked(EFCCharacterClass ClickedClass);

	UFUNCTION()
	void HandleConfirmButtonClicked();

	UFUNCTION()
	void HandleViewModelClassSelected(EFCCharacterClass SelectedClass);

	UFUNCTION()
	void HandleViewModelClassConfirmed(EFCCharacterClass ConfirmedClass, const TArray<FName>& StartingDeck);

	void RefreshDetailPanel();
	void BuildClassItems();
};
