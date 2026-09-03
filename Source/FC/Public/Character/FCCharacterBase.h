#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "FCCharacterBase.generated.h"

class UAbilitySystemComponent;
class UFCAttributeSet;
class UFCElementComponent;

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

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFCAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Element", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFCElementComponent> ElementComponent;
};
