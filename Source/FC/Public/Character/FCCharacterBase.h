#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagContainer.h"
#include "Combat/Interfaces/FCCombatTargetInterface.h"
#include "FCCharacterBase.generated.h"

class UAbilitySystemComponent;
class UFCAttributeSet;
class UFCElementComponent;
class UAnimSequence;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFCOnCharacterDeathSignature, AActor*, DeadActor, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FFCOnCharacterDamageTakenSignature, AActor*, DamagedActor, float, DamageAmount, AActor*, DamageCauser, const FHitResult&, HitResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFCOnCharacterHealedSignature, AActor*, HealedActor, float, HealAmount);

UENUM(BlueprintType)
enum class EFCDeathDirection : uint8
{
	Front UMETA(DisplayName = "Front"),
	Back UMETA(DisplayName = "Back"),
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right")
};

UCLASS()
class FC_API AFCCharacterBase : public ACharacter, public IAbilitySystemInterface, public IGameplayCueInterface, public IFCCombatTargetInterface
{
	GENERATED_BODY()

public:
	AFCCharacterBase();

	// --- IFCCombatTargetInterface ---
	virtual FGameplayTag GetCombatTeamTag() const override;
	virtual bool IsCombatTargetAlive() const override { return !bIsDead; }
	virtual bool CanBeTargetedBy(const AActor* SourceActor) const override;
	virtual FVector GetCombatAimLocation() const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFCAttributeSet* GetAttributeSet() const;

	UFUNCTION(BlueprintPure, Category = "Element")
	UFCElementComponent* GetElementComponent() const;

	/** Trigger death lifecycle, disable collision/movement, and broadcast death delegate */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat")
	virtual void Die(AActor* Killer = nullptr);

	/** Called when the character takes damage (executed on authoritative server) */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat")
	virtual void HandleDamageTaken(float DamageAmount, AActor* DamageCauser, const FHitResult& HitResult);

	/** Called to restore character health on authoritative server (clamped to MaxHealth) */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat")
	virtual void ApplyHeal(float HealAmount);

	/** Calculates relative hit direction from an instigator actor (Front, Back, Left, Right) */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat")
	EFCDeathDirection CalculateHitDirection(AActor* InstigatorActor) const;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "FC|Combat")
	bool IsDead() const { return bIsDead; }

	/** Multicast delegate fired when character dies */
	UPROPERTY(BlueprintAssignable, Category = "FC|Combat")
	FFCOnCharacterDeathSignature OnDeath;

	/** Multicast delegate fired when character takes damage */
	UPROPERTY(BlueprintAssignable, Category = "FC|Combat")
	FFCOnCharacterDamageTakenSignature OnDamageTaken;

	/** Multicast delegate fired when character is healed */
	UPROPERTY(BlueprintAssignable, Category = "FC|Combat")
	FFCOnCharacterHealedSignature OnHealed;

	/** Sets the active combat target actor (used for pitch/yaw projectile aiming) */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat")
	virtual void SetCombatTarget(AActor* InTarget);

	UFUNCTION(BlueprintPure, Category = "FC|Combat")
	virtual AActor* GetCombatTarget() const;

	/** Sets the world location target for aiming when no specific actor is targeted */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat")
	virtual void SetTargetAimLocation(const FVector& InLocation);

	UFUNCTION(BlueprintPure, Category = "FC|Combat")
	virtual FVector GetTargetAimLocation() const;

	/** Immediately rotates character Yaw to face the target world position */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat")
	virtual void RotateTowardsTarget(const FVector& InTargetLocation);

	/** Directional death animation trigger (shared across Player and Mob subclasses) */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat")
	virtual void PlayDeathAnimation(EFCDeathDirection Direction);

	UFUNCTION(BlueprintPure, Category = "FC|Combat")
	UAnimSequence* GetDeathAnimationForDirection(EFCDeathDirection Direction) const;

	/** Directional hit reaction animation trigger (shared across Player and Mob subclasses) */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat")
	virtual void PlayHitAnimation(EFCDeathDirection Direction);

	UFUNCTION(BlueprintPure, Category = "FC|Combat")
	UAnimSequence* GetHitAnimationForDirection(EFCDeathDirection Direction) const;

	/** IGameplayCueInterface override to handle GameplayCue events (e.g. HitReaction) */
	virtual void HandleGameplayCue(UObject* Self, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead, VisibleAnywhere, BlueprintReadOnly, Category = "FC|Combat")
	bool bIsDead = false;

	UPROPERTY(ReplicatedUsing = OnRep_DeathDirection, VisibleAnywhere, BlueprintReadOnly, Category = "FC|Combat")
	EFCDeathDirection DeathDirection = EFCDeathDirection::Front;

	UFUNCTION()
	virtual void OnRep_IsDead();

	UFUNCTION()
	virtual void OnRep_DeathDirection();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFCAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Element", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFCElementComponent> ElementComponent;

	/** Transient reference to active combat target for 3D aim direction calculation */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CombatTarget;

	/** World location for aiming when no specific actor is targeted */
	UPROPERTY(Transient)
	FVector TargetAimLocation = FVector::ZeroVector;

	// --- Shared Directional Death & Hit Reaction Animations (Hoisted SSOT) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Front;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Front_02;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Front_03;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Back;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Animation")
	TObjectPtr<UAnimSequence> DeathAnim_Right;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Animation")
	TObjectPtr<UAnimMontage> HitMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Animation")
	TObjectPtr<UAnimSequence> HitAnim_Front;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Animation")
	TObjectPtr<UAnimSequence> HitAnim_Back;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Animation")
	TObjectPtr<UAnimSequence> HitAnim_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Animation")
	TObjectPtr<UAnimSequence> HitAnim_Right;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Animation", meta = (ClampMin = "0.1"))
	float HitPlayRate = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|Combat", meta = (ClampMin = "0.0"))
	float HitReactionCooldown = 0.25f;

	float LastHitReactTime = -100.0f;
};

