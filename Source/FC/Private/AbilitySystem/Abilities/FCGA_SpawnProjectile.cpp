#include "AbilitySystem/Abilities/FCGA_SpawnProjectile.h"
#include "Combat/Projectile/FCProjectileBase.h"
#include "Combat/Projectile/FCProjectileDataAsset.h"
#include "Combat/Interfaces/FCCombatTargetInterface.h"
#include "Combat/Interfaces/FCProjectileProviderInterface.h"
#include "Card/FCCardExecutionContext.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/FCGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Character/FCCharacterBase.h"
#include "AIController.h"

UFCGA_SpawnProjectile::UFCGA_SpawnProjectile()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ProjectileClass = AFCProjectileBase::StaticClass();
	ClassTag = FCGameplayTags::Class_Mage;
	CardTypeTag = FCGameplayTags::Card_Type_Attack;
}

USceneComponent* UFCGA_SpawnProjectile::GetLaunchReferenceComponent(AActor* Avatar) const
{
	if (!Avatar)
	{
		return nullptr;
	}

	TInlineComponentArray<USceneComponent*> SceneComponents;
	Avatar->GetComponents(SceneComponents);
	for (USceneComponent* Comp : SceneComponents)
	{
		if (Comp && Comp->GetName().Contains(TEXT("Arrow"), ESearchCase::IgnoreCase))
		{
			return Comp;
		}
	}

	if (UArrowComponent* ArrowComp = Avatar->FindComponentByClass<UArrowComponent>())
	{
		return ArrowComp;
	}

	return nullptr;
}

FTransform UFCGA_SpawnProjectile::GetLaunchTransform(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* TriggerEventData) const
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return FTransform::Identity;
	}

	AActor* Avatar = ActorInfo->AvatarActor.Get();

	FVector ForwardVector = Avatar->GetActorForwardVector();
	FVector RightVector = Avatar->GetActorRightVector();
	FVector UpVector = Avatar->GetActorUpVector();
	FRotator SpawnRotation = Avatar->GetActorRotation();
	FVector BaseLocation = Avatar->GetActorLocation();

	if (const IFCCombatTargetInterface* AvatarCombatTarget = Cast<IFCCombatTargetInterface>(Avatar))
	{
		BaseLocation = AvatarCombatTarget->GetCombatAimLocation();
	}
	else if (const ACharacter* Character = Cast<ACharacter>(Avatar))
	{
		if (const USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			BaseLocation = Mesh->DoesSocketExist(FName("spine_03"))
				? Mesh->GetSocketLocation(FName("spine_03"))
				: Avatar->GetActorLocation() + FVector(0.f, 0.f, 50.f);
		}
	}
	else if (const USceneComponent* LaunchRefComp = GetLaunchReferenceComponent(Avatar))
	{
		ForwardVector = LaunchRefComp->GetForwardVector();
		RightVector = LaunchRefComp->GetRightVector();
		UpVector = LaunchRefComp->GetUpVector();
		SpawnRotation = LaunchRefComp->GetComponentRotation();
		BaseLocation = LaunchRefComp->GetComponentLocation();
	}

	const FVector SpawnLocation = BaseLocation
		+ (ForwardVector * MuzzleOffset.X)
		+ (RightVector * MuzzleOffset.Y)
		+ (UpVector * MuzzleOffset.Z);

	// 1. Prefer immutable per-invocation FFCCardAbilityTargetData (eliminates shared mutable target races)
	AActor* TargetActor = nullptr;
	FVector TargetAimLocation = FVector::ZeroVector;

	if (const FFCCardAbilityTargetData* CardTargetData = TriggerEventData ? FFCCardAbilityTargetData::ExtractFromHandle(TriggerEventData->TargetData) : nullptr)
	{
		TargetAimLocation = CardTargetData->AimLocation;
		if (CardTargetData->VerifiedTargets.Num() > 0)
		{
			TargetActor = CardTargetData->VerifiedTargets[0].Get();
		}
	}

	if (!TargetActor && TargetAimLocation.IsZero())
	{
		if (const AFCCharacterBase* Char = Cast<AFCCharacterBase>(Avatar))
		{
			TargetActor = Char->GetCombatTarget();
			TargetAimLocation = Char->GetTargetAimLocation();
		}
	}

	if (!TargetActor)
	{
		if (const APawn* Pawn = Cast<APawn>(Avatar))
		{
			if (const AAIController* AIC = Cast<AAIController>(Pawn->GetController()))
			{
				TargetActor = AIC->GetFocusActor();
			}
		}
	}

	if (TargetActor)
	{
		if (const IFCCombatTargetInterface* TargetCombat = Cast<IFCCombatTargetInterface>(TargetActor))
		{
			TargetAimLocation = TargetCombat->GetCombatAimLocation();
		}
		else
		{
			TargetAimLocation = TargetActor->GetActorLocation();
		}
	}
	else if (!TargetAimLocation.IsZero())
	{
		TargetAimLocation.Z = SpawnLocation.Z;
	}

	if (!TargetAimLocation.IsZero())
	{
		const FVector AimDirection = (TargetAimLocation - SpawnLocation).GetSafeNormal();
		if (!AimDirection.IsNearlyZero())
		{
			FRotator AimRotator = AimDirection.Rotation();
			AimRotator.Pitch = FMath::ClampAngle(AimRotator.Pitch, -35.0f, 35.0f);
			AimRotator.Roll = 0.0f;
			SpawnRotation = AimRotator;
		}
	}

	return FTransform(SpawnRotation, SpawnLocation);
}

void UFCGA_SpawnProjectile::ActivateAbility(
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

	// 1. Resolve Projectile DataAsset & Class Override via polymorphic IFCProjectileProviderInterface (DIP)
	const UFCProjectileDataAsset* EffectiveDataAsset = ProjectileDataAsset.Get();
	TSubclassOf<AFCProjectileBase> ExplicitClassOverride = nullptr;

	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		if (const IFCProjectileProviderInterface* Provider = Cast<IFCProjectileProviderInterface>(ActorInfo->AvatarActor.Get()))
		{
			if (TSubclassOf<AFCProjectileBase> ClassOverride = Provider->GetProjectileClassOverrideForAbility())
			{
				ExplicitClassOverride = ClassOverride;
			}
			if (const UFCProjectileDataAsset* AssetOverride = Provider->GetProjectileDataAssetOverrideForAbility())
			{
				EffectiveDataAsset = AssetOverride;
			}
		}
	}

	// Check SourceObject from CardDataAsset or directly passed UFCProjectileDataAsset
	if (!ExplicitClassOverride && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (const FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle))
		{
			if (const UFCProjectileDataAsset* DirectProjAsset = Cast<UFCProjectileDataAsset>(Spec->SourceObject.Get()))
			{
				EffectiveDataAsset = DirectProjAsset;
			}
			else if (const UFCCardDataAsset* CardAsset = Cast<UFCCardDataAsset>(Spec->SourceObject.Get()))
			{
				if (UFCProjectileDataAsset* CardProjAsset = CardAsset->GameplayData.GetProjectileDataAsset())
				{
					EffectiveDataAsset = CardProjAsset;
				}
			}
		}
	}

	if (!ExplicitClassOverride && !EffectiveDataAsset && TriggerEventData)
	{
		EffectiveDataAsset = [&]() -> const UFCProjectileDataAsset*
		{
			const FFCCardAbilityTargetData* CardTargetData = FFCCardAbilityTargetData::ExtractFromHandle(TriggerEventData->TargetData);
			if (!CardTargetData || CardTargetData->CardId.IsNone())
			{
				return nullptr;
			}

			const UObject* WorldContextObj = (ActorInfo && ActorInfo->AvatarActor.IsValid())
				? static_cast<const UObject*>(ActorInfo->AvatarActor.Get())
				: static_cast<const UObject*>(this);

			const UFCCardSubsystem* CardSubsystem = UFCCardSubsystem::GetCardSubsystem(WorldContextObj);
			const UFCCardDataAsset* CardAsset = CardSubsystem ? CardSubsystem->GetCardDataAsset(CardTargetData->CardId) : nullptr;
			return CardAsset ? CardAsset->GameplayData.GetProjectileDataAsset() : nullptr;
		}();
	}

	// 2. Synchronize flight & presentation parameters from DataAsset
	float EffectiveLaunchSpeed = LaunchSpeed;
	float EffectiveBaseDamage = BaseDamage;
	float EffectiveAttackPowerScale = 1.0f;
	TArray<FGameplayTag> EffectiveElementTags = ProjectileElementTags;
	FGameplayTag EffectiveClassTag = ClassTag.IsValid() ? ClassTag : FCGameplayTags::Class_Mage.GetTag();
	FGameplayTag EffectiveCardTypeTag = CardTypeTag.IsValid() ? CardTypeTag : FCGameplayTags::Card_Type_Attack.GetTag();
	USoundBase* EffectiveCastSound = CastSound;
	UNiagaraSystem* EffectiveCastVFX = CastVFX;

	if (EffectiveDataAsset)
	{
		EffectiveLaunchSpeed = EffectiveDataAsset->LaunchSpeed;
		EffectiveBaseDamage = EffectiveDataAsset->Damage;
		EffectiveAttackPowerScale = EffectiveDataAsset->AttackPowerScale;
		EffectiveElementTags = EffectiveDataAsset->ProjectileElementTags;
		if (EffectiveDataAsset->SourceClassTag.IsValid())
		{
			EffectiveClassTag = EffectiveDataAsset->SourceClassTag;
		}
		if (EffectiveDataAsset->SourceCardTypeTag.IsValid())
		{
			EffectiveCardTypeTag = EffectiveDataAsset->SourceCardTypeTag;
		}

		if (USoundBase* LoadedCastSound = EffectiveDataAsset->CastSound.LoadSynchronous())
		{
			EffectiveCastSound = LoadedCastSound;
		}
		if (UNiagaraSystem* LoadedCastVFX = EffectiveDataAsset->CastVFX.LoadSynchronous())
		{
			EffectiveCastVFX = LoadedCastVFX;
		}
	}

	const FTransform LaunchTransform = GetLaunchTransform(ActorInfo, TriggerEventData);
	const FVector SpawnLocation = LaunchTransform.GetLocation();
	const FRotator SpawnRotation = LaunchTransform.Rotator();

	// 3. Authoritative Projectile Spawning on Server
	TSubclassOf<AFCProjectileBase> ClassToSpawn = ExplicitClassOverride;
	if (!ClassToSpawn)
	{
		ClassToSpawn = (EffectiveDataAsset && EffectiveDataAsset->ProjectileClass)
			? EffectiveDataAsset->ProjectileClass
			: ProjectileClass;
	}
	if (!ClassToSpawn)
	{
		ClassToSpawn = AFCProjectileBase::StaticClass();
	}

	if (HasAuthority(&ActivationInfo) && ClassToSpawn)
	{
		UWorld* World = GetWorld();
		if (World && ActorInfo->AvatarActor.IsValid())
		{
			AActor* Avatar = ActorInfo->AvatarActor.Get();
			APawn* InstigatorPawn = Cast<APawn>(Avatar);

			AFCProjectileBase* Projectile = World->SpawnActorDeferred<AFCProjectileBase>(
				ClassToSpawn,
				LaunchTransform,
				ActorInfo->OwnerActor.Get(),
				InstigatorPawn,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn
			);

			if (Projectile)
			{
				if (EffectiveDataAsset)
				{
					Projectile->InitializeFromDataAsset(EffectiveDataAsset);
				}

				// AttackPower scaling is evaluated cleanly inside UFCExecCalc_Damage upon impact
				Projectile->SetDamage(EffectiveBaseDamage);
				Projectile->SetAttackPowerScale(EffectiveAttackPowerScale);

				if (EffectiveElementTags.Num() > 0)
				{
					Projectile->SetProjectileElementTags(EffectiveElementTags);
				}
				Projectile->SetSourceClassTag(EffectiveClassTag);
				Projectile->SetSourceCardTypeTag(EffectiveCardTypeTag);

				if (UProjectileMovementComponent* MoveComp = Projectile->GetProjectileMovement())
				{
					MoveComp->InitialSpeed = EffectiveLaunchSpeed;
					MoveComp->MaxSpeed = EffectiveLaunchSpeed;
					MoveComp->ProjectileGravityScale = EffectiveDataAsset ? EffectiveDataAsset->GravityScale : 0.0f;
					MoveComp->Velocity = LaunchTransform.GetRotation().GetForwardVector() * EffectiveLaunchSpeed;
					MoveComp->bInitialVelocityInLocalSpace = false;
				}

				UGameplayStatics::FinishSpawningActor(Projectile, LaunchTransform);
			}
		}
	}

	// 4. Presentation Audio & Visuals
	if (EffectiveCastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EffectiveCastSound, SpawnLocation);
	}

	if (EffectiveCastVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, EffectiveCastVFX, SpawnLocation, SpawnRotation);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
