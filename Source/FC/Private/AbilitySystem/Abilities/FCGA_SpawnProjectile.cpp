#include "AbilitySystem/Abilities/FCGA_SpawnProjectile.h"
#include "Combat/Projectile/FCProjectileBase.h"
#include "Combat/Projectile/FCProjectileDataAsset.h"
#include "Data/Card/FCCardDataAsset.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/Mob/FCMobCharacter.h"
#include "AIController.h"

UFCGA_SpawnProjectile::UFCGA_SpawnProjectile()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ProjectileClass = AFCProjectileBase::StaticClass();
}

USceneComponent* UFCGA_SpawnProjectile::GetLaunchReferenceComponent(AActor* Avatar) const
{
	if (!Avatar)
	{
		return nullptr;
	}

	// 1. Search for scene components named "Arrow" or containing "Arrow" (case-insensitive)
	TInlineComponentArray<USceneComponent*> SceneComponents;
	Avatar->GetComponents(SceneComponents);
	for (USceneComponent* Comp : SceneComponents)
	{
		if (Comp && Comp->GetName().Contains(TEXT("Arrow"), ESearchCase::IgnoreCase))
		{
			return Comp;
		}
	}

	// 2. Fallback to any UArrowComponent found by class
	if (UArrowComponent* ArrowComp = Avatar->FindComponentByClass<UArrowComponent>())
	{
		return ArrowComp;
	}

	return nullptr;
}

FTransform UFCGA_SpawnProjectile::GetLaunchTransform(const FGameplayAbilityActorInfo* ActorInfo) const
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

	if (const ACharacter* Character = Cast<ACharacter>(Avatar))
	{
		// 1. 발사 방향: 캐릭터 캡슐의 정면 (이동 방향 설정에 따라 메시가 바라보는 실제 시각적 정면과 100% 일치함)
		SpawnRotation = Avatar->GetActorRotation();
		ForwardVector = Avatar->GetActorForwardVector();
		RightVector = Avatar->GetActorRightVector();
		UpVector = Avatar->GetActorUpVector();

		// 2. 발사 위치: 바닥(Capsule Z=0) 착시를 방지하기 위해 가슴(spine_03) 높이를 기준으로 설정
		if (const USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (Mesh->DoesSocketExist(FName("spine_03")))
			{
				BaseLocation = Mesh->GetSocketLocation(FName("spine_03"));
			}
			else
			{
				BaseLocation = Avatar->GetActorLocation() + FVector(0.f, 0.f, 50.f);
			}
		}
	}
	// Fallback to Arrow Component if explicitly requested
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

	// 3. 3D 고저차 조준 (Pitch & Yaw Aiming):
	// AI 몬스터가 플레이어를 조준할 때 대상이 경사로나 언덕 등 고저차가 있는 곳에 위치하면
	// 수평(Pitch=0)으로만 발사되지 않고 대상의 가슴(spine_03)을 향해 3D 벡터로 조준을 보정합니다.
	AActor* TargetActor = nullptr;
	if (const AFCMobCharacter* MobChar = Cast<AFCMobCharacter>(Avatar))
	{
		TargetActor = MobChar->GetCombatTarget();
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
		FVector TargetAimLocation = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
		if (const ACharacter* TargetChar = Cast<ACharacter>(TargetActor))
		{
			if (const USkeletalMeshComponent* TargetMesh = TargetChar->GetMesh())
			{
				if (TargetMesh->DoesSocketExist(FName("spine_03")))
				{
					TargetAimLocation = TargetMesh->GetSocketLocation(FName("spine_03"));
				}
			}
		}

		const FVector AimDirection = (TargetAimLocation - SpawnLocation).GetSafeNormal();
		if (!AimDirection.IsNearlyZero())
		{
			FRotator AimRotator = AimDirection.Rotation();
			// Clamp pitch angle between -75 and +75 degrees to avoid abnormal extreme elevation
			AimRotator.Pitch = FMath::ClampAngle(AimRotator.Pitch, -75.0f, 75.0f);
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

	// 1. Resolve Effective Projectile Data Asset
	const UFCProjectileDataAsset* EffectiveDataAsset = ProjectileDataAsset.Get();
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (const FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle))
		{
			if (const UFCCardDataAsset* CardAsset = Cast<UFCCardDataAsset>(Spec->SourceObject.Get()))
			{
				if (CardAsset->GameplayData.ProjectileDataAsset)
				{
					EffectiveDataAsset = CardAsset->GameplayData.ProjectileDataAsset;
				}
			}
		}
	}

	// 2. Synchronize effective flight and presentation parameters if data asset exists
	float EffectiveLaunchSpeed = LaunchSpeed;
	float EffectiveBaseDamage = BaseDamage;
	TArray<EFCElement> EffectiveElements = ProjectileElements;
	EFCCharacterClass EffectiveClass = CharacterClass;
	EFCCardType EffectiveCardType = CardType;
	USoundBase* EffectiveCastSound = CastSound;
	UNiagaraSystem* EffectiveCastVFX = CastVFX;

	if (EffectiveDataAsset)
	{
		EffectiveLaunchSpeed = EffectiveDataAsset->LaunchSpeed;
		EffectiveBaseDamage = EffectiveDataAsset->Damage;
		EffectiveElements = EffectiveDataAsset->ProjectileElements;
		EffectiveClass = EffectiveDataAsset->SourceClass;
		EffectiveCardType = EffectiveDataAsset->SourceCardType;

		if (USoundBase* LoadedCastSound = EffectiveDataAsset->CastSound.LoadSynchronous())
		{
			EffectiveCastSound = LoadedCastSound;
		}
		if (UNiagaraSystem* LoadedCastVFX = EffectiveDataAsset->CastVFX.LoadSynchronous())
		{
			EffectiveCastVFX = LoadedCastVFX;
		}
	}

	const FTransform LaunchTransform = GetLaunchTransform(ActorInfo);
	const FVector SpawnLocation = LaunchTransform.GetLocation();
	const FRotator SpawnRotation = LaunchTransform.Rotator();

	// 3. Authoritative Projectile Spawning on Server
	TSubclassOf<AFCProjectileBase> ClassToSpawn = (EffectiveDataAsset && EffectiveDataAsset->ProjectileClass)
		? EffectiveDataAsset->ProjectileClass
		: ProjectileClass;

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

				float ScaledDamage = EffectiveBaseDamage;
				if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Avatar))
				{
					if (UAbilitySystemComponent* SourceASC = ASI->GetAbilitySystemComponent())
					{
						if (const UFCAttributeSet* AttrSet = SourceASC->GetSet<UFCAttributeSet>())
						{
							ScaledDamage += AttrSet->GetAttackPower();
						}
					}
				}
				else if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
				{
					if (const UFCAttributeSet* AttrSet = ActorInfo->AbilitySystemComponent->GetSet<UFCAttributeSet>())
					{
						ScaledDamage += AttrSet->GetAttackPower();
					}
				}

				Projectile->SetDamage(ScaledDamage);
				if (EffectiveElements.Num() > 0)
				{
					Projectile->SetProjectileElements(EffectiveElements);
				}
				Projectile->SetSourceClass(EffectiveClass);
				Projectile->SetSourceCardType(EffectiveCardType);

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
