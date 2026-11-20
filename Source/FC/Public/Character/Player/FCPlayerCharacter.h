#pragma once

#include "CoreMinimal.h"
#include "Character/FCCharacterBase.h"
#include "Data/FCGameplayTags.h"
#include "FCPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UFCCameraComponent;
class UFCCameraModeDataAsset;
class UInputAction;
class UAnimSequence;
class UAnimMontage;
class AFCMobCharacter;
struct FInputActionValue;

UCLASS()
class FC_API AFCPlayerCharacter : public AFCCharacterBase
{
	GENERATED_BODY()

public:
	AFCPlayerCharacter();

	virtual FGameplayTag GetCombatTeamTag() const override { return FCGameplayTags::Combat_Team_Player; }

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRep_PlayerState() override;

	virtual void Die(AActor* Killer = nullptr) override;

	virtual void Jump() override;
	virtual void StopJumping() override;

	/** Toggles between 1st Person and Shoulder view */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ToggleCameraView();

	/** Sets active camera mode data asset */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetCameraMode(UFCCameraModeDataAsset* NewMode);

	/** Camera mode component getter */
	UFUNCTION(BlueprintPure, Category = "Camera")
	UFCCameraComponent* GetCameraModeComponent() const { return CameraModeComponent; }

	/** Jump Input Action getter */
	UFUNCTION(BlueprintPure, Category = "FC|Player|Input")
	UInputAction* GetJumpAction() const { return JumpAction; }

	/** Jump Input Action setter */
	UFUNCTION(BlueprintCallable, Category = "FC|Player|Input")
	void SetJumpAction(UInputAction* InAction) { JumpAction = InAction; }

	/** Starts health drain timer loop on authoritative server */
	UFUNCTION(BlueprintCallable, Category = "FC|Player|HealthDrain")
	void StartHealthDrain();

	/** Stops health drain timer loop */
	UFUNCTION(BlueprintCallable, Category = "FC|Player|HealthDrain")
	void StopHealthDrain();

	/** Resets current drain interval to InitialDrainInterval and restarts timer */
	UFUNCTION(BlueprintCallable, Category = "FC|Player|HealthDrain")
	void ResetHealthDrain();

	/** Returns the active health drain interval in seconds */
	UFUNCTION(BlueprintPure, Category = "FC|Player|HealthDrain")
	float GetCurrentDrainInterval() const { return CurrentDrainInterval; }

	/** Called when player kills an enemy mob, restoring health */
	UFUNCTION(BlueprintCallable, Category = "FC|Player|Combat")
	virtual void OnKilledEnemy(AFCMobCharacter* VictimMob);

	/** Amount of health restored when player defeats an enemy mob (default: 5.0) */
	UFUNCTION(BlueprintPure, Category = "FC|Player|Combat")
	float GetHealOnKillAmount() const { return HealOnKillAmount; }

	UFUNCTION(BlueprintCallable, Category = "FC|Player|Combat")
	void SetHealOnKillAmount(float InAmount) { HealOnKillAmount = FMath::Max(0.0f, InAmount); }

protected:
	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo();
	virtual bool CanJumpInternal_Implementation() const override;

	/** Enhanced Input handlers */
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFCCameraComponent> CameraModeComponent;

	/** Move Input Action (WASD) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	/** Look Input Action (Mouse Look) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	/** Jump Input Action (Space) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	/** Toggle Camera View Input Action (V) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ToggleCameraAction;

	/** Handler for health drain timer tick */
	void HandleHealthDrainTick();

	/** Whether health drain over time is enabled */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|HealthDrain")
	bool bEnableHealthDrain = true;

	/** Initial interval in seconds before first health drain tick (default: 10.0s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|HealthDrain", meta = (ClampMin = "0.1"))
	float InitialDrainInterval = 10.0f;

	/** Amount of health drained per tick (default: 1.0) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|HealthDrain", meta = (ClampMin = "0.1"))
	float HealthDrainAmount = 1.0f;

	/** Minimum drain interval in seconds (default: 1.0s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|HealthDrain", meta = (ClampMin = "0.05"))
	float MinDrainInterval = 1.0f;

	/** Amount subtracted from drain interval each tick to gradually accelerate drain rate (default: 0.15s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|HealthDrain", meta = (ClampMin = "0.0"))
	float DrainAccelerationStep = 0.15f;

	/** Multiplicative decay factor applied to drain interval each tick (default: 0.98, set to 1.0 for pure linear) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|HealthDrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DrainDecayMultiplier = 0.98f;

	/** Current drain interval in seconds */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Player|HealthDrain")
	float CurrentDrainInterval = 10.0f;

	/** Base health recovered when defeating an enemy mob (default: 5.0) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|Combat", meta = (ClampMin = "0.0"))
	float HealOnKillAmount = 5.0f;

	/** Timer handle for periodic health drain */
	FTimerHandle HealthDrainTimerHandle;
};
