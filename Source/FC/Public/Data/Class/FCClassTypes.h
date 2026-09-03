#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FCClassTypes.generated.h"

class UTexture2D;

/**
 * Character Class / Job Types
 */
UENUM(BlueprintType)
enum class EFCCharacterClass : uint8
{
	Neutral     UMETA(DisplayName = "Neutral / 중립"),
	Mage        UMETA(DisplayName = "Mage / 마법사"),
	Warrior     UMETA(DisplayName = "Warrior / 전사"),
	Rogue       UMETA(DisplayName = "Rogue / 도적"),
	Priest      UMETA(DisplayName = "Priest / 사제")
};

/**
 * Element / School / Affinity Types (Mage Specialization)
 */
UENUM(BlueprintType)
enum class EFCElement : uint8
{
	None        UMETA(DisplayName = "None / 무속성"),
	Fire        UMETA(DisplayName = "Fire / 화염"),
	Earth       UMETA(DisplayName = "Earth / 대지"),
	Water       UMETA(DisplayName = "Water / 빙결/물"),
	Wind        UMETA(DisplayName = "Wind / 바람"),
	Lightning   UMETA(DisplayName = "Lightning / 번개"),
	Holy        UMETA(DisplayName = "Holy / 신성"),
	Dark        UMETA(DisplayName = "Dark / 암흑")
};

/**
 * Base stats and configuration for a character class definition
 */
USTRUCT(BlueprintType)
struct FC_API FFCCharacterClassData
{
	GENERATED_BODY()

	/** Class type enum */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class")
	EFCCharacterClass ClassType = EFCCharacterClass::Neutral;

	/** Localized class display name */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class")
	FText ClassDisplayName;

	/** Localized class description */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class")
	FText ClassDescription;

	/** Class artwork or badge icon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class")
	TSoftObjectPtr<UTexture2D> ClassIcon;

	/** Elements naturally affiliated with this class (e.g. Mage -> Fire, Earth, Water, Wind, Lightning) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class")
	TArray<EFCElement> AffinityElements;

	/** Default starter deck (CardIds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class")
	TArray<FName> StartingDeck;

	/** Base Health attribute */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class|Stats", meta = (ClampMin = "1.0"))
	float BaseMaxHealth = 100.0f;

	/** Base Mana attribute */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class|Stats", meta = (ClampMin = "0.0"))
	float BaseMaxMana = 50.0f;

	/** Base Attack Power attribute */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class|Stats")
	float BaseAttackPower = 0.0f;

	/** Gameplay tags associated with this class (e.g. Class.Mage) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class")
	FGameplayTagContainer ClassTags;
};

/**
 * Lightweight static adapter utility for formatting and querying class traits
 */
struct FC_API FCClassTraitUtils
{
	/** Returns localized name of the class */
	static FText GetClassDisplayName(EFCCharacterClass InClass)
	{
		switch (InClass)
		{
		case EFCCharacterClass::Neutral:
			return NSLOCTEXT("FCClass", "ClassNeutral", "중립");
		case EFCCharacterClass::Mage:
			return NSLOCTEXT("FCClass", "ClassMage", "마법사");
		case EFCCharacterClass::Warrior:
			return NSLOCTEXT("FCClass", "ClassWarrior", "전사");
		case EFCCharacterClass::Rogue:
			return NSLOCTEXT("FCClass", "ClassRogue", "도적");
		case EFCCharacterClass::Priest:
			return NSLOCTEXT("FCClass", "ClassPriest", "사제");
		default:
			return FText::GetEmpty();
		}
	}

	/** Returns localized name of the element */
	static FText GetElementDisplayName(EFCElement InElement)
	{
		switch (InElement)
		{
		case EFCElement::Fire:
			return NSLOCTEXT("FCElement", "ElementFire", "화염");
		case EFCElement::Earth:
			return NSLOCTEXT("FCElement", "ElementEarth", "대지");
		case EFCElement::Water:
			return NSLOCTEXT("FCElement", "ElementWater", "빙결/물");
		case EFCElement::Wind:
			return NSLOCTEXT("FCElement", "ElementWind", "바람");
		case EFCElement::Lightning:
			return NSLOCTEXT("FCElement", "ElementLightning", "번개");
		case EFCElement::Holy:
			return NSLOCTEXT("FCElement", "ElementHoly", "신성");
		case EFCElement::Dark:
			return NSLOCTEXT("FCElement", "ElementDark", "암흑");
		case EFCElement::None:
		default:
			return NSLOCTEXT("FCElement", "ElementNone", "무속성");
		}
	}

	/** Returns the trait category name for a given class (e.g. Mage -> "속성", Warrior -> "태세") */
	static FText GetTraitCategoryName(EFCCharacterClass InClass)
	{
		switch (InClass)
		{
		case EFCCharacterClass::Mage:
			return NSLOCTEXT("FCClassTrait", "TraitCategoryMage", "속성");
		case EFCCharacterClass::Warrior:
			return NSLOCTEXT("FCClassTrait", "TraitCategoryWarrior", "태세");
		case EFCCharacterClass::Rogue:
			return NSLOCTEXT("FCClassTrait", "TraitCategoryRogue", "연계");
		case EFCCharacterClass::Priest:
			return NSLOCTEXT("FCClassTrait", "TraitCategoryPriest", "권능");
		case EFCCharacterClass::Neutral:
		default:
			return NSLOCTEXT("FCClassTrait", "TraitCategoryNeutral", "공용");
		}
	}

	/** Formats the elements of a Mage card into a joined localized string (e.g. "화염 / 대지") */
	static FText FormatMageElementsText(const TArray<EFCElement>& Elements)
	{
		TArray<FString> ElementStrings;
		for (EFCElement Elem : Elements)
		{
			if (Elem != EFCElement::None)
			{
				ElementStrings.Add(GetElementDisplayName(Elem).ToString());
			}
		}

		if (ElementStrings.Num() == 0)
		{
			if (Elements.Contains(EFCElement::None))
			{
				return GetElementDisplayName(EFCElement::None);
			}
			return FText::GetEmpty();
		}

		return FText::FromString(FString::Join(ElementStrings, TEXT(" / ")));
	}

	/** Checks whether a card of CardClass is usable by a player of PlayerClass (Neutral cards can be used by all) */
	static bool CanCardBeUsedByClass(EFCCharacterClass CardClass, EFCCharacterClass PlayerClass)
	{
		if (CardClass == EFCCharacterClass::Neutral)
		{
			return true;
		}
		return CardClass == PlayerClass;
	}

	/** Returns true if this class supports elemental affinities (currently Mage only) */
	static bool SupportsElementalAffinities(EFCCharacterClass InClass)
	{
		return InClass == EFCCharacterClass::Mage;
	}
};
