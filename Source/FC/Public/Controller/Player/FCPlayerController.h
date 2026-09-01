#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FCPlayerController.generated.h"

class UInputMappingContext;
class UFCHUDWidget;
class UFCHUDViewModel;
struct FFCCardTargetInfo;

/**
 * AFCPlayerController
 * 
 * Manages player-specific input context mappings, UI interaction states,
 * automated HUD widget lifecycle, and card gameplay requests.
 */
UCLASS()
class FC_API AFCPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFCPlayerController();

	/** Switches input mapping contexts dynamically (e.g. between Exploration and Card Combat) */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SwitchMappingContext(UInputMappingContext* NewContext, int32 Priority = 0);

	/** Configures input mode and mouse cursor for Card Combat or UI interaction */
	UFUNCTION(BlueprintCallable, Category = "Input|CardCombat")
	void SetCardCombatInputMode(bool bEnableCardMode);

	/** Submits a request to play a card through the authoritative DeckComponent */
	UFUNCTION(BlueprintCallable, Category = "Card|Actions")
	void RequestPlayCard(const FGuid& CardGuid, const FFCCardTargetInfo& TargetInfo);

	/** Initializes or binds the HUD with the current player state and deck component */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetupHUD();

	/** Returns the spawned active HUD widget */
	UFUNCTION(BlueprintPure, Category = "UI")
	UFCHUDWidget* GetHUDWidget() const { return HUDWidget; }

	/** Returns the active top-level HUD presentation ViewModel */
	UFUNCTION(BlueprintPure, Category = "UI")
	UFCHUDViewModel* GetHUDViewModel() const { return HUDViewModel; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* P) override;

	/** Widget class to spawn for HUD (e.g. WBP_HUD) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UFCHUDWidget> HUDWidgetClass;

	/** Automatically switch to card combat input mode with mouse cursor when HUD spawns */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	bool bAutoEnableCardCombatInputMode = true;

	/** Active HUD widget instance */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UFCHUDWidget> HUDWidget;

	/** Active top-level HUD presentation ViewModel instance */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UFCHUDViewModel> HUDViewModel;

	/** Default Input Mapping Context (e.g. Exploration / Base Movement) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Priority for the default mapping context */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	int32 DefaultMappingPriority = 0;

	/** Optional Input Mapping Context for Card Battle / UI Mode */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|CardCombat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> CardCombatMappingContext;

	/** Priority for the card combat mapping context */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|CardCombat", meta = (AllowPrivateAccess = "true"))
	int32 CardCombatMappingPriority = 1;

	UFUNCTION()
	void SyncHandToViewModel();

	UFUNCTION()
	void OnDeckPileCountsChanged(int32 DrawCount, int32 DiscardCount, int32 ExhaustCount);

private:
	void InitializeDefaultMappingContext();
	void BindDeckComponentEvents();
	void BindAttributeListeners();
};
