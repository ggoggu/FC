#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Data/Card/FCCardTypes.h"
#include "FCPlayerController.generated.h"

class UInputMappingContext;
class UFCHUDWidget;
class UFCHUDViewModel;
class UFCCardDataAsset;
class AFCCardPickupActor;
class UFCStatusPresenterComponent;
class UFCHUDPresenterComponent;
class UFCCardTargetingComponent;
struct FFCCardTargetInfo;

/**
 * AFCPlayerController
 * 
 * Thin autonomous client controller orchestrating input contexts, server RPC dispatch,
 * and delegating specialized domains to modular components:
 * - UFCHUDPresenterComponent (HUD & GAS attribute/deck ViewModel bindings)
 * - UFCCardTargetingComponent (screen/raycast targeting via TInstancedStruct policies)
 * - UFCStatusPresenterComponent (DataTable-driven status toggles)
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

	/** Authoritative Server RPC to claim a card pickup actor in the world */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Card|Pickup")
	void Server_ClaimCardPickup(AFCCardPickupActor* PickupActor, EFCCardZone Destination = EFCCardZone::DiscardPile);

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
	UFCHUDWidget* GetHUDWidget() const;

	/** Returns the active top-level HUD presentation ViewModel */
	UFUNCTION(BlueprintPure, Category = "UI")
	UFCHUDViewModel* GetHUDViewModel() const;

	/** Returns the status toggle presentation adapter component */
	UFUNCTION(BlueprintPure, Category = "UI|Status")
	UFCStatusPresenterComponent* GetStatusPresenterComponent() const { return StatusPresenterComponent; }

	/** Returns the modular HUD presenter component */
	UFUNCTION(BlueprintPure, Category = "UI")
	UFCHUDPresenterComponent* GetHUDPresenterComponent() const { return HUDPresenterComponent; }

	/** Returns the modular card targeting component */
	UFUNCTION(BlueprintPure, Category = "Card|Targeting")
	UFCCardTargetingComponent* GetCardTargetingComponent() const { return CardTargetingComponent; }

	/** Adds or updates a circular status toggle in the HUD */
	UFUNCTION(BlueprintCallable, Category = "UI|Status")
	void SetStatusToggle(const struct FFCStatusToggleData& InData);

	/** Removes a status toggle from the HUD by Id */
	UFUNCTION(BlueprintCallable, Category = "UI|Status")
	void RemoveStatusToggle(FName InStatusId);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void AcknowledgePossession(APawn* P) override;
	virtual void OnRep_PlayerState() override;

	/** Widget class to spawn for HUD (e.g. WBP_HUD) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UFCHUDWidget> HUDWidgetClass;

	/** Automatically switch to card combat input mode with mouse cursor when HUD spawns */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	bool bAutoEnableCardCombatInputMode = true;

	/** Active HUD widget instance (mirrored from HUDPresenterComponent for BP access) */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UFCHUDWidget> HUDWidget;

	/** Active top-level HUD presentation ViewModel instance (mirrored from HUDPresenterComponent) */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UFCHUDViewModel> HUDViewModel;

	/** Presentation component translating ASC attributes and tags to status toggles */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Status", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFCStatusPresenterComponent> StatusPresenterComponent;

	/** Presentation component managing HUD lifecycle, Deck events, and GAS attribute bindings */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFCHUDPresenterComponent> HUDPresenterComponent;

	/** Modular client targeting component evaluating card TargetPolicies */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card|Targeting", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFCCardTargetingComponent> CardTargetingComponent;

	/** Default Input Mapping Context (e.g. Exploration / Base Movement) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	int32 DefaultMappingPriority = 0;

	/** Optional Input Mapping Context for Card Battle / UI Mode */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|CardCombat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> CardCombatMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|CardCombat", meta = (AllowPrivateAccess = "true"))
	int32 CardCombatMappingPriority = 1;

	/** Targeting tuning parameters forwarded to CardTargetingComponent */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Card|Targeting")
	bool bAutoTargetAttackablesOnProjectileCards = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Card|Targeting", meta = (ClampMin = "100.0"))
	float ProjectileTargetMaxRange = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Card|Targeting", meta = (ClampMin = "5.0", ClampMax = "89.0"))
	float ProjectileTargetHalfAngleDegrees = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Card|Targeting", meta = (ClampMin = "50.0"))
	float ProjectileTargetProximityRadius = 350.0f;

private:
	void InitializeDefaultMappingContext();

	/** Server-side timestamp for pickup RPC rate-limiting */
	double LastClaimPickupServerTime = -1000.0;

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
