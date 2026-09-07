#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/NetSerialization.h"
#include "Data/Class/FCClassTypes.h"
#include "FCCardTypes.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UTexture2D;
class USoundBase;
class UNiagaraSystem;
class AActor;
class UPrimitiveComponent;
class UFCProjectileDataAsset;

/**
 * Card category/type classification
 */
UENUM(BlueprintType)
enum class EFCCardType : uint8
{
	Attack      UMETA(DisplayName = "Attack"),
	Skill       UMETA(DisplayName = "Skill"),
	Power       UMETA(DisplayName = "Power"),
	Item        UMETA(DisplayName = "Item"),
	Curse       UMETA(DisplayName = "Curse")
};

/**
 * Card targeting paradigm
 */
UENUM(BlueprintType)
enum class EFCCardTargetType : uint8
{
	Self            UMETA(DisplayName = "Self"),
	SingleTarget    UMETA(DisplayName = "Single Target"),
	AllEnemies      UMETA(DisplayName = "All Enemies"),
	DirectionalAoE  UMETA(DisplayName = "Directional AoE"),
	None            UMETA(DisplayName = "None / No Target")
};

/**
 * Card rarity tier
 */
UENUM(BlueprintType)
enum class EFCCardRarity : uint8
{
	Common      UMETA(DisplayName = "Common"),
	Uncommon    UMETA(DisplayName = "Uncommon"),
	Rare        UMETA(DisplayName = "Rare"),
	Epic        UMETA(DisplayName = "Epic"),
	Legendary   UMETA(DisplayName = "Legendary")
};

/**
 * Card gameplay keyword classification
 */
UENUM(BlueprintType)
enum class EFCCardKeyword : uint8
{
	None        UMETA(DisplayName = "None"),
	Exhaust     UMETA(DisplayName = "Exhaust / 소멸"),
	Retain      UMETA(DisplayName = "Retain / 보존"),
	Innate      UMETA(DisplayName = "Innate / 선천성"),
	Ethereal    UMETA(DisplayName = "Ethereal / 휘발성"),
	Unplayable  UMETA(DisplayName = "Unplayable / 사용 불가")
};

/**
 * Card zone / combat circulation state
 */
UENUM(BlueprintType)
enum class EFCCardZone : uint8
{
	DrawPile    UMETA(DisplayName = "Draw Pile / Deck"),
	Hand        UMETA(DisplayName = "Hand"),
	DiscardPile UMETA(DisplayName = "Discard Pile / Graveyard"),
	ExhaustPile UMETA(DisplayName = "Exhaust Pile / Banish")
};

/**
 * Card destination when dynamically adding cards into deck
 */
UENUM(BlueprintType)
enum class EFCCardAddDestination : uint8
{
	DrawPile    UMETA(DisplayName = "Draw Pile"),
	DiscardPile UMETA(DisplayName = "Discard Pile"),
	Hand        UMETA(DisplayName = "Hand"),
	ExhaustPile UMETA(DisplayName = "Exhaust Pile")
};

/**
 * Gameplay / Server Authoritative Card Definition Data
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardGameplayData
{
	GENERATED_BODY()

	/** Unique identifier tag or name for this card type */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay")
	FName CardId = NAME_None;

	/** Base Mana cost to play the card */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay", meta = (ClampMin = "0"))
	int32 BaseManaCost = 1;

	/** Card functional type */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay")
	EFCCardType CardType = EFCCardType::Attack;

	/** Target selection requirement */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay")
	EFCCardTargetType TargetType = EFCCardTargetType::SingleTarget;

	/** Whether this card shoots or spawns a projectile (e.g. Fireball, FireArrow) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay")
	bool bSpawnsProjectile = false;

	/** Optional Projectile Data Asset (active only when bSpawnsProjectile is true) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay", meta = (EditCondition = "bSpawnsProjectile", EditConditionHides))
	TObjectPtr<UFCProjectileDataAsset> ProjectileDataAsset;

	/** Gameplay Ability granted and activated when this card is played */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay")
	TSubclassOf<UGameplayAbility> CardAbilityClass;

	/** Optional direct Gameplay Effects applied upon playing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay")
	TArray<TSubclassOf<UGameplayEffect>> CardEffectClasses;

	/** Gameplay tags associated with this card (e.g. Card.Archetype.Fire, Card.Keyword.Exhaust) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay")
	FGameplayTagContainer CardTags;

	/** Base numerical value (damage, shield, heal amount) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay")
	float BaseValue = 10.0f;

	/** Character class required to play/deck this card (Neutral = all classes) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay")
	EFCCharacterClass RequiredClass = EFCCharacterClass::Neutral;

	/** Elemental affinities (Active for Mage cards) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay")
	TArray<EFCElement> Elements;

	/** Elements consumed from target(s) to activate or enhance this card's effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay")
	TArray<EFCElement> ConsumedElements;

	/** Whether this card is exhausted (sent to Exhaust Zone/Pile) upon being played */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay")
	bool bExhaustsOnPlay = false;

	/** Gameplay keywords associated with this card (e.g. Exhaust, Retain, Innate) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Gameplay")
	TArray<EFCCardKeyword> Keywords;

	/** Helper to check if card exhausts upon being played */
	bool DoesExhaustOnPlay() const
	{
		return bExhaustsOnPlay 
			|| Keywords.Contains(EFCCardKeyword::Exhaust)
			|| CardTags.HasTag(FGameplayTag::RequestGameplayTag(FName("Card.Keyword.Exhaust"), false));
	}

	/** Helper to check if card has the Retain keyword (preserved in hand on turn cycle) */
	bool DoesRetain() const
	{
		return Keywords.Contains(EFCCardKeyword::Retain)
			|| CardTags.HasTag(FGameplayTag::RequestGameplayTag(FName("Card.Keyword.Retain"), false));
	}

	/** Helper to check if card has the Ethereal keyword (exhausts if held at end of turn cycle) */
	bool IsEthereal() const
	{
		return Keywords.Contains(EFCCardKeyword::Ethereal)
			|| CardTags.HasTag(FGameplayTag::RequestGameplayTag(FName("Card.Keyword.Ethereal"), false));
	}

	/** Helper to format gameplay keywords for presentation/UI display */
	FText GetFormattedKeywordsText() const
	{
		TArray<FText> KeywordTexts;
		if (DoesExhaustOnPlay())
		{
			KeywordTexts.Add(NSLOCTEXT("FC_Card", "Keyword_Exhaust", "소멸"));
		}

		for (EFCCardKeyword Keyword : Keywords)
		{
			switch (Keyword)
			{
			case EFCCardKeyword::Retain:
				KeywordTexts.Add(NSLOCTEXT("FC_Card", "Keyword_Retain", "보존"));
				break;
			case EFCCardKeyword::Innate:
				KeywordTexts.Add(NSLOCTEXT("FC_Card", "Keyword_Innate", "선천성"));
				break;
			case EFCCardKeyword::Ethereal:
				KeywordTexts.Add(NSLOCTEXT("FC_Card", "Keyword_Ethereal", "휘발성"));
				break;
			case EFCCardKeyword::Unplayable:
				KeywordTexts.Add(NSLOCTEXT("FC_Card", "Keyword_Unplayable", "사용 불가"));
				break;
			default:
				break;
			}
		}

		if (KeywordTexts.Num() == 0)
		{
			return FText::GetEmpty();
		}

		return FText::Join(FText::FromString(TEXT(", ")), KeywordTexts);
	}

	/** Helper to check if card has a specific elemental affinity */
	bool HasElement(EFCElement InElement) const
	{
		return Elements.Contains(InElement);
	}

	/** Helper to check if card is Neutral */
	bool IsNeutral() const
	{
		return RequiredClass == EFCCharacterClass::Neutral;
	}

	/** Helper to check if card can hold elemental affinities */
	bool CanHaveElements() const
	{
		return RequiredClass == EFCCharacterClass::Mage;
	}

	/** Helper to check if card consumes element stacks */
	bool RequiresElementConsumption() const
	{
		return ConsumedElements.Num() > 0;
	}

	/** Helper to check if this card shoots or spawns a projectile */
	bool SpawnsProjectile() const
	{
		return bSpawnsProjectile || ProjectileDataAsset != nullptr;
	}

	/** Helper to format class trait text via adapter */
	FText GetFormattedTraitText() const
	{
		if (RequiredClass == EFCCharacterClass::Mage)
		{
			return FCClassTraitUtils::FormatMageElementsText(Elements);
		}
		return FText::GetEmpty();
	}
};

/**
 * Presentation / Visual & Audio Display Data (Client-Facing, Soft Referenced)
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardDisplayData
{
	GENERATED_BODY()

	/** Localized card title */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Display")
	FText CardName;

	/** Localized card ability description */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Display")
	FText CardDescription;

	/** Flavor / lore text */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Display")
	FText CardLore;

	/** Soft reference to card artwork texture (prevents blocking sync loads) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Display")
	TSoftObjectPtr<UTexture2D> CardIcon;

	/** Soft reference to card frame/border texture */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Display")
	TSoftObjectPtr<UTexture2D> CardFrame;

	/** Soft reference to audio cue played on card activation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Display")
	TSoftObjectPtr<USoundBase> PlaySound;

	/** Soft reference to Niagara particle effect on play */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Display")
	TSoftObjectPtr<UNiagaraSystem> PlayVFX;

	/** Visual rarity framing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Display")
	EFCCardRarity Rarity = EFCCardRarity::Common;
};

/**
 * Target payload passed with Server RPC when playing a card
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardTargetInfo
{
	GENERATED_BODY()

	/** Targeted actor if single-target or interaction */
	UPROPERTY(BlueprintReadWrite, Category = "Card|Target")
	TWeakObjectPtr<AActor> TargetActor = nullptr;

	/** World location for ground-targeted / AoE cards */
	UPROPERTY(BlueprintReadWrite, Category = "Card|Target")
	FVector_NetQuantize TargetLocation = FVector::ZeroVector;

	/** Specific component hit (e.g. head/limb/weakspot) */
	UPROPERTY(BlueprintReadWrite, Category = "Card|Target")
	TWeakObjectPtr<UPrimitiveComponent> TargetHitComponent = nullptr;
};
