#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "FCCharacterBase.generated.h"

class UAbilitySystemComponent;
class UFCAttributeSet;
class UFCElementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFCOnCharacterDeathSignature, AActor*, DeadActor, AActor*, Killer);

UCLASS()
class FC_API AFCCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AFCCharacterBase();

	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFCAttributeSet* GetAttributeSet() const;

	UFUNCTION(BlueprintPure, Category = "Element")
	UFCElementComponent* GetElementComponent() const;

	/** Trigger death lifecycle, disable collision/movement, and broadcast death delegate */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat")
	virtual void Die(AActor* Killer = nullptr);

	UFUNCTION(BlueprintPure, Category = "FC|Combat")
	bool IsDead() const { return bIsDead; }

	/** Multicast delegate fired when character dies */
	UPROPERTY(BlueprintAssignable, Category = "FC|Combat")
	FFCOnCharacterDeathSignature OnDeath;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Combat")
	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFCAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Element", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFCElementComponent> ElementComponent;
};

