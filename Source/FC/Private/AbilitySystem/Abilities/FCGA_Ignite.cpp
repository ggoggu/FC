#include "AbilitySystem/Abilities/FCGA_Ignite.h"
#include "Combat/FCCombatUtils.h"
#include "Combat/Element/FCElementComponent.h"
#include "Card/FCCardExecutionContext.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/FCGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "AbilitySystem/Effects/FCGE_Damage.h"
#include "Character/FCCharacterBase.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/Pawn.h"

UFCGA_Ignite::UFCGA_Ignite()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	TargetElementTag = FCGameplayTags::Element_Fire;
	DamagePerStack = 10.0f;
	Radius = 1000.0f;
	bConsumeFireStacks = false;
	DamageEffectClass = UFCGE_Damage::StaticClass();
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

	if (HasAuthority(&ActivationInfo))
	{
		TArray<AActor*> Targets;
		if (TriggerEventData && TriggerEventData->TargetData.Num() > 0)
		{
			if (const FGameplayAbilityTargetData* RawData = TriggerEventData->TargetData.Get(0))
			{
				for (const TWeakObjectPtr<AActor>& TargetPtr : RawData->GetActors())
				{
					if (AActor* Target = TargetPtr.Get())
					{
						Targets.AddUnique(Target);
					}
				}
			}
		}

		if (Targets.Num() == 0)
		{
			Targets = FindNearbyEnemies(Avatar, Radius);
		}

		const FGameplayTag EffectiveTag = TargetElementTag.IsValid() ? TargetElementTag : FCGameplayTags::Element_Fire;
		for (AActor* Enemy : Targets)
		{
			if (Enemy)
			{
				const float AppliedDamage = ApplyIgniteToTarget(
					Avatar,
					Enemy,
					EffectiveDamagePerStack,
					bConsumeFireStacks,
					DamageEffectClass,
					EffectiveTag);

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
	TSubclassOf<UGameplayEffect> InDamageEffectClass,
	FGameplayTag InElementTag)
{
	if (!TargetActor || InDamagePerStack <= 0.0f)
	{
		return 0.0f;
	}

	if (TargetActor->GetWorld() && !TargetActor->HasAuthority())
	{
		return 0.0f;
	}

	const FGameplayTag EffectiveTag = InElementTag.IsValid() ? InElementTag : FCGameplayTags::Element_Fire;
	UFCElementComponent* ElementComp = UFCCombatUtils::GetElementComponent(TargetActor);
	const int32 StackCount = ElementComp ? ElementComp->GetElementTagCount(EffectiveTag) : 0;
	if (StackCount <= 0)
	{
		return 0.0f;
	}

	const float DamageToDeal = static_cast<float>(StackCount) * InDamagePerStack;

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

	// Route through unified GAS IncomingDamage pipeline when ASC is live, or unified AttributeSet helper in headless tests
	if (TargetASC && TargetASC->GetWorld() && InDamageEffectClass)
	{
		FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
		Context.AddInstigator(InstigatorActor, InstigatorActor);

		if (FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(InDamageEffectClass, 1.0f, Context); SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(FCGameplayTags::Data_Damage, DamageToDeal);
			TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
	else if (AttributeSet)
	{
		AttributeSet->ApplyIncomingDamageValue(DamageToDeal, InstigatorActor, InstigatorActor);
	}
	else
	{
		UGameplayStatics::ApplyDamage(TargetActor, DamageToDeal, InstigatorActor ? InstigatorActor->GetInstigatorController() : nullptr, InstigatorActor, nullptr);
	}

	if (bInConsumeStacks && ElementComp)
	{
		ElementComp->RemoveAllStacksOfElementTag(EffectiveTag);
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
	const FCollisionShape SphereShape = FCollisionShape::MakeSphere(InRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(CenterActor);

	World->OverlapMultiByChannel(
		OverlapResults,
		Origin,
		FQuat::Identity,
		ECC_Pawn,
		SphereShape,
		QueryParams);

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

	// Single SSOT targetability via polymorphic IFCCombatTargetInterface
	if (UFCCombatUtils::IsAttackableTarget(SourceActor, CandidateActor))
	{
		return true;
	}

	return UFCCombatUtils::GetElementComponent(CandidateActor) != nullptr;
}
