#pragma once

#include "CoreMinimal.h"
#include "Character/FCCharacterBase.h"
#include "FCPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
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

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRep_PlayerState() override;

	virtual void Die(AActor* Killer = nullptr) override;
	virtual void HandleDamageTaken(float DamageAmount, AActor* DamageCauser, const FHitResult& HitResult) override;

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

	/** Plays directional death animation on local mesh */
	UFUNCTION(BlueprintCallable, Category = "FC|Player|Combat")
	void PlayDeathAnimation(EFCDeathDirection Direction);

	/** Replicated multicast RPC to play cosmetic death animation across network */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDeathAnimation(EFCDeathDirection Direction);

	UAnimSequence* GetDeathAnimationForDirection(EFCDeathDirection Direction) const;

	/** Plays directional hit reaction animation on local mesh */
	UFUNCTION(BlueprintCallable, Category = "FC|Player|Combat")
	void PlayHitAnimation(EFCDeathDirection Direction);

	/** Replicated multicast RPC to play cosmetic hit animation across network */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayHitAnimation(EFCDeathDirection Direction);

	UAnimSequence* GetHitAnimationForDirection(EFCDeathDirection Direction) const;

protected:
	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo();

	/** Enhanced Input handlers */
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComponent;

	/** Move Input Action (WASD) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	/** Look Input Action (Mouse Look) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	/** Directional death animations from Character/Mannequins/Anims/Death */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Front;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Front_02;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Front_03;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Back;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Right;

	/** Optional hit animation montage override */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|Animation")
	TObjectPtr<UAnimMontage> HitMontage;

	/** Directional hit animations (defaults to animations in Character/Mannequins/Anims/Death) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|Animation")
	TObjectPtr<UAnimSequence> HitAnim_Front;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|Animation")
	TObjectPtr<UAnimSequence> HitAnim_Back;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|Animation")
	TObjectPtr<UAnimSequence> HitAnim_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|Animation")
	TObjectPtr<UAnimSequence> HitAnim_Right;

	/** Play rate for hit animation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|Animation", meta = (ClampMin = "0.1"))
	float HitPlayRate = 1.5f;

	/** Minimum time in seconds between playing hit reaction animations */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Player|Combat", meta = (ClampMin = "0.0"))
	float HitReactionCooldown = 0.25f;

	/** Timestamp of last played hit reaction */
	float LastHitReactTime = -100.0f;

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
