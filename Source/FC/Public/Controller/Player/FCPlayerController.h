#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FCPlayerController.generated.h"

class UInputMappingContext;
class UFCHUDWidget;
class UFCHUDViewModel;
class UFCCardDataAsset;
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

	/** Resolves targeting info (target actor, target 3D world location) under the current mouse cursor */
	UFUNCTION(BlueprintCallable, Category = "Card|Targeting")
	FFCCardTargetInfo ResolveCardTargetUnderCursor(const UFCCardDataAsset* CardAsset = nullptr);

	/** Resolves targeting info at a specific 2D viewport screen coordinate */
	UFUNCTION(BlueprintCallable, Category = "Card|Targeting")
	FFCCardTargetInfo ResolveCardTargetAtScreenPosition(const FVector2D& ScreenPos, const UFCCardDataAsset* CardAsset = nullptr);

	/** Handles card slot selection via keyboard number key input (0 = 1st card, ..., 9 = 10th card) */
	UFUNCTION(BlueprintCallable, Category = "Input|CardCombat")
	void HandleNumberKeyInput(int32 SlotIndex);

	/** Handles left mouse button click (plays held card if any) */
	UFUNCTION(BlueprintCallable, Category = "Input|CardCombat")
	void OnLeftMouseButtonPressed();

	/** Handles right mouse button click or cancel key (cancels held card if any) */
	UFUNCTION(BlueprintCallable, Category = "Input|CardCombat")
	void OnRightMouseButtonPressed();

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
	virtual void OnRep_PlayerState() override;

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

	/** Whether to automatically search for and lock onto attackable targets (mobs, chests) for projectile cards */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Card|Targeting")
	bool bAutoTargetAttackablesOnProjectileCards = true;

	/** Maximum search range for projectile card auto-targeting */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Card|Targeting", meta = (ClampMin = "100.0"))
	float ProjectileTargetMaxRange = 3000.0f;

	/** Half-angle in degrees of the frontal targeting cone for projectile cards */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Card|Targeting", meta = (ClampMin = "5.0", ClampMax = "89.0"))
	float ProjectileTargetHalfAngleDegrees = 30.0f;

	/** Proximity radius around cursor drop location to search for attackable objects */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Card|Targeting", meta = (ClampMin = "50.0"))
	float ProjectileTargetProximityRadius = 350.0f;

	UFUNCTION()
	void SyncHandToViewModel();

	UFUNCTION()
	void OnDeckPileCountsChanged(int32 DrawCount, int32 DiscardCount, int32 ExhaustCount);

	UFUNCTION()
	void OnDeckCycleTriggered();

	UFUNCTION()
	void OnDeckCycleSettingsChanged(float NewInterval, int32 NewDrawCount);

private:
	void InitializeDefaultMappingContext();
	void BindDeckComponentEvents();
	void BindAttributeListeners();

	void OnNumberKey1Pressed() { HandleNumberKeyInput(0); }
	void OnNumberKey2Pressed() { HandleNumberKeyInput(1); }
	void OnNumberKey3Pressed() { HandleNumberKeyInput(2); }
	void OnNumberKey4Pressed() { HandleNumberKeyInput(3); }
	void OnNumberKey5Pressed() { HandleNumberKeyInput(4); }
	void OnNumberKey6Pressed() { HandleNumberKeyInput(5); }
	void OnNumberKey7Pressed() { HandleNumberKeyInput(6); }
	void OnNumberKey8Pressed() { HandleNumberKeyInput(7); }
	void OnNumberKey9Pressed() { HandleNumberKeyInput(8); }
	void OnNumberKey0Pressed() { HandleNumberKeyInput(9); }
};
