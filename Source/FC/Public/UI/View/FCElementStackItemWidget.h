#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Class/FCClassTypes.h"
#include "FCElementStackItemWidget.generated.h"

class UImage;
class UBorder;
class UTextBlock;
class UTexture2D;

/**
 * UFCElementStackItemWidget
 * 
 * Lightweight UMG UserWidget representing an individual element stack token or gem.
 * Reusable and pooled by UFCElementOverheadWidget without ViewModel allocations.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class FC_API UFCElementStackItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFCElementStackItemWidget(const FObjectInitializer& ObjectInitializer);

	/** Sets the active element and updates visual presentation (Color, Initial, Icon) */
	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	virtual void SetElement(EFCElement InElement, int32 InSlotIndex = 0);

	/** Resets this item widget to empty/none state */
	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	virtual void ClearElement();

	UFUNCTION(BlueprintPure, Category = "FC|Element")
	EFCElement GetCurrentElement() const { return CurrentElement; }

	UFUNCTION(BlueprintPure, Category = "FC|Element")
	int32 GetCurrentSlotIndex() const { return CurrentSlotIndex; }

	// Static UI styling and formatting helpers
	UFUNCTION(BlueprintPure, Category = "FC|Element")
	static FLinearColor GetColorForElement(EFCElement InElement);

	UFUNCTION(BlueprintPure, Category = "FC|Element")
	static FText GetDisplayNameForElement(EFCElement InElement);

	UFUNCTION(BlueprintPure, Category = "FC|Element")
	static FString GetShortNameForElement(EFCElement InElement);

protected:
	virtual void NativeConstruct() override;

	/** Blueprint event hook triggered whenever element is set or updated */
	UFUNCTION(BlueprintImplementableEvent, Category = "FC|Element|Events")
	void OnElementUpdated(EFCElement NewElement, int32 SlotIndex);

	/** Blueprint event hook triggered when stack added animation should play */
	UFUNCTION(BlueprintImplementableEvent, Category = "FC|Element|Events")
	void OnPlayStackAddedAnimation();

	UPROPERTY(BlueprintReadOnly, Category = "FC|Element")
	EFCElement CurrentElement = EFCElement::None;

	UPROPERTY(BlueprintReadOnly, Category = "FC|Element")
	int32 CurrentSlotIndex = 0;

	/** Optional element icon image */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|Element|View")
	TObjectPtr<UImage> Img_ElementIcon;

	/** Optional colored background or ring border */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|Element|View")
	TObjectPtr<UBorder> Border_Background;

	/** Optional short text initial (e.g. F, E, W) displayed when icon is absent */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "FC|Element|View")
	TObjectPtr<UTextBlock> Text_ElementInitial;
};
