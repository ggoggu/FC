#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Data/Class/Traits/FCClassTraitBehaviorBase.h"
#include "Card/Effects/FCCardEffectBase.h"
#include "FCClassTypes.generated.h"

class UTexture2D;

/**
 * FFCElementDefinitionRow
 *
 * DataTable row defining visual and gameplay metadata for an Element GameplayTag (Element.*).
 * Eliminates hardcoded switch statements across UI widgets, ViewModels, and trait formatters.
 */
USTRUCT(BlueprintType)
struct FC_API FFCElementDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Element")
	FGameplayTag ElementTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Element")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Element")
	FString ShortCode = TEXT("-");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Element")
	FLinearColor Color = FLinearColor(0.5f, 0.5f, 0.5f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Element")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Element")
	FGameplayTag StackAppliedCueTag;
};

/**
 * FFCElementReactionRow
 *
 * DataTable row defining a data-driven elemental reaction rule (e.g. Fire + Water -> Vaporize).
 * Evaluated authoritatively by UFCElementComponent when stacks accumulate, reusing FFCCardEffectBase fragments.
 */
USTRUCT(BlueprintType)
struct FC_API FFCElementReactionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Element|Reaction")
	FName ReactionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Element|Reaction")
	FText DisplayName;

	/** Required element tags (supports duplicates such as {Element.Fire, Element.Fire}) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Element|Reaction")
	TArray<FGameplayTag> RequiredElementTags;

	/** Higher priority rules evaluate first */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Element|Reaction")
	int32 Priority = 0;

	/** Whether trigger elements are atomically consumed when this reaction fires */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Element|Reaction")
	bool bConsumeTriggerElements = true;

	/** Composable effect fragments executed when the reaction triggers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Element|Reaction", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FFCCardEffectBase>> ReactionEffects;
};

/**
 * FFCCharacterClassData
 *
 * Unified DataTable row (DT_ClassCatalog) & DataAsset struct for a character class (Class.*).
 * Embeds a polymorphic TInstancedStruct TraitBehavior for zero-conflict parallel class development.
 */
USTRUCT(BlueprintType)
struct FC_API FFCCharacterClassData : public FTableRowBase
{
	GENERATED_BODY()

	/** Class identity GameplayTag (e.g. Class.Mage, Class.Warrior, Class.Neutral) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class")
	FGameplayTag ClassTag;

	/** Localized class display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class")
	FText ClassDisplayName;

	/** Localized class description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class")
	FText ClassDescription;

	/** Localized trait category label (e.g. 속성, 태세, 연계, 권능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class")
	FText TraitCategoryName;

	/** Class artwork or badge icon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class")
	TSoftObjectPtr<UTexture2D> ClassIcon;

	/** Whether this class is selectable on the Class Selection screen */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class")
	bool bIsSelectable = false;

	/** Optional lock explanation displayed when bIsSelectable is false */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class")
	FText LockReasonText;

	/** Whether this class appears in the playable class selection roster */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class")
	bool bShowInClassSelect = true;

	/** Display order in class select UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class")
	int32 DisplayOrder = 0;

	/** Elements naturally affiliated with this class (Element.*) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class")
	TArray<FGameplayTag> AffinityElementTags;

	/** Default starter deck (CardIds) — single SSOT across GameMode, ClassSelect, and DeckComponent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class")
	TArray<FName> StartingDeck;

	/** Base Health attribute */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class|Stats", meta = (ClampMin = "1.0"))
	float BaseMaxHealth = 100.0f;

	/** Base Mana attribute */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class|Stats", meta = (ClampMin = "0.0"))
	float BaseMaxMana = 50.0f;

	/** Base Attack Power attribute */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class|Stats")
	float BaseAttackPower = 0.0f;

	/** Gameplay tags associated with this class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class")
	FGameplayTagContainer ClassTags;

	/** Polymorphic class trait behavior (e.g. FFCClassTrait_Mage) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Class", meta = (ExcludeBaseStruct))
	TInstancedStruct<FFCClassTraitBehaviorBase> TraitBehavior;
};

/**
 * FCClassTraitUtils
 *
 * Zero-switch static façade delegating all Class & Element metadata queries
 * to UFCClassSubsystem's DataTable-backed registries.
 */
struct FC_API FCClassTraitUtils
{
	static FText GetClassDisplayName(FGameplayTag ClassTag, const UObject* WorldContextObject = nullptr);
	static FText GetElementDisplayName(FGameplayTag ElementTag, const UObject* WorldContextObject = nullptr);
	static FString GetElementShortCode(FGameplayTag ElementTag, const UObject* WorldContextObject = nullptr);
	static FLinearColor GetElementColor(FGameplayTag ElementTag, const UObject* WorldContextObject = nullptr);
	static FText GetTraitCategoryName(FGameplayTag ClassTag, const UObject* WorldContextObject = nullptr);
	static FText FormatElementTagsText(const TArray<FGameplayTag>& ElementTags, const UObject* WorldContextObject = nullptr);
	static bool CanCardBeUsedByClass(FGameplayTag CardClassTag, FGameplayTag PlayerClassTag);
	static bool SupportsElementalAffinities(FGameplayTag ClassTag, const UObject* WorldContextObject = nullptr);
};
