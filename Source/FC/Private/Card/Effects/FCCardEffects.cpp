#include "Card/Effects/FCCardEffects.h"
#include "Card/FCCardExecutionContext.h"
#include "Card/FCCardDeckComponent.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/FCGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "AbilitySystem/Abilities/FCGA_SpawnProjectile.h"
#include "AbilitySystem/Effects/FCGE_Damage.h"
#include "Combat/FCCombatUtils.h"
#include "Combat/Element/FCElementComponent.h"
#include "Combat/Projectile/FCProjectileDataAsset.h"
#include "Combat/Wall/FCSandWall.h"
#include "Character/FCCharacterBase.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

namespace
{
	bool ActivateCardAbilityWithContext(
		UAbilitySystemComponent* ASC,
		TSubclassOf<UGameplayAbility> AbilityClass,
		const FFCCardExecutionContext& Context,
		const UObject* OverrideSourceObject = nullptr)
	{
		if (!ASC || !AbilityClass)
		{
			return false;
		}

		// Mirror verified target onto character for legacy abilities while forwarding immutable TargetData
		if (AFCCharacterBase* Char = Cast<AFCCharacterBase>(Context.CasterActor.Get()))
		{
			Char->SetCombatTarget(Context.GetPrimaryTarget());
			Char->SetTargetAimLocation(Context.VerifiedAimLocation);
		}

		FGameplayAbilitySpec Spec(AbilityClass, Context.UpgradeLevel + 1);
		const UObject* CandidateSourceObject = OverrideSourceObject
			? OverrideSourceObject
			: static_cast<const UObject*>(Context.CardDataAsset.Get());
		if (CandidateSourceObject && CandidateSourceObject->IsSupportedForNetworking())
		{
			Spec.SourceObject = const_cast<UObject*>(CandidateSourceObject);
		}

		FGameplayEventData EventData;
		EventData.Instigator = Context.CasterActor.Get();
		EventData.Target = Context.GetPrimaryTarget();
		EventData.OptionalObject = Spec.SourceObject.Get();
		EventData.EventMagnitude = Context.CardData ? Context.CardData->BaseValue : 0.0f;
		EventData.TargetData = Context.MakeAbilityTargetDataHandle();

		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbilityAndActivateOnce(Spec, &EventData);
		return Handle.IsValid();
	}
}

bool FFCCardEffect_SpawnProjectile::Execute(FFCCardExecutionContext& Context) const
{
	UAbilitySystemComponent* ASC = Context.CasterASC.Get();
	if (!ASC)
	{
		return false;
	}

	TSubclassOf<UGameplayAbility> AbilityToRun = nullptr;
	if (!OverrideAbilityClass.IsNull())
	{
		AbilityToRun = OverrideAbilityClass.IsValid() ? OverrideAbilityClass.Get() : OverrideAbilityClass.LoadSynchronous();
	}
	if (!AbilityToRun)
	{
		AbilityToRun = UFCGA_SpawnProjectile::StaticClass();
	}

	const UObject* SourceObj = Context.CardDataAsset.Get();
	if (!ProjectileDataAsset.IsNull())
	{
		if (UFCProjectileDataAsset* LoadedProj = ProjectileDataAsset.IsValid() ? ProjectileDataAsset.Get() : ProjectileDataAsset.LoadSynchronous())
		{
			// Prefer passing CardDataAsset if it carries the same projectile, otherwise pass ProjectileDataAsset directly
			if (!Context.CardDataAsset.IsValid())
			{
				SourceObj = LoadedProj;
			}
			else if (Context.CardDataAsset->GameplayData.ProjectileDataAsset.IsNull())
			{
				SourceObj = LoadedProj;
			}
		}
	}

	return ActivateCardAbilityWithContext(ASC, AbilityToRun, Context, SourceObj);
}

void FFCCardEffect_SpawnProjectile::GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	if (!ProjectileDataAsset.IsNull())
	{
		OutPaths.AddUnique(ProjectileDataAsset.ToSoftObjectPath());
	}
	if (!OverrideAbilityClass.IsNull())
	{
		OutPaths.AddUnique(OverrideAbilityClass.ToSoftObjectPath());
	}
}

bool FFCCardEffect_ApplyGameplayEffect::Execute(FFCCardExecutionContext& Context) const
{
	UAbilitySystemComponent* CasterASC = Context.CasterASC.Get();
	AActor* Caster = Context.CasterActor.Get();
	if (!CasterASC || !Caster || GameplayEffectClass.IsNull())
	{
		return false;
	}

	TSubclassOf<UGameplayEffect> EffectClass = GameplayEffectClass.IsValid()
		? GameplayEffectClass.Get()
		: GameplayEffectClass.LoadSynchronous();
	if (!EffectClass)
	{
		return false;
	}

	const float BaseMag = (MagnitudeOverride > 0.0f)
		? MagnitudeOverride
		: (Context.CardData ? Context.CardData->BaseValue : 0.0f);
	const float EffectiveMag = (BaseMag + (UpgradeBonusPerLevel * static_cast<float>(Context.UpgradeLevel))) * MagnitudeSign;

	auto ApplyToASC = [&](UAbilitySystemComponent* TargetASC) -> bool
	{
		if (!TargetASC)
		{
			return false;
		}
		FGameplayEffectContextHandle EffectContext = CasterASC->MakeEffectContext();
		EffectContext.AddInstigator(Caster, Caster);
		FGameplayEffectSpecHandle SpecHandle = CasterASC->MakeOutgoingSpec(EffectClass, Context.UpgradeLevel + 1, EffectContext);
		if (!SpecHandle.IsValid())
		{
			return false;
		}
		if (SetByCallerTag.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerTag, EffectiveMag);
		}
		TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		return true;
	};

	if (bApplyToSelf)
	{
		return ApplyToASC(CasterASC);
	}

	bool bAppliedAny = false;
	for (const TWeakObjectPtr<AActor>& TargetPtr : Context.VerifiedTargets)
	{
		if (AActor* Target = TargetPtr.Get())
		{
			if (Target == Caster)
			{
				bAppliedAny |= ApplyToASC(CasterASC);
			}
			else if (const IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(Target))
			{
				bAppliedAny |= ApplyToASC(TargetASI->GetAbilitySystemComponent());
			}
		}
	}

	return bAppliedAny;
}

void FFCCardEffect_ApplyGameplayEffect::GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	if (!GameplayEffectClass.IsNull())
	{
		OutPaths.AddUnique(GameplayEffectClass.ToSoftObjectPath());
	}
}

bool FFCCardEffect_ActivateAbility::Execute(FFCCardExecutionContext& Context) const
{
	UAbilitySystemComponent* ASC = Context.CasterASC.Get();
	if (!ASC || AbilityClass.IsNull())
	{
		return false;
	}

	TSubclassOf<UGameplayAbility> LoadedClass = AbilityClass.IsValid()
		? AbilityClass.Get()
		: AbilityClass.LoadSynchronous();
	return ActivateCardAbilityWithContext(ASC, LoadedClass, Context);
}

void FFCCardEffect_ActivateAbility::GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	if (!AbilityClass.IsNull())
	{
		OutPaths.AddUnique(AbilityClass.ToSoftObjectPath());
	}
}

bool FFCCardEffect_ApplyElementTags::Execute(FFCCardExecutionContext& Context) const
{
	AActor* Caster = Context.CasterActor.Get();
	if (!Caster || !Caster->HasAuthority())
	{
		return false;
	}

	const TArray<FGameplayTag>& TagsToApply = ElementTags.Num() > 0
		? ElementTags
		: (Context.CardData ? Context.CardData->ElementTags : ElementTags);

	if (TagsToApply.Num() == 0)
	{
		return false;
	}

	auto ApplyToActor = [&](AActor* Target) -> bool
	{
		if (UFCElementComponent* ElemComp = UFCCombatUtils::GetElementComponent(Target))
		{
			ElemComp->AddElementTagStacks(TagsToApply);
			return true;
		}
		return false;
	};

	if (bApplyToSelf)
	{
		return ApplyToActor(Caster);
	}

	bool bAppliedAny = false;
	for (const TWeakObjectPtr<AActor>& TargetPtr : Context.VerifiedTargets)
	{
		if (AActor* Target = TargetPtr.Get())
		{
			bAppliedAny |= ApplyToActor(Target);
		}
	}
	return bAppliedAny;
}

bool FFCCardEffect_DetonateElementTag::Execute(FFCCardExecutionContext& Context) const
{
	AActor* Caster = Context.CasterActor.Get();
	if (!Caster || !Caster->HasAuthority() || !TargetElementTag.IsValid())
	{
		return false;
	}

	const float BasePerStack = (DamagePerStackOverride > 0.0f)
		? DamagePerStackOverride
		: (Context.CardData ? Context.CardData->BaseValue : 10.0f);
	const float DamagePerStack = BasePerStack + (UpgradeBonusPerStack * static_cast<float>(Context.UpgradeLevel));
	if (DamagePerStack <= 0.0f)
	{
		return false;
	}

	TSubclassOf<UGameplayEffect> DamageGE = nullptr;
	if (!DamageEffectClass.IsNull())
	{
		DamageGE = DamageEffectClass.IsValid() ? DamageEffectClass.Get() : DamageEffectClass.LoadSynchronous();
	}
	if (!DamageGE)
	{
		DamageGE = UFCGE_Damage::StaticClass();
	}

	bool bHitAny = false;
	for (const TWeakObjectPtr<AActor>& TargetPtr : Context.VerifiedTargets)
	{
		AActor* Target = TargetPtr.Get();
		UFCElementComponent* ElemComp = UFCCombatUtils::GetElementComponent(Target);
		if (!Target || !ElemComp)
		{
			continue;
		}

		const int32 StackCount = ElemComp->GetElementTagCount(TargetElementTag);
		if (StackCount <= 0)
		{
			continue;
		}

		const float TotalDamage = static_cast<float>(StackCount) * DamagePerStack;
		if (const IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(Target))
		{
			if (UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent())
			{
				FGameplayEffectContextHandle EffectContext = TargetASC->MakeEffectContext();
				EffectContext.AddInstigator(Caster, Caster);
				if (FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageGE, 1.0f, EffectContext); SpecHandle.IsValid())
				{
					SpecHandle.Data->SetSetByCallerMagnitude(FCGameplayTags::Data_Damage, -TotalDamage);
					TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
					bHitAny = true;
				}
			}
		}

		if (bConsumeStacks)
		{
			ElemComp->RemoveAllStacksOfElementTag(TargetElementTag);
		}
	}

	return bHitAny;
}

void FFCCardEffect_DetonateElementTag::GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	if (!DamageEffectClass.IsNull())
	{
		OutPaths.AddUnique(DamageEffectClass.ToSoftObjectPath());
	}
}

bool FFCCardEffect_SpawnTacticalActor::Execute(FFCCardExecutionContext& Context) const
{
	AActor* Caster = Context.CasterActor.Get();
	if (!Caster || !Caster->HasAuthority() || ActorClass.IsNull())
	{
		return false;
	}

	UWorld* World = Caster->GetWorld();
	if (!World)
	{
		return false;
	}

	TSubclassOf<AActor> LoadedClass = ActorClass.IsValid() ? ActorClass.Get() : ActorClass.LoadSynchronous();
	if (!LoadedClass)
	{
		return false;
	}

	const FVector BaseLocation = Caster->GetActorLocation();
	FVector AimDir = Caster->GetActorForwardVector();
	if (AActor* PrimaryTarget = Context.GetPrimaryTarget())
	{
		const FVector ToTarget = (PrimaryTarget->GetActorLocation() - BaseLocation).GetSafeNormal2D();
		if (!ToTarget.IsNearlyZero())
		{
			AimDir = ToTarget;
		}
	}
	else if (!Context.VerifiedAimLocation.IsZero())
	{
		const FVector ToAim = (Context.VerifiedAimLocation - BaseLocation).GetSafeNormal2D();
		if (!ToAim.IsNearlyZero())
		{
			AimDir = ToAim;
		}
	}

	const FVector SpawnLocation = BaseLocation + (AimDir * ForwardDistance);
	FRotator SpawnRotation = AimDir.Rotation();
	SpawnRotation.Pitch = 0.0f;
	SpawnRotation.Roll = 0.0f;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Caster;
	SpawnParams.Instigator = Cast<APawn>(Caster);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Spawned = World->SpawnActor<AActor>(LoadedClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!Spawned)
	{
		return false;
	}

	const float EffectiveDuration = (DurationOverride > 0.0f)
		? DurationOverride
		: (Context.CardData && Context.CardData->BaseValue > 0.0f ? Context.CardData->BaseValue : 0.0f);

	if (EffectiveDuration > 0.0f)
	{
		if (AFCSandWall* SandWall = Cast<AFCSandWall>(Spawned))
		{
			SandWall->SetWallDuration(EffectiveDuration);
		}
		else
		{
			Spawned->SetLifeSpan(EffectiveDuration);
		}
	}

	return true;
}

void FFCCardEffect_SpawnTacticalActor::GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	if (!ActorClass.IsNull())
	{
		OutPaths.AddUnique(ActorClass.ToSoftObjectPath());
	}
}

bool FFCCardEffect_DrawCards::Execute(FFCCardExecutionContext& Context) const
{
	if (DrawCount <= 0)
	{
		return false;
	}
	if (UFCCardDeckComponent* DeckComp = Context.DeckComponent.Get())
	{
		DeckComp->DrawCards(DrawCount);
		return true;
	}
	return false;
}
