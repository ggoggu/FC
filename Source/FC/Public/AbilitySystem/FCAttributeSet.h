#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Data/FCPlayerPersistenceTypes.h"
#include "FCAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class FC_API UFCAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UFCAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	/** Exports current health/mana attribute snapshot for persistence */
	UFUNCTION(BlueprintCallable, Category = "Attributes|Persistence")
	FFCPlayerStatSaveData ExportStatSaveData() const;

	/** Restores health/mana attribute snapshot from persistence */
	UFUNCTION(BlueprintCallable, Category = "Attributes|Persistence")
	void RestoreFromStatSaveData(const FFCPlayerStatSaveData& InStatData);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes|Health")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UFCAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Attributes|Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UFCAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "Attributes|Mana")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UFCAttributeSet, Mana)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "Attributes|Mana")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UFCAttributeSet, MaxMana)

protected:
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	virtual void OnRep_Mana(const FGameplayAttributeData& OldMana);

	UFUNCTION()
	virtual void OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana);
};
