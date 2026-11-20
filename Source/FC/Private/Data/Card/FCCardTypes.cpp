#include "Data/Card/FCCardTypes.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Class/FCClassSubsystem.h"
#include "Data/FCGameplayTags.h"
#include "Card/Policies/FCCardTargetPolicies.h"
#include "Card/Conditions/FCCardConditions.h"
#include "Card/Effects/FCCardEffects.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "Combat/Projectile/FCProjectileDataAsset.h"

namespace
{
	FGameplayTag CardTypeToTag(EFCCardType Type)
	{
		switch (Type)
		{
		case EFCCardType::Attack: return FCGameplayTags::Card_Type_Attack;
		case EFCCardType::Skill:  return FCGameplayTags::Card_Type_Skill;
		case EFCCardType::Power:  return FCGameplayTags::Card_Type_Power;
		case EFCCardType::Item:   return FCGameplayTags::Card_Type_Item;
		case EFCCardType::Curse:  return FCGameplayTags::Card_Type_Curse;
		default:                  return FCGameplayTags::Card_Type_Attack;
		}
	}
}

void FFCCardGameplayData::EnsureNormalizedPipeline()
{
	if (!RequiredClassTag.IsValid())
	{
		RequiredClassTag = FCGameplayTags::Class_Neutral;
	}

	if (!CardTypeTag.IsValid())
	{
		CardTypeTag = CardTypeToTag(CardType);
	}

	const bool bIsAttack = (CardTypeTag == FCGameplayTags::Card_Type_Attack) || (CardType == EFCCardType::Attack);

	if (!TargetPolicy.IsValid())
	{
		switch (TargetType)
		{
		case EFCCardTargetType::Self:
			TargetPolicy = TInstancedStruct<FFCCardTargetPolicyBase>::Make<FFCCardTargetPolicy_Self>();
			break;
		case EFCCardTargetType::SingleTarget:
		{
			FFCCardTargetPolicy_SingleTarget Policy;
			Policy.bAllowFriendlyTargets = !bIsAttack;
			TargetPolicy = TInstancedStruct<FFCCardTargetPolicyBase>::Make<FFCCardTargetPolicy_SingleTarget>(Policy);
			break;
		}
		case EFCCardTargetType::DirectionalAoE:
		{
			FFCCardTargetPolicy_Directional Policy;
			Policy.bAllowFriendlyTargets = !bIsAttack;
			TargetPolicy = TInstancedStruct<FFCCardTargetPolicyBase>::Make<FFCCardTargetPolicy_Directional>(Policy);
			break;
		}
		case EFCCardTargetType::AllEnemies:
			TargetPolicy = TInstancedStruct<FFCCardTargetPolicyBase>::Make<FFCCardTargetPolicy_AllEnemiesInRadius>();
			break;
		case EFCCardTargetType::None:
		default:
			TargetPolicy = TInstancedStruct<FFCCardTargetPolicyBase>::Make<FFCCardTargetPolicy_None>();
			break;
		}
	}

	if (Conditions.Num() == 0 && ConsumedElementTags.Num() > 0)
	{
		FFCCardCondition_ConsumeElementTags ConsumeCond;
		ConsumeCond.RequiredElementTags = ConsumedElementTags;
		ConsumeCond.bBlockCardPlayIfUnmet = false;
		Conditions.Add(TInstancedStruct<FFCCardConditionBase>::Make<FFCCardCondition_ConsumeElementTags>(ConsumeCond));
	}

	if (PlayEffects.Num() == 0)
	{
		const bool bProjectileCard = bSpawnsProjectile || !ProjectileDataAsset.IsNull();
		if (bProjectileCard)
		{
			FFCCardEffect_SpawnProjectile ProjEffect;
			ProjEffect.ProjectileDataAsset = ProjectileDataAsset;
			ProjEffect.OverrideAbilityClass = CardAbilityClass;
			PlayEffects.Add(TInstancedStruct<FFCCardEffectBase>::Make<FFCCardEffect_SpawnProjectile>(ProjEffect));
		}
		else if (!CardAbilityClass.IsNull())
		{
			FFCCardEffect_ActivateAbility AbilityEffect;
			AbilityEffect.AbilityClass = CardAbilityClass;
			PlayEffects.Add(TInstancedStruct<FFCCardEffectBase>::Make<FFCCardEffect_ActivateAbility>(AbilityEffect));
		}

		for (const TSoftClassPtr<UGameplayEffect>& EffectClass : CardEffectClasses)
		{
			if (!EffectClass.IsNull())
			{
				FFCCardEffect_ApplyGameplayEffect GEEffect;
				GEEffect.GameplayEffectClass = EffectClass;
				GEEffect.SetByCallerTag = FCGameplayTags::Data_Magnitude;
				GEEffect.bApplyToSelf = (TargetType == EFCCardTargetType::Self);
				PlayEffects.Add(TInstancedStruct<FFCCardEffectBase>::Make<FFCCardEffect_ApplyGameplayEffect>(GEEffect));
			}
		}

		if (!bProjectileCard && bIsAttack && ElementTags.Num() > 0 && CardAbilityClass.IsNull())
		{
			FFCCardEffect_ApplyElementTags ElemEffect;
			ElemEffect.ElementTags = ElementTags;
			PlayEffects.Add(TInstancedStruct<FFCCardEffectBase>::Make<FFCCardEffect_ApplyElementTags>(ElemEffect));
		}
	}
}

void FFCCardGameplayData::GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	if (!CardAbilityClass.IsNull())
	{
		OutPaths.AddUnique(CardAbilityClass.ToSoftObjectPath());
	}
	for (const TSoftClassPtr<UGameplayEffect>& EffectClass : CardEffectClasses)
	{
		if (!EffectClass.IsNull())
		{
			OutPaths.AddUnique(EffectClass.ToSoftObjectPath());
		}
	}
	if (!ProjectileDataAsset.IsNull())
	{
		OutPaths.AddUnique(ProjectileDataAsset.ToSoftObjectPath());
	}

	for (const TInstancedStruct<FFCCardConditionBase>& CondStruct : Conditions)
	{
		if (const FFCCardConditionBase* Cond = CondStruct.GetPtr())
		{
			Cond->GatherSoftAssetPaths(OutPaths);
		}
	}

	for (const TInstancedStruct<FFCCardEffectBase>& EffectStruct : PlayEffects)
	{
		if (const FFCCardEffectBase* Effect = EffectStruct.GetPtr())
		{
			Effect->GatherSoftAssetPaths(OutPaths);
		}
	}
}

bool FFCCardGameplayData::DoesExhaustOnPlay() const
{
	return CardTags.HasTag(FCGameplayTags::Card_Keyword_Exhaust);
}

bool FFCCardGameplayData::DoesRetain() const
{
	return CardTags.HasTag(FCGameplayTags::Card_Keyword_Retain);
}

bool FFCCardGameplayData::IsEthereal() const
{
	return CardTags.HasTag(FCGameplayTags::Card_Keyword_Ethereal);
}

bool FFCCardGameplayData::IsInnate() const
{
	return CardTags.HasTag(FCGameplayTags::Card_Keyword_Innate);
}

bool FFCCardGameplayData::IsUnplayable() const
{
	return CardTags.HasTag(FCGameplayTags::Card_Keyword_Unplayable);
}

FText FFCCardGameplayData::GetFormattedKeywordsText(const UObject* WorldContextObject) const
{
	return UFCCardSubsystem::FormatKeywordTagsText(CardTags, WorldContextObject);
}

bool FFCCardGameplayData::IsNeutral() const
{
	return !RequiredClassTag.IsValid() || RequiredClassTag == FCGameplayTags::Class_Neutral;
}

bool FFCCardGameplayData::CanHaveElements(const UObject* WorldContextObject) const
{
	return FCClassTraitUtils::SupportsElementalAffinities(RequiredClassTag, WorldContextObject);
}

FText FFCCardGameplayData::GetFormattedTraitText(const UObject* WorldContextObject) const
{
	const UFCClassSubsystem* ClassSub = UFCClassSubsystem::GetClassSubsystem(WorldContextObject);
	const FFCCharacterClassData* ClassRow = ClassSub
		? ClassSub->FindClassRowPtr(RequiredClassTag)
		: UFCClassSubsystem::FindBuiltinClassDefault(RequiredClassTag);

	if (ClassRow)
	{
		if (const FFCClassTraitBehaviorBase* Trait = ClassRow->TraitBehavior.GetPtr())
		{
			return Trait->FormatCardTraitText(ElementTags, WorldContextObject);
		}
	}

	return FText::GetEmpty();
}

bool FFCCardGameplayData::SpawnsProjectile() const
{
	if (bSpawnsProjectile || !ProjectileDataAsset.IsNull())
	{
		return true;
	}
	for (const TInstancedStruct<FFCCardEffectBase>& EffectStruct : PlayEffects)
	{
		if (EffectStruct.GetScriptStruct() && EffectStruct.GetScriptStruct()->IsChildOf(FFCCardEffect_SpawnProjectile::StaticStruct()))
		{
			return true;
		}
	}
	return false;
}

TSubclassOf<UGameplayAbility> FFCCardGameplayData::GetCardAbilityClass() const
{
	if (!CardAbilityClass.IsNull())
	{
		return CardAbilityClass.IsValid() ? CardAbilityClass.Get() : CardAbilityClass.LoadSynchronous();
	}

	for (const TInstancedStruct<FFCCardEffectBase>& EffectStruct : PlayEffects)
	{
		if (const FFCCardEffect_ActivateAbility* AbilityEffect = EffectStruct.GetPtr<FFCCardEffect_ActivateAbility>())
		{
			if (!AbilityEffect->AbilityClass.IsNull())
			{
				return AbilityEffect->AbilityClass.IsValid() ? AbilityEffect->AbilityClass.Get() : AbilityEffect->AbilityClass.LoadSynchronous();
			}
		}
		else if (const FFCCardEffect_SpawnProjectile* ProjEffect = EffectStruct.GetPtr<FFCCardEffect_SpawnProjectile>())
		{
			if (!ProjEffect->OverrideAbilityClass.IsNull())
			{
				return ProjEffect->OverrideAbilityClass.IsValid() ? ProjEffect->OverrideAbilityClass.Get() : ProjEffect->OverrideAbilityClass.LoadSynchronous();
			}
		}
	}
	return nullptr;
}

TArray<TSubclassOf<UGameplayEffect>> FFCCardGameplayData::GetCardEffectClasses() const
{
	TArray<TSubclassOf<UGameplayEffect>> ResolvedEffects;
	for (const TSoftClassPtr<UGameplayEffect>& SoftEffect : CardEffectClasses)
	{
		if (!SoftEffect.IsNull())
		{
			if (TSubclassOf<UGameplayEffect> Loaded = SoftEffect.IsValid() ? SoftEffect.Get() : SoftEffect.LoadSynchronous())
			{
				ResolvedEffects.AddUnique(Loaded);
			}
		}
	}
	for (const TInstancedStruct<FFCCardEffectBase>& EffectStruct : PlayEffects)
	{
		if (const FFCCardEffect_ApplyGameplayEffect* GEEffect = EffectStruct.GetPtr<FFCCardEffect_ApplyGameplayEffect>())
		{
			if (!GEEffect->GameplayEffectClass.IsNull())
			{
				if (TSubclassOf<UGameplayEffect> Loaded = GEEffect->GameplayEffectClass.IsValid() ? GEEffect->GameplayEffectClass.Get() : GEEffect->GameplayEffectClass.LoadSynchronous())
				{
					ResolvedEffects.AddUnique(Loaded);
				}
			}
		}
	}
	return ResolvedEffects;
}

UFCProjectileDataAsset* FFCCardGameplayData::GetProjectileDataAsset() const
{
	if (!ProjectileDataAsset.IsNull())
	{
		return ProjectileDataAsset.IsValid() ? ProjectileDataAsset.Get() : ProjectileDataAsset.LoadSynchronous();
	}
	for (const TInstancedStruct<FFCCardEffectBase>& EffectStruct : PlayEffects)
	{
		if (const FFCCardEffect_SpawnProjectile* ProjEffect = EffectStruct.GetPtr<FFCCardEffect_SpawnProjectile>())
		{
			if (!ProjEffect->ProjectileDataAsset.IsNull())
			{
				return ProjEffect->ProjectileDataAsset.IsValid() ? ProjEffect->ProjectileDataAsset.Get() : ProjEffect->ProjectileDataAsset.LoadSynchronous();
			}
		}
	}
	return nullptr;
}
