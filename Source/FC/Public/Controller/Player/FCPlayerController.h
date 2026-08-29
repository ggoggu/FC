#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FCPlayerController.generated.h"

class UInputMappingContext;

/**
 * AFCPlayerController
 * 
 * Manages player-specific input context mappings, UI interaction states,
 * and high-level input modes for exploration and card-battling gameplay.
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

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

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

private:
	void InitializeDefaultMappingContext();
};
