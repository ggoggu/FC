#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Card/Effects/FCCardEffectBase.h"
#include "FCCardEffects.generated.h"

class AActor;
class UGameplayAbility;
class UGameplayEffect;
class UFCProjectileDataAsset;

/**
 * Spawns a projectile by activating UFCGA_SpawnProjectile (or an override GA) with immutable TargetData.
 * Element stacks on projectile cards are applied strictly on hit (never double-applied at cast time).
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Effect: Spawn Projectile"))
struct FC_API FFCCardEffect_SpawnProjectile : public FFCCardEffectBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	TSoftObjectPtr<UFCProjectileDataAsset> ProjectileDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	TSoftClassPtr<UGameplayAbility> OverrideAbilityClass;

	virtual bool Execute(FFCCardExecutionContext& Context) const override;
	virtual void GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const override;
};

/**
 * Applies a GameplayEffect to self or verified target(s), optionally injecting SetByCaller magnitude
 * from CardData->BaseValue + UpgradeLevel scaling.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Effect: Apply Gameplay Effect"))
struct FC_API FFCCardEffect_ApplyGameplayEffect : public FFCCardEffectBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	TSoftClassPtr<UGameplayEffect> GameplayEffectClass;

	/** Optional SetByCaller tag (e.g. Data.Magnitude, Data.Damage). If set, injects EffectiveMagnitude. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	FGameplayTag SetByCallerTag;

	/** If > 0, overrides CardData->BaseValue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	float MagnitudeOverride = 0.0f;

	/** Added per card upgrade level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	float UpgradeBonusPerLevel = 0.0f;

	/** Multiplier applied to magnitude before SetByCaller injection (e.g. -1.0f for negative damage) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	float MagnitudeSign = 1.0f;

	/** Forces application to CasterASC regardless of VerifiedTargets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	bool bApplyToSelf = false;

	virtual bool Execute(FFCCardExecutionContext& Context) const override;
	virtual void GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const override;
};

/**
 * Grants and activates a GameplayAbility once, forwarding immutable FFCCardAbilityTargetData via FGameplayEventData.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Effect: Activate Gameplay Ability"))
struct FC_API FFCCardEffect_ActivateAbility : public FFCCardEffectBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	TSoftClassPtr<UGameplayAbility> AbilityClass;

	virtual bool Execute(FFCCardExecutionContext& Context) const override;
	virtual void GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const override;
};

/**
 * Directly applies elemental stack tags to verified target(s) or self (for instant non-projectile attacks/skills).
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Effect: Apply Element Tags"))
struct FC_API FFCCardEffect_ApplyElementTags : public FFCCardEffectBase
{
	GENERATED_BODY()

	/** Explicit elements to apply; if empty, uses CardData->ElementTags */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	TArray<FGameplayTag> ElementTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	bool bApplyToSelf = false;

	virtual bool Execute(FFCCardExecutionContext& Context) const override;
};

/**
 * Detonates accumulated stacks of any TargetElementTag across all VerifiedTargets (generalizes Ignite to all elements).
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Effect: Detonate Element Tag"))
struct FC_API FFCCardEffect_DetonateElementTag : public FFCCardEffectBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	FGameplayTag TargetElementTag;

	/** If <= 0, uses CardData->BaseValue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	float DamagePerStackOverride = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	float UpgradeBonusPerStack = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	bool bConsumeStacks = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	TSoftClassPtr<UGameplayEffect> DamageEffectClass;

	virtual bool Execute(FFCCardExecutionContext& Context) const override;
	virtual void GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const override;
};

/**
 * Spawns a tactical actor (barrier, trap, totem) oriented along the resolved aim vector.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Effect: Spawn Tactical Actor"))
struct FC_API FFCCardEffect_SpawnTacticalActor : public FFCCardEffectBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect")
	TSoftClassPtr<AActor> ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect", meta = (ClampMin = "0.0"))
	float ForwardDistance = 150.0f;

	/** If > 0, sets actor lifespan (if <= 0, uses CardData->BaseValue when positive) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect", meta = (ClampMin = "0.0"))
	float DurationOverride = 0.0f;

	virtual bool Execute(FFCCardExecutionContext& Context) const override;
	virtual void GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const override;
};

/**
 * Draws additional cards from the caster's deck component.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Effect: Draw Cards"))
struct FC_API FFCCardEffect_DrawCards : public FFCCardEffectBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Effect", meta = (ClampMin = "1"))
	int32 DrawCount = 1;

	virtual bool Execute(FFCCardExecutionContext& Context) const override;
};
