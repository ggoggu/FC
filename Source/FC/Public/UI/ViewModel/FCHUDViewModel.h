#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "FCHUDViewModel.generated.h"

class UFCHandViewModel;

/**
 * UFCHUDViewModel
 * 
 * Top-level HUD presentation ViewModel holding player stats (Health, Mana),
 * pile counters, and child HandViewModel.
 */
UCLASS(BlueprintType)
class FC_API UFCHUDViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	// --- Player Attributes ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|HUD|Player")
	int32 CurrentHealth = 100;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|HUD|Player")
	int32 MaxHealth = 100;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|HUD|Player")
	int32 CurrentMana = 3;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|HUD|Player")
	int32 MaxMana = 3;

	// --- Deck / Combat State ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|HUD|Deck")
	int32 DrawPileCount = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|HUD|Deck")
	int32 DiscardPileCount = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|HUD|Deck")
	int32 ExhaustPileCount = 0;

	// --- Sub-ViewModel ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|HUD|Hand")
	TObjectPtr<UFCHandViewModel> HandViewModel;

public:
	UFCHandViewModel* GetOrCreateHandViewModel();

	// Setters utilizing UE_MVVM_SET_PROPERTY_VALUE
	void SetCurrentHealth(int32 InHealth);
	void SetMaxHealth(int32 InMaxHealth);
	void SetCurrentMana(int32 InMana);
	void SetMaxMana(int32 InMaxMana);

	void SetDrawPileCount(int32 InCount) { UE_MVVM_SET_PROPERTY_VALUE(DrawPileCount, InCount); }
	void SetDiscardPileCount(int32 InCount) { UE_MVVM_SET_PROPERTY_VALUE(DiscardPileCount, InCount); }
	void SetExhaustPileCount(int32 InCount) { UE_MVVM_SET_PROPERTY_VALUE(ExhaustPileCount, InCount); }

	// --- Computed FieldNotify Getters ---
	UFUNCTION(BlueprintPure, FieldNotify)
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, FieldNotify)
	float GetManaPercent() const;

	UFUNCTION(BlueprintPure, FieldNotify)
	FText GetHealthDisplayText() const;

	UFUNCTION(BlueprintPure, FieldNotify)
	FText GetManaDisplayText() const;
};
