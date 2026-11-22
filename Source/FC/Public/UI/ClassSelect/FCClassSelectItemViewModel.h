#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "GameplayTagContainer.h"
#include "Data/Class/FCClassTypes.h"
#include "FCClassSelectItemViewModel.generated.h"

class UTexture2D;

/**
 * UFCClassSelectItemViewModel
 * 
 * Presentation ViewModel for an individual class item/card in the Class Selection UI.
 * Driven purely by FFCCharacterClassData from DT_ClassCatalog (zero hardcoded class switches/locks).
 */
UCLASS(BlueprintType)
class FC_API UFCClassSelectItemViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	// --- Identity & Info ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect|Item")
	FGameplayTag ClassTag;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect|Item")
	FText ClassDisplayName;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect|Item")
	FText ClassDescription;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect|Item")
	TSoftObjectPtr<UTexture2D> ClassIcon;

	// --- Combat Base Stats ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect|Item")
	float BaseMaxHealth = 100.0f;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect|Item")
	float BaseMaxMana = 50.0f;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect|Item")
	float BaseAttackPower = 0.0f;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect|Item")
	FText AffinityElementsText;

	// --- State & Selectability ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect|Item")
	bool bIsSelectable = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect|Item")
	bool bIsSelected = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|ClassSelect|Item")
	FText LockReasonText;

public:
	/** Populates ViewModel fields from DataTable class definition */
	void InitializeFromClassData(const FFCCharacterClassData& InData, const UObject* WorldContextObject = nullptr);

	// Setters using UE_MVVM_SET_PROPERTY_VALUE
	void SetClassTag(FGameplayTag InTag) { UE_MVVM_SET_PROPERTY_VALUE(ClassTag, InTag); }
	void SetClassDisplayName(const FText& InName) { UE_MVVM_SET_PROPERTY_VALUE(ClassDisplayName, InName); }
	void SetClassDescription(const FText& InDesc) { UE_MVVM_SET_PROPERTY_VALUE(ClassDescription, InDesc); }
	void SetClassIcon(TSoftObjectPtr<UTexture2D> InIcon) { UE_MVVM_SET_PROPERTY_VALUE(ClassIcon, InIcon); }
	void SetBaseMaxHealth(float InHealth) { UE_MVVM_SET_PROPERTY_VALUE(BaseMaxHealth, InHealth); }
	void SetBaseMaxMana(float InMana) { UE_MVVM_SET_PROPERTY_VALUE(BaseMaxMana, InMana); }
	void SetBaseAttackPower(float InAP) { UE_MVVM_SET_PROPERTY_VALUE(BaseAttackPower, InAP); }
	void SetAffinityElementsText(const FText& InText) { UE_MVVM_SET_PROPERTY_VALUE(AffinityElementsText, InText); }
	void SetIsSelectable(bool bInSelectable) { UE_MVVM_SET_PROPERTY_VALUE(bIsSelectable, bInSelectable); }
	void SetIsSelected(bool bInSelected) { UE_MVVM_SET_PROPERTY_VALUE(bIsSelected, bInSelected); }
	void SetLockReasonText(const FText& InText) { UE_MVVM_SET_PROPERTY_VALUE(LockReasonText, InText); }

	bool GetIsSelectable() const { return bIsSelectable; }
	bool GetIsSelected() const { return bIsSelected; }
	FGameplayTag GetClassTag() const { return ClassTag; }
};
