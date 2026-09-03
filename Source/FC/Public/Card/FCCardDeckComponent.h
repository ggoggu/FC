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
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCardCycleTriggeredSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCycleSettingsChangedSignature, float, NewInterval, int32, NewDrawCount);

/**
 * UFCCardDeckComponent
 * 
 * Modular ActorComponent managing card draw, hand, discard, exhaust piles,
 * periodic turn cycle, server validation, and card playing logic. Can be attached to PlayerState or Pawns.
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

	UPROPERTY(BlueprintAssignable, Category = "Card|Events")
	FOnCardCycleTriggeredSignature OnCardCycleTriggered;

	UPROPERTY(BlueprintAssignable, Category = "Card|Events")
	FOnCycleSettingsChangedSignature OnCycleSettingsChanged;

	// --- Getters ---
	const FFCCardHandContainer& GetHandContainer() const { return HandContainer; }
	int32 GetDrawPileCount() const { return DrawPileCount; }
	int32 GetDiscardPileCount() const { return DiscardPileCount; }
	int32 GetExhaustPileCount() const { return ExhaustPileCount; }
	int32 GetMaxHandSize() const { return MaxHandSize; }

	UFUNCTION(BlueprintPure, Category = "Card|Cycle")
	float GetCycleInterval() const { return CycleInterval; }

	UFUNCTION(BlueprintPure, Category = "Card|Cycle")
	int32 GetCycleDrawCount() const { return CycleDrawCount; }

	UFUNCTION(BlueprintPure, Category = "Card|Cycle")
	bool IsAutoCycleEnabled() const { return bAutoCycleEnabled; }

	UFUNCTION(BlueprintPure, Category = "Card|Cycle")
	float GetCycleRemainingTime() const;

	UFUNCTION(BlueprintPure, Category = "Card|Cycle")
	float GetCycleProgress() const;

	/** Authoritative Server Getter for Exhaust Pile */
	const TArray<FName>& GetServerExhaustPile() const { return ServerExhaustPile; }
	const TArray<FName>& GetServerDrawPile() const { return ServerDrawPile; }
	const TArray<FName>& GetServerDiscardPile() const { return ServerDiscardPile; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Rules")
	void SetMaxHandSize(int32 InMaxHandSize) { MaxHandSize = InMaxHandSize; }

	// --- Client RPCs ---
	UFUNCTION(Client, Reliable, Category = "Card|Network")
	void Client_OnCycleTriggered();

	// --- Server RPCs ---
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Card|Network")
	void Server_PlayCard(const FGuid& CardGuid, const FFCCardTargetInfo& TargetInfo);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Card|Network")
	void Server_DrawCards(int32 Count);

	// --- Server Authority APIs ---
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	void InitializeDeck(const TArray<FName>& StartingDeck);

	/** Initializes deck using starter deck configured for the given character class */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	void InitializeDeckForClass(EFCCharacterClass InClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	void DrawCards(int32 Count = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	bool DiscardCard(const FGuid& CardGuid);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	bool ExhaustCard(const FGuid& CardGuid);

	/** Retrieves a card from the exhaust pile and places it into the target destination */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	bool RetrieveCardFromExhaust(FName CardId, EFCCardAddDestination Destination = EFCCardAddDestination::Hand);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	bool UpgradeCardInHand(const FGuid& CardGuid, int32 NewUpgradeLevel = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	bool SetCardLockedInHand(const FGuid& CardGuid, bool bLocked);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	void ReshuffleDiscardIntoDraw();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	void DiscardEntireHand();

	/** Dynamically adds a card to the deck, discard pile, hand, or exhaust pile */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Authority")
	bool AddCardToDeck(FName CardId, EFCCardAddDestination Destination = EFCCardAddDestination::DiscardPile, bool bShuffleIfDrawPile = true);

	// --- Periodic Hand & Mana Cycle APIs ---
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Cycle")
	void SetCycleInterval(float InInterval);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Cycle")
	void SetCycleDrawCount(int32 InDrawCount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Cycle")
	void SetAutoCycleEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Cycle")
	void StartCycleTimer();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Cycle")
	void StopCycleTimer();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Cycle")
	void ResetCycleTimer();

	/**
	 * Server-authoritative turn cycle:
	 * 1. Discards entire hand (Retain cards preserved, Ethereal cards exhausted, rest discarded)
	 * 2. Refreshes owner's mana to MaxMana
	 * 3. Draws CycleDrawCount cards from deck
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Cycle")
	void ExecuteHandCycle();

	/** Server-authoritative mana refresh to MaxMana */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Cycle")
	void RefreshOwnerMana();

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	/** Maximum number of cards allowed in hand (default: 10) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Rules", meta = (ClampMin = "1"))
	int32 MaxHandSize = 10;

	/** Cycle duration in seconds between periodic hand refreshes (default: 30.0s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CycleSettings, Category = "Card|Cycle", meta = (ClampMin = "1.0"))
	float CycleInterval = 30.0f;

	/** Number of cards drawn from draw pile on each cycle (default: 5) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CycleSettings, Category = "Card|Cycle", meta = (ClampMin = "0"))
	int32 CycleDrawCount = 5;

	/** Whether periodic hand cycle timer is running automatically */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CycleSettings, Category = "Card|Cycle")
	bool bAutoCycleEnabled = true;

	/** World timestamp when current cycle finishes (COND_OwnerOnly, used for zero-tick client countdowns) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CycleEndTime, Category = "Card|Cycle")
	float CycleEndTime = 0.0f;

	UFUNCTION()
	virtual void OnRep_HandContainer();

	UFUNCTION()
	virtual void OnRep_PileCounts();

	UFUNCTION()
	virtual void OnRep_CycleSettings();

	UFUNCTION()
	virtual void OnRep_CycleEndTime();

private:
	// Server-Only Hidden Piles (Preventing Client Fog-of-War Memory Cheats)
	TArray<FName> ServerDrawPile;
	TArray<FName> ServerDiscardPile;
	TArray<FName> ServerExhaustPile;

	FTimerHandle CycleTimerHandle;

	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
};
