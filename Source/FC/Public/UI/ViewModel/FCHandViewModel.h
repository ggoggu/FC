#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "FCHandViewModel.generated.h"

class UFCCardViewModel;
class UFCCardSubsystem;
struct FFCCardHandContainer;
struct FFCCardItem;

/**
 * UFCHandViewModel
 * 
 * Presentation ViewModel managing the active hand of cards.
 * Manages child card ViewModels, selection state, and playability validation.
 */
UCLASS(BlueprintType)
class FC_API UFCHandViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	// --- Collection State ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Hand")
	TArray<TObjectPtr<UFCCardViewModel>> CardsInHand;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Hand")
	int32 SelectedCardIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Hand")
	int32 MaxHandSize = 10;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Hand")
	int32 CurrentHandCount = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Hand")
	bool bHasSelection = false;

public:
	// Synchronization & Mutation API
	void SyncFromHandContainer(const FFCCardHandContainer& Container, UFCCardSubsystem* DataSubsystem);
	void UpdateCardItem(const FFCCardItem& InItem, UFCCardSubsystem* DataSubsystem);

	UFUNCTION(BlueprintCallable, Category = "FC|Hand")
	void SelectCardByGuid(const FGuid& InGuid);

	UFUNCTION(BlueprintCallable, Category = "FC|Hand")
	void SelectCardByIndex(int32 InIndex);

	UFUNCTION(BlueprintCallable, Category = "FC|Hand")
	void ClearSelection();

	UFUNCTION(BlueprintPure, Category = "FC|Hand")
	UFCCardViewModel* GetSelectedCard() const;

	UFUNCTION(BlueprintCallable, Category = "FC|Hand")
	void UpdatePlayability(int32 CurrentPlayerMana);

	void SetMaxHandSize(int32 InMax) { UE_MVVM_SET_PROPERTY_VALUE(MaxHandSize, InMax); }
};
