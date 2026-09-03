#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Data/Card/FCCardTypes.h"
#include "FCCardViewModel.generated.h"

struct FFCCardItem;
class UFCCardDataAsset;
class UTexture2D;

/**
 * UFCCardViewModel
 * 
 * Presentation ViewModel for a single card in UI/Hand.
 * Exposes FieldNotify properties and computed getters for zero-tick UMG bindings.
 */
UCLASS(BlueprintType)
class FC_API UFCCardViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	// --- Identifiers & Dynamic State ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	FGuid CardGuid;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	FName CardId = NAME_None;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	int32 UpgradeLevel = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	bool bIsUpgraded = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	bool bIsLocked = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	int32 ManaCost = 1;

	// --- Visual & Display Properties ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	FText CardName;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	FText CardDescription;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	TSoftObjectPtr<UTexture2D> CardIcon;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	EFCCardType CardType = EFCCardType::Attack;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	EFCCardRarity Rarity = EFCCardRarity::Common;

	// --- Class & Trait Properties ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	EFCCharacterClass RequiredClass = EFCCharacterClass::Neutral;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	TArray<EFCElement> Elements;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	bool bHasClassTrait = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	bool bIsNeutral = true;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	FText FormattedClassText;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	FText ClassTraitTypeName;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	FText ClassTraitFormattedText;

	// --- Keyword & Exhaust Properties ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card|Keyword")
	bool bExhaustsOnPlay = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card|Keyword")
	FText FormattedKeywords;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card|Keyword")
	bool bHasKeywords = false;

	// --- Interaction State ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	bool bIsPlayable = true;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	bool bIsSelected = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card")
	bool bIsHovered = false;

	// --- Hand Fan Layout Presentation State ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card|Layout")
	int32 HandIndex = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card|Layout")
	int32 TotalCardsInHand = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card|Layout")
	float TargetFanAngle = 0.0f;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Card|Layout")
	FVector2D TargetFanOffset = FVector2D::ZeroVector;

public:
	// Property Setters utilizing UE_MVVM_SET_PROPERTY_VALUE
	void SetCardGuid(const FGuid& InGuid) { UE_MVVM_SET_PROPERTY_VALUE(CardGuid, InGuid); }
	void SetCardId(FName InId) { UE_MVVM_SET_PROPERTY_VALUE(CardId, InId); }
	void SetUpgradeLevel(int32 InLevel);
	void SetIsLocked(bool bInLocked);
	void SetManaCost(int32 InCost);
	void SetCardName(const FText& InName);
	void SetCardDescription(const FText& InDescription) { UE_MVVM_SET_PROPERTY_VALUE(CardDescription, InDescription); }
	void SetCardIcon(TSoftObjectPtr<UTexture2D> InIcon) { UE_MVVM_SET_PROPERTY_VALUE(CardIcon, InIcon); }
	void SetCardType(EFCCardType InType) { UE_MVVM_SET_PROPERTY_VALUE(CardType, InType); }
	void SetRarity(EFCCardRarity InRarity) { UE_MVVM_SET_PROPERTY_VALUE(Rarity, InRarity); }
	void SetRequiredClass(EFCCharacterClass InClass) { UE_MVVM_SET_PROPERTY_VALUE(RequiredClass, InClass); }
	void SetElements(const TArray<EFCElement>& InElements) { UE_MVVM_SET_PROPERTY_VALUE(Elements, InElements); }
	void SetHasClassTrait(bool bInHasTrait) { UE_MVVM_SET_PROPERTY_VALUE(bHasClassTrait, bInHasTrait); }
	void SetIsNeutral(bool bInNeutral) { UE_MVVM_SET_PROPERTY_VALUE(bIsNeutral, bInNeutral); }
	void SetFormattedClassText(const FText& InText) { UE_MVVM_SET_PROPERTY_VALUE(FormattedClassText, InText); }
	void SetClassTraitTypeName(const FText& InText) { UE_MVVM_SET_PROPERTY_VALUE(ClassTraitTypeName, InText); }
	void SetClassTraitFormattedText(const FText& InText) { UE_MVVM_SET_PROPERTY_VALUE(ClassTraitFormattedText, InText); }
	void SetExhaustsOnPlay(bool bInExhaust) { UE_MVVM_SET_PROPERTY_VALUE(bExhaustsOnPlay, bInExhaust); }
	void SetFormattedKeywords(const FText& InKeywords) { UE_MVVM_SET_PROPERTY_VALUE(FormattedKeywords, InKeywords); }
	void SetHasKeywords(bool bInHasKeywords) { UE_MVVM_SET_PROPERTY_VALUE(bHasKeywords, bInHasKeywords); }
	void SetIsPlayable(bool bInPlayable);
	void SetIsSelected(bool bInSelected) { UE_MVVM_SET_PROPERTY_VALUE(bIsSelected, bInSelected); }
	void SetIsHovered(bool bInHovered) { UE_MVVM_SET_PROPERTY_VALUE(bIsHovered, bInHovered); }
	void SetHandIndex(int32 InIndex) { UE_MVVM_SET_PROPERTY_VALUE(HandIndex, InIndex); }
	void SetTotalCardsInHand(int32 InTotal) { UE_MVVM_SET_PROPERTY_VALUE(TotalCardsInHand, InTotal); }
	void SetTargetFanAngle(float InAngle) { UE_MVVM_SET_PROPERTY_VALUE(TargetFanAngle, InAngle); }
	void SetTargetFanOffset(const FVector2D& InOffset) { UE_MVVM_SET_PROPERTY_VALUE(TargetFanOffset, InOffset); }

	/** Initializes or synchronizes the ViewModel from runtime item and static definition */
	void InitializeFromCardItem(const FFCCardItem& InItem, const UFCCardDataAsset* InDataAsset);

	// --- Computed FieldNotify Getters ---
	UFUNCTION(BlueprintPure, FieldNotify)
	FText GetFormattedDisplayName() const;

	UFUNCTION(BlueprintPure, FieldNotify)
	bool GetCanPlay() const;
};
