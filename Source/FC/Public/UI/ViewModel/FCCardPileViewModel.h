#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Data/Card/FCCardTypes.h"
#include "Data/Card/FCCardPileContainer.h"
#include "FCCardPileViewModel.generated.h"

class UFCCardViewModel;
class UFCCardSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPileCardsUpdatedSignature);

/**
 * UFCCardPileViewModel
 *
 * Presentation ViewModel for the Deck (Draw Pile) and Discard Pile inspection modal.
 * Displays canonically sorted cards (never actual draw order) via FieldNotify and Widget pooling.
 */
UCLASS(BlueprintType)
class FC_API UFCCardPileViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** Delegate fired whenever displayed pile cards or open state change */
	UPROPERTY(BlueprintAssignable, Category = "FC|Pile")
	FOnPileCardsUpdatedSignature OnPileCardsUpdated;

	// --- Presentation State ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Pile")
	bool bIsOpen = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Pile")
	EFCCardZone ActiveZone = EFCCardZone::DrawPile;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Pile")
	FText PileTitle;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Pile")
	int32 CardCount = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Pile")
	TArray<TObjectPtr<UFCCardViewModel>> Cards;

public:
	/** Updates cached sorted pile summaries from replicated deck containers */
	void UpdatePileSummaries(
		const FFCCardPileContainer& InDrawSummary,
		const FFCCardPileContainer& InDiscardSummary,
		UFCCardSubsystem* InCardSubsystem);

	/** Opens the pile viewer for the specified zone (DrawPile or DiscardPile) */
	UFUNCTION(BlueprintCallable, Category = "FC|Pile")
	void OpenPile(EFCCardZone InZone, UFCCardSubsystem* InCardSubsystem = nullptr);

	/** Toggles the pile viewer: closes if already showing InZone, otherwise opens/switches to InZone */
	UFUNCTION(BlueprintCallable, Category = "FC|Pile")
	void TogglePile(EFCCardZone InZone, UFCCardSubsystem* InCardSubsystem = nullptr);

	/** Closes the pile viewer modal */
	UFUNCTION(BlueprintCallable, Category = "FC|Pile")
	void ClosePile();

	UFUNCTION(BlueprintPure, FieldNotify, Category = "FC|Pile")
	FText GetCardCountDisplayText() const;

private:
	void RebuildCards(UFCCardSubsystem* InCardSubsystem);
	static FText GetTitleForZone(EFCCardZone InZone);

	TArray<FFCCardPileEntry> CachedDrawEntries;
	TArray<FFCCardPileEntry> CachedDiscardEntries;
	TWeakObjectPtr<UFCCardSubsystem> CachedCardSubsystem;
};
