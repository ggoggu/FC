#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "GameplayTagContainer.h"
#include "UI/ClassSelect/FCClassSelectTypes.h"
#include "FCClassSelectViewModel.generated.h"

class UFCClassSelectItemViewModel;
class UFCClassSubsystem;
class APlayerController;

/**
 * UFCClassSelectViewModel
 * 
 * Master Presentation ViewModel for the Class Selection Screen.
 * Populates class cards dynamically from UFCClassSubsystem / DT_ClassCatalog (zero class switches).
 */
UCLASS(BlueprintType)
class FC_API UFCClassSelectViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	// --- Collection & Selection State ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect")
	TArray<TObjectPtr<UFCClassSelectItemViewModel>> ClassItemList;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect")
	TObjectPtr<UFCClassSelectItemViewModel> SelectedItem;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect")
	FGameplayTag SelectedClassTag;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect")
	bool bHasSelectedClass = false;

	// --- Events ---
	UPROPERTY(BlueprintAssignable, Category = "FC|ClassSelect|Events")
	FOnClassSelectedSignature OnClassSelected;

	UPROPERTY(BlueprintAssignable, Category = "FC|ClassSelect|Events")
	FOnClassConfirmedSignature OnClassConfirmed;

public:
	/** Initializes class items catalog dynamically from UFCClassSubsystem */
	UFUNCTION(BlueprintCallable, Category = "FC|ClassSelect")
	void InitializeClasses(UFCClassSubsystem* Subsystem);

	/** Attempts to select a character class GameplayTag. Returns true if successfully selected. */
	UFUNCTION(BlueprintCallable, Category = "FC|ClassSelect")
	bool SelectClass(FGameplayTag InClassTag);

	/** Computes whether the player can confirm and proceed with the current selection */
	UFUNCTION(BlueprintPure, FieldNotify, Category = "FC|ClassSelect")
	bool GetCanConfirmSelection() const;

	/** Confirms current selection, stores to player persistence/state, and broadcasts OnClassConfirmed */
	UFUNCTION(BlueprintCallable, Category = "FC|ClassSelect")
	bool ConfirmSelection(APlayerController* PC, const TArray<FName>& StartingDeckOverride);

	// Direct setters
	void SetSelectedItem(UFCClassSelectItemViewModel* InItem);
	void SetSelectedClassTag(FGameplayTag InTag);
	void SetHasSelectedClass(bool bInHas);
};
