#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "FCHandViewModel.generated.h"

class UFCCardViewModel;
class UFCCardSubsystem;
struct FFCCardHandContainer;
struct FFCCardItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHandCardsUpdatedSignature);

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
	/** Delegate fired whenever cards in hand change or are re-synced */
	UPROPERTY(BlueprintAssignable, Category = "FC|Hand")
	FOnHandCardsUpdatedSignature OnCardsUpdated;

	// --- Collection State ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Hand")
	TArray<TObjectPtr<UFCCardViewModel>> CardsInHand;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Hand")
	int32 SelectedCardIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Hand")
	int32 HoveredCardIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Hand")
	int32 MaxHandSize = 10;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Hand")
	int32 CurrentHandCount = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Hand")
	bool bHasSelection = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Hand")
	bool bHasHover = false;

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
	void HoverCardByGuid(const FGuid& InGuid);

	UFUNCTION(BlueprintCallable, Category = "FC|Hand")
	void HoverCardByIndex(int32 InIndex);

	UFUNCTION(BlueprintCallable, Category = "FC|Hand")
	void ClearHover();

	UFUNCTION(BlueprintPure, Category = "FC|Hand")
	UFCCardViewModel* GetHoveredCard() const;

	UFUNCTION(BlueprintCallable, Category = "FC|Hand")
	void UpdatePlayability(int32 CurrentPlayerMana);

	void SetMaxHandSize(int32 InMax) { UE_MVVM_SET_PROPERTY_VALUE(MaxHandSize, InMax); }

	/**
	 * Pure calculation utility to compute fan layout position and angle for a card in hand.
	 * 
	 * @param CardIndex Index of the card (0-based)
	 * @param TotalCards Total number of cards in hand
	 * @param CardSpacing Base horizontal spacing between cards in pixels
	 * @param MaxHandWidth Maximum total width of the hand before spacing is automatically compressed
	 * @param ArcHeight Height of the vertical arc drop for outer cards in pixels
	 * @param MaxFanAngle Maximum total rotation angle spread across the hand in degrees
	 * @param AngleStep Base rotation angle step per card in degrees
	 * @param OutTranslation Computed 2D translation offset (X horizontal offset, Y arc drop)
	 * @param OutAngle Computed tilt angle in degrees (negative for left cards, positive for right cards)
	 */
	UFUNCTION(BlueprintPure, Category = "FC|Hand|FanLayout")
	static void CalculateCardFanTransform(
		int32 CardIndex,
		int32 TotalCards,
		float CardSpacing,
		float MaxHandWidth,
		float ArcHeight,
		float MaxFanAngle,
		float AngleStep,
		FVector2D& OutTranslation,
		float& OutAngle);
};
