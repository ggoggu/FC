#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Data/Class/FCClassTypes.h"
#include "FCElementStackItemViewModel.generated.h"

class UTexture2D;

/**
 * UFCElementStackItemViewModel
 * 
 * Presentation Sub-ViewModel for an individual element stack token (e.g. Fire, Water, Earth).
 * Bound to individual stack slot widgets via UE5 MVVM FieldNotify.
 */
UCLASS(BlueprintType)
class FC_API UFCElementStackItemViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element")
	EFCElement Element = EFCElement::None;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element")
	int32 SlotIndex = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element")
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element")
	FLinearColor ElementColor = FLinearColor::White;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Element")
	TSoftObjectPtr<UTexture2D> ElementIcon;

public:
	// Setters utilizing UE_MVVM_SET_PROPERTY_VALUE
	void SetElement(EFCElement InElement);
	void SetSlotIndex(int32 InIndex) { UE_MVVM_SET_PROPERTY_VALUE(SlotIndex, InIndex); }
	void SetDisplayName(const FText& InName) { UE_MVVM_SET_PROPERTY_VALUE(DisplayName, InName); }
	void SetElementColor(const FLinearColor& InColor) { UE_MVVM_SET_PROPERTY_VALUE(ElementColor, InColor); }
	void SetElementIcon(TSoftObjectPtr<UTexture2D> InIcon) { UE_MVVM_SET_PROPERTY_VALUE(ElementIcon, InIcon); }

	/** Convenience initialization helper */
	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	void InitializeFromElement(EFCElement InElement, int32 InSlotIndex);

	// Static UI styling and formatting helpers
	UFUNCTION(BlueprintPure, Category = "FC|Element")
	static FLinearColor GetColorForElement(EFCElement InElement);

	UFUNCTION(BlueprintPure, Category = "FC|Element")
	static FText GetDisplayNameForElement(EFCElement InElement);

	UFUNCTION(BlueprintPure, Category = "FC|Element")
	static FString GetShortNameForElement(EFCElement InElement);
};
