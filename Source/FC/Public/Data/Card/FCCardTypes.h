#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/NetSerialization.h"
#include "Engine/DataTable.h"
#include "StructUtils/InstancedStruct.h"
#include "Data/Class/FCClassTypes.h"
#include "Card/Policies/FCCardTargetPolicyBase.h"
#include "Card/Conditions/FCCardConditionBase.h"
#include "Card/Effects/FCCardEffectBase.h"
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
 * Card category/type classification (used for UI sorting order alongside CardTypeTag)
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
 * Card targeting paradigm hint
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
 * Unified card zone / circulation pile classification
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
 * Gameplay / Server Authoritative Card Definition Data
 *
 * Composes polymorphic TInstancedStruct policies, conditions, and effect fragments
 * with pure GameplayTag class, element, and keyword classification inside DataTable rows.
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardGameplayData
{
	GENERATED_BODY()

	/** Unique identifier tag or name for this card type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay")
	FName CardId = NAME_None;

	/** Optional legacy or localized lookup aliases indexed automatically by UFCCardSubsystem */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay")
	TArray<FName> LegacyAliases;

	/** Base Mana cost to play the card */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay", meta = (ClampMin = "0"))
	int32 BaseManaCost = 1;

	/** Card functional type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay")
	EFCCardType CardType = EFCCardType::Attack;

	/** GameplayTag functional type (Card.Type.*) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay")
	FGameplayTag CardTypeTag;

	/** Target selection hint (auto-normalizes TargetPolicy when TargetPolicy is unset) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay")
	EFCCardTargetType TargetType = EFCCardTargetType::SingleTarget;

	/** Modular client/server targeting strategy (TInstancedStruct) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Pipeline", meta = (ExcludeBaseStruct))
	TInstancedStruct<FFCCardTargetPolicyBase> TargetPolicy;

	/** Modular server play preconditions & resource consumption strategies (TInstancedStruct) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Pipeline", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FFCCardConditionBase>> Conditions;

	/** Modular server execution effect fragments executed sequentially on card play (TInstancedStruct) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Pipeline", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FFCCardEffectBase>> PlayEffects;

	/** Optional Projectile Data Asset shortcut (soft referenced) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay")
	bool bSpawnsProjectile = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay", meta = (EditCondition = "bSpawnsProjectile", EditConditionHides))
	TSoftObjectPtr<UFCProjectileDataAsset> ProjectileDataAsset;

	/** Optional Gameplay Ability shortcut (soft referenced) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay")
	TSoftClassPtr<UGameplayAbility> CardAbilityClass;

	/** Optional direct Gameplay Effects shortcut (soft referenced) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay")
	TArray<TSoftClassPtr<UGameplayEffect>> CardEffectClasses;

	/** Gameplay tags associated with this card (Card.Keyword.*, Card.Archetype.* — SSOT for keywords) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay")
	FGameplayTagContainer CardTags;

	/** Base numerical value (damage, shield, heal, buff magnitude) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay")
	float BaseValue = 10.0f;

	/** Character class required to play/deck this card (Class.Neutral = all classes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay")
	FGameplayTag RequiredClassTag;

	/** Elemental affinities by GameplayTag (Element.*) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay")
	TArray<FGameplayTag> ElementTags;

	/** Elements consumed from target(s) by GameplayTag (Element.*) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Gameplay")
	TArray<FGameplayTag> ConsumedElementTags;

	/** Populates default Tags and TInstancedStruct pipeline fragments from shortcut properties when unset */
	void EnsureNormalizedPipeline();

	/** Collects all soft asset paths (including nested TInstancedStruct conditions & effects) */
	void GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const;

	/** Keyword helpers backed solely by CardTags (Card.Keyword.*) */
	bool DoesExhaustOnPlay() const;
	bool DoesRetain() const;
	bool IsEthereal() const;
	bool IsInnate() const;
	bool IsUnplayable() const;

	/** Formats Card.Keyword.* tags for UI display via UFCCardSubsystem Keyword DataTable */
	FText GetFormattedKeywordsText(const UObject* WorldContextObject = nullptr) const;

	/** Returns true if card holds a specific elemental GameplayTag */
	bool HasElementTag(FGameplayTag InElementTag) const
	{
		return ElementTags.Contains(InElementTag);
	}

	/** Returns true if card is Neutral (usable by all classes) */
	bool IsNeutral() const;

	/** Returns true if card's RequiredClassTag supports elemental affinities (DataTable-driven) */
	bool CanHaveElements(const UObject* WorldContextObject = nullptr) const;

	/** Returns true if card consumes element stacks */
	bool RequiresElementConsumption() const
	{
		return ConsumedElementTags.Num() > 0;
	}

	/** Returns true if this card shoots or spawns a projectile */
	bool SpawnsProjectile() const;

	TSubclassOf<UGameplayAbility> GetCardAbilityClass() const;
	TArray<TSubclassOf<UGameplayEffect>> GetCardEffectClasses() const;
	UFCProjectileDataAsset* GetProjectileDataAsset() const;

	/** Formats class trait text polymorphically via ClassData->TraitBehavior */
	FText GetFormattedTraitText(const UObject* WorldContextObject = nullptr) const;
};

/**
 * Presentation / Visual & Audio Display Data (Client-Facing, Soft Referenced)
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardDisplayData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Display")
	FText CardName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Display")
	FText CardDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Display")
	FText CardLore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Display")
	TSoftObjectPtr<UTexture2D> CardIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Display")
	TSoftObjectPtr<UTexture2D> CardFrame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Display")
	TSoftObjectPtr<USoundBase> PlaySound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Display")
	TSoftObjectPtr<UNiagaraSystem> PlayVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Display")
	EFCCardRarity Rarity = EFCCardRarity::Common;
};

/**
 * Row structure for split Card Catalog DataTables (DT_CardCatalog / DT_Cards_*)
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	FFCCardGameplayData GameplayData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	FFCCardDisplayData DisplayData;
};

/**
 * Target payload passed with Server RPC when playing a card
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardTargetInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Card|Target")
	TWeakObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Card|Target")
	FVector_NetQuantize TargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Card|Target")
	TWeakObjectPtr<UPrimitiveComponent> TargetHitComponent = nullptr;
};
