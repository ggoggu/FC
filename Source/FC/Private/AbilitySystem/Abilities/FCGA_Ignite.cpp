#include "AbilitySystem/Abilities/FCGA_Ignite.h"
#include "Combat/FCCombatUtils.h"
#include "Combat/Element/FCElementComponent.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Data/Card/FCCardDataAsset.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/Pawn.h"

UFCGA_Ignite::UFCGA_Ignite()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	DamagePerStack = 10.0f;
	Radius = 1000.0f;
	bConsumeFireStacks = false;
}

void UFCGA_Ignite::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. Resolve Effective Damage Per Stack from CardDataAsset if available
	float EffectiveDamagePerStack = DamagePerStack;
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (const FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle))
		{
			if (const UFCCardDataAsset* CardAsset = Cast<UFCCardDataAsset>(Spec->SourceObject.Get()))
			{
				if (CardAsset->GameplayData.BaseValue > 0.0f)
				{
					EffectiveDamagePerStack = CardAsset->GameplayData.BaseValue;
				}
			}
		}
	}

	// 2. Play Caster Presentation (Audio & VFX)
	const FVector CasterLocation = Avatar->GetActorLocation();
	const FRotator CasterRotation = Avatar->GetActorRotation();

	if (CastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastSound, CasterLocation);
	}
	if (CastVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, CastVFX, CasterLocation, CasterRotation);
	}

	// 3. Server-Authoritative Target Search & Damage Application
	if (HasAuthority(&ActivationInfo))
	{
		TArray<AActor*> NearbyEnemies = FindNearbyEnemies(Avatar, Radius);

		for (AActor* Enemy : NearbyEnemies)
		{
			if (Enemy)
			{
				const float AppliedDamage = ApplyIgniteToTarget(
					Avatar,
					Enemy,
					EffectiveDamagePerStack,
					bConsumeFireStacks,
					DamageEffectClass
				);

				if (AppliedDamage > 0.0f && TargetImpactVFX)
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, TargetImpactVFX, Enemy->GetActorLocation());
				}
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

float UFCGA_Ignite::ApplyIgniteToTarget(
	AActor* InstigatorActor,
	AActor* TargetActor,
	float InDamagePerStack,
	bool bInConsumeStacks,
	TSubclassOf<UGameplayEffect> InDamageEffectClass)
{
	if (!TargetActor || InDamagePerStack <= 0.0f)
	{
		return 0.0f;
	}

	// Server-authoritative check (allow test execution if no world authority check fails)
	if (TargetActor->GetWorld() && !TargetActor->HasAuthority())
	{
		return 0.0f;
	}

	// Query fire stacks on target
	const int32 FireStacks = UFCCombatUtils::GetTargetElementCount(TargetActor, EFCElement::Fire);
	if (FireStacks <= 0)
	{
		return 0.0f;
	}

	const float DamageToDeal = FireStacks * InDamagePerStack;

	// Resolve target Ability System Component & AttributeSet
	UAbilitySystemComponent* TargetASC = nullptr;
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor))
	{
		TargetASC = ASI->GetAbilitySystemComponent();
	}

	UFCAttributeSet* AttributeSet = nullptr;
	if (const AFCCharacterBase* FCChar = Cast<AFCCharacterBase>(TargetActor))
	{
		AttributeSet = FCChar->GetAttributeSet();
	}
	else if (TargetASC)
	{
		AttributeSet = const_cast<UFCAttributeSet*>(TargetASC->GetSet<UFCAttributeSet>());
	}

	if (TargetASC && InDamageEffectClass)
	{
		FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
		Context.AddInstigator(InstigatorActor, InstigatorActor);

		if (FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(InDamageEffectClass, 1.0f, Context); SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage"), false), -DamageToDeal);
			TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
	else if (AttributeSet)
	{
		const float CurrentShield = AttributeSet->GetShield();
		const float CurrentHealth = AttributeSet->GetHealth();
		float RemainingDamage = DamageToDeal;

		const bool bUseAscSetters = TargetASC && TargetASC->GetOwner() && TargetASC->GetWorld();

		if (CurrentShield > 0.0f)
		{
			const float Absorbed = FMath::Min(CurrentShield, RemainingDamage);
			const float NewShield = CurrentShield - Absorbed;
			if (bUseAscSetters)
			{
				AttributeSet->SetShield(NewShield);
			}
			else
			{
				AttributeSet->InitShield(NewShield);
			}
			RemainingDamage -= Absorbed;
		}

		if (RemainingDamage > 0.0f)
		{
			const float NewHealth = FMath::Clamp(CurrentHealth - RemainingDamage, 0.0f, AttributeSet->GetMaxHealth());
			if (bUseAscSetters)
			{
				AttributeSet->SetHealth(NewHealth);
			}
			else
			{
				AttributeSet->InitHealth(NewHealth);
			}
		}
	}
	else
	{
		UGameplayStatics::ApplyDamage(TargetActor, DamageToDeal, InstigatorActor ? InstigatorActor->GetInstigatorController() : nullptr, InstigatorActor, nullptr);
	}

	// Optional: Consume/Remove Fire stacks if requested
	if (bInConsumeStacks)
	{
		if (UFCElementComponent* ElementComp = UFCCombatUtils::GetElementComponent(TargetActor))
		{
			ElementComp->RemoveAllStacksOfElement(EFCElement::Fire);
		}
	}

	return DamageToDeal;
}

TArray<AActor*> UFCGA_Ignite::FindNearbyEnemies(AActor* CenterActor, float InRadius)
{
	TArray<AActor*> Enemies;
	if (!CenterActor || InRadius <= 0.0f)
	{
		return Enemies;
	}

	UWorld* World = CenterActor->GetWorld();
	if (!World)
	{
		return Enemies;
	}

	const FVector Origin = CenterActor->GetActorLocation();
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(InRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(CenterActor);

	World->OverlapMultiByChannel(
		OverlapResults,
		Origin,
		FQuat::Identity,
		ECC_Pawn,
		SphereShape,
		QueryParams
	);

	for (const FOverlapResult& Overlap : OverlapResults)
	{
		if (AActor* Candidate = Overlap.GetActor())
		{
			if (!Enemies.Contains(Candidate) && IsEnemyActor(CenterActor, Candidate))
			{
				Enemies.Add(Candidate);
			}
		}
	}

	return Enemies;
}

bool UFCGA_Ignite::IsEnemyActor(AActor* SourceActor, AActor* CandidateActor)
{
	if (!CandidateActor || CandidateActor == SourceActor)
	{
		return false;
	}

	// 1. If candidate is explicitly marked as Player, ignore (no friendly fire)
	if (CandidateActor->ActorHasTag(FName("Player")))
	{
		return false;
	}

	if (const APawn* Pawn = Cast<APawn>(CandidateActor))
	{
		if (Pawn->IsPlayerControlled())
		{
			return false;
		}
	}

	// 2. Candidate has Mob / Enemy tag or is AFCMobCharacter
	if (CandidateActor->ActorHasTag(FName("Enemy")) || CandidateActor->ActorHasTag(FName("Mob")) || CandidateActor->IsA<AFCMobCharacter>())
	{
		return true;
	}

	// 3. Any actor possessing an element component or attribute set that is not player controlled
	if (UFCCombatUtils::GetElementComponent(CandidateActor) != nullptr)
	{
		return true;
	}

	return false;
}
