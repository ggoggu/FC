#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "FCCharacterBase.generated.h"

class UAbilitySystemComponent;
class UFCAttributeSet;
class UFCElementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFCOnCharacterDeathSignature, AActor*, DeadActor, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FFCOnCharacterDamageTakenSignature, AActor*, DamagedActor, float, DamageAmount, AActor*, DamageCauser, const FHitResult&, HitResult);

UENUM(BlueprintType)
enum class EFCDeathDirection : uint8
{
	Front UMETA(DisplayName = "Front"),
	Back UMETA(DisplayName = "Back"),
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right")
};

UCLASS()
class FC_API AFCCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AFCCharacterBase();

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

protected:
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead, VisibleAnywhere, BlueprintReadOnly, Category = "FC|Combat")
	bool bIsDead = false;

	UFUNCTION()
	virtual void OnRep_IsDead();

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
};

