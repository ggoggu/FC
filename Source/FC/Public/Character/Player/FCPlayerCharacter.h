#pragma once

#include "CoreMinimal.h"
#include "Character/FCCharacterBase.h"
#include "FCPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAnimSequence;
class UAnimMontage;
struct FInputActionValue;

UCLASS()
class FC_API AFCPlayerCharacter : public AFCCharacterBase
{
	GENERATED_BODY()

public:
	AFCPlayerCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	virtual void Die(AActor* Killer = nullptr) override;
	virtual void HandleDamageTaken(float DamageAmount, AActor* DamageCauser, const FHitResult& HitResult) override;

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
};
