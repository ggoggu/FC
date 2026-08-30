#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Card/FCCardTypes.h"
#include "Data/Card/FCCardHandContainer.h"
#include "Data/FCPlayerPersistenceTypes.h"
#include "FCCardDeckComponent.generated.h"

class UFCCardDataAsset;
class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCardHandUpdatedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardItemChangedSignature, const FFCCardItem&, ChangedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardPlayedSignature, const FGuid&, CardGuid, FName, CardId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPileCountsChangedSignature, int32, DrawCount, int32, DiscardCount, int32, ExhaustCount);

/**
 * UFCCardDeckComponent
 * 
 * Modular ActorComponent managing card draw, hand, discard, exhaust piles,
 * server validation, and card playing logic. Can be attached to PlayerState or Pawns.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FC_API UFCCardDeckComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFCCardDeckComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Public Delegates ---
	UPROPERTY(BlueprintAssignable, Category = "Card|Events")
	FOnCardHandUpdatedSignature OnCardHandUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Card|Events")
	FOnCardItemChangedSignature OnCardItemChanged;

	UPROPERTY(BlueprintAssignable, Category = "Card|Events")
	FOnCardPlayedSignature OnCardPlayed;

	UPROPERTY(BlueprintAssignable, Category = "Card|Events")
	FOnPileCountsChangedSignature OnPileCountsChanged;

	// --- Getters ---
	const FFCCardHandContainer& GetHandContainer() const { return HandContainer; }
	int32 GetDrawPileCount() const { return DrawPileCount; }
	int32 GetDiscardPileCount() const { return DiscardPileCount; }
	int32 GetExhaustPileCount() const { return ExhaustPileCount; }

	// --- Server RPCs ---
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Card|Network")
	void Server_PlayCard(const FGuid& CardGuid, const FFCCardTargetInfo& TargetInfo);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Card|Network")
	void Server_DrawCards(int32 Count);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Card|Network")
	void Server_EndTurn();

	// --- Server Authority APIs ---
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	void InitializeDeck(const TArray<FName>& StartingDeck);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	void DrawCards(int32 Count = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	bool DiscardCard(const FGuid& CardGuid);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	bool ExhaustCard(const FGuid& CardGuid);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	bool UpgradeCardInHand(const FGuid& CardGuid, int32 NewUpgradeLevel = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	bool SetCardLockedInHand(const FGuid& CardGuid, bool bLocked);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	void ReshuffleDiscardIntoDraw();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	void DiscardEntireHand();

	/** Dynamically adds a card to the deck, discard pile, or hand */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	bool AddCardToDeck(FName CardId, EFCCardAddDestination Destination = EFCCardAddDestination::DiscardPile, bool bShuffleIfDrawPile = true);

	/** Exports full deck and hand snapshot for level persistence */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Persistence")
	FFCCardDeckSaveData ExportDeckSaveData() const;

	/** Restores full deck and hand snapshot after level transition */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Persistence")
	void RestoreFromDeckSaveData(const FFCCardDeckSaveData& SaveData);

	/** Client-side helper called by FastArray callbacks */
	void NotifyHandChanged();
	void NotifyItemChanged(const FFCCardItem& Item);

protected:
	virtual void BeginPlay() override;

	/** Authoritative Hand cards replicated COND_OwnerOnly */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HandContainer, Category = "Card|Replication")
	FFCCardHandContainer HandContainer;

	/** Replicated pile counts for client HUD */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PileCounts, Category = "Card|Replication")
	int32 DrawPileCount = 0;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PileCounts, Category = "Card|Replication")
	int32 DiscardPileCount = 0;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PileCounts, Category = "Card|Replication")
	int32 ExhaustPileCount = 0;

	UFUNCTION()
	virtual void OnRep_HandContainer();

	UFUNCTION()
	virtual void OnRep_PileCounts();

private:
	// Server-Only Hidden Piles (Preventing Client Fog-of-War Memory Cheats)
	TArray<FName> ServerDrawPile;
	TArray<FName> ServerDiscardPile;
	TArray<FName> ServerExhaustPile;

	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
};
