#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Data/Class/FCClassTypes.h"
#include "FCElementStackItemWidget.generated.h"

class UImage;
class UBorder;
class UTextBlock;
class UTexture2D;

/**
 * UFCElementStackItemWidget
 * 
 * Lightweight UMG UserWidget representing an individual GameplayTag element stack token or gem.
 * Resolves color, short code, display name, and icon dynamically from DT_ElementCatalog (OCP).
 */
UCLASS(BlueprintType, Blueprintable)
class FC_API UFCElementStackItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFCElementStackItemWidget(const FObjectInitializer& ObjectInitializer);

	/** Sets the active element GameplayTag and updates visual presentation (Color, Initial, Icon) */
	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	virtual void SetElement(FGameplayTag InElementTag, int32 InSlotIndex = 0);

	/** Resets this item widget to empty/none state */
	UFUNCTION(BlueprintCallable, Category = "FC|Element")
	virtual void ClearElement();

	UFUNCTION(BlueprintPure, Category = "FC|Element")
	FGameplayTag GetCurrentElement() const { return CurrentElementTag; }

	UFUNCTION(BlueprintPure, Category = "FC|Element")
	int32 GetCurrentSlotIndex() const { return CurrentSlotIndex; }

	// Static UI styling and formatting helpers driven by DT_ElementCatalog
	UFUNCTION(BlueprintPure, Category = "FC|Element", meta = (WorldContext = "WorldContextObject"))
	static FLinearColor GetColorForElement(FGameplayTag InElementTag, const UObject* WorldContextObject = nullptr);

	UFUNCTION(BlueprintPure, Category = "FC|Element", meta = (WorldContext = "WorldContextObject"))
	static FText GetDisplayNameForElement(FGameplayTag InElementTag, const UObject* WorldContextObject = nullptr);

	UFUNCTION(BlueprintPure, Category = "FC|Element", meta = (WorldContext = "WorldContextObject"))
	static FString GetShortNameForElement(FGameplayTag InElementTag, const UObject* WorldContextObject = nullptr);

protected:
	virtual void NativeConstruct() override;

	/** Blueprint event hook triggered whenever element is set or updated */
	UFUNCTION(BlueprintImplementableEvent, Category = "FC|Element|Events")
	void OnElementUpdated(FGameplayTag NewElementTag, int32 SlotIndex);

	/** Blueprint event hook triggered when stack added animation should play */
	UFUNCTION(BlueprintImplementableEvent, Category = "FC|Element|Events")
	void OnPlayStackAddedAnimation();

	UPROPERTY(BlueprintReadOnly, Category = "FC|Element")
	FGameplayTag CurrentElementTag;

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
