#include "Character/Mob/FCMobCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Controller/AI/FCMobAIController.h"
#include "Gameplay/Spawner/FCMobSpawnerBase.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "BrainComponent.h"
#include "Gameplay/FCChestActor.h"
#include "Engine/World.h"

AFCMobCharacter::AFCMobCharacter()
{
	// Pure base class: no hardcoded attack ability or montage by default (configured per BP/subclass)
	AttackAbilityClass = nullptr;
	AttackMontage = nullptr;
	ProjectileClassOverride = nullptr;
	ProjectileDataAssetOverride = nullptr;

	// Load all directional death animations from Character/Mannequins/Anims/Death
	static ConstructorHelpers::FObjectFinder<UAnimSequence> FrontDeathFinder1(
		TEXT("/Game/Character/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01"));
	if (FrontDeathFinder1.Succeeded())
	{
		DeathAnim_Front = FrontDeathFinder1.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> FrontDeathFinder2(
		TEXT("/Game/Character/Mannequins/Anims/Death/MM_Death_Front_02.MM_Death_Front_02"));
	if (FrontDeathFinder2.Succeeded())
	{
		DeathAnim_Front_02 = FrontDeathFinder2.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> FrontDeathFinder3(
		TEXT("/Game/Character/Mannequins/Anims/Death/MM_Death_Front_03.MM_Death_Front_03"));
	if (FrontDeathFinder3.Succeeded())
	{
		DeathAnim_Front_03 = FrontDeathFinder3.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> BackDeathFinder(
		TEXT("/Game/Character/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01"));
	if (BackDeathFinder.Succeeded())
	{
		DeathAnim_Back = BackDeathFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> LeftDeathFinder(
		TEXT("/Game/Character/Mannequins/Anims/Death/MM_Death_Left_01.MM_Death_Left_01"));
	if (LeftDeathFinder.Succeeded())
	{
		DeathAnim_Left = LeftDeathFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> RightDeathFinder(
		TEXT("/Game/Character/Mannequins/Anims/Death/MM_Death_Right_01.MM_Death_Right_01"));
	if (RightDeathFinder.Succeeded())
	{
		DeathAnim_Right = RightDeathFinder.Object;
	}

	// Hit reaction animations default to the animations in Character/Mannequins/Anims/Death
	HitAnim_Front = DeathAnim_Front;
	HitAnim_Back = DeathAnim_Back;
	HitAnim_Left = DeathAnim_Left;
	HitAnim_Right = DeathAnim_Right;

	// Ensure mob rotates towards movement direction, not controller yaw
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 450.0f, 0.0f);
		MoveComp->MaxWalkSpeed = PatrolSpeed;
		MoveComp->bRequestedMoveUseAcceleration = true;
	}

	// Auto-possess with dedicated Mob AIController when placed in level or spawned at runtime
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AFCMobAIController::StaticClass();

	// Default loot drop settings
	DropChestClass = AFCChestActor::StaticClass();
	DropChestChance = 1.0f;
	DropChestOffset = FVector::ZeroVector;
	bSnapChestToGround = true;
	InitialMaxHealth = 100.0f;
	DeathDespawnDelay = 5.0f;

	// Mobs use Minimal replication mode for Gameplay Ability System
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	}
}

void AFCMobCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitAbilityActorInfo();

	if (HasAuthority() && AttributeSet)
	{
		if (InitialMaxHealth > 0.0f)
		{
			AttributeSet->SetMaxHealth(InitialMaxHealth);
			AttributeSet->SetHealth(InitialMaxHealth);
		}
	}
}

void AFCMobCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OwningSpawner.IsValid())
	{
		OwningSpawner->HandleMobDestroyed(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AFCMobCharacter::InitAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// Grant configured attack ability on server authority
		if (HasAuthority() && AttackAbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackAbilityClass, 1, INDEX_NONE, this));
		}
	}
}

bool AFCMobCharacter::TryActivateAttackAbility()
{
	if (!AbilitySystemComponent || !AttackAbilityClass)
	{
		return false;
	}

	return AbilitySystemComponent->TryActivateAbilityByClass(AttackAbilityClass);
}

void AFCMobCharacter::Die(AActor* Killer)
{
	if (bIsDead)
	{
		return;
	}

	Super::Die(Killer);

	if (HasAuthority())
	{
		// Stop AI logic
		if (AController* Cont = GetController())
		{
			if (AAIController* AICont = Cast<AAIController>(Cont))
			{
				if (UBrainComponent* BrainComp = AICont->GetBrainComponent())
				{
					BrainComp->StopLogic(TEXT("Mob Died"));
				}
			}
		}

		// Roll chance and spawn drop chest
		AttemptDropChest();

		// Notify spawner
		if (OwningSpawner.IsValid())
		{
			OwningSpawner->HandleMobDied(this, Killer);
		}

		// Schedule cleanup or destroy immediately
		if (DeathDespawnDelay <= 0.0f)
		{
			Destroy();
		}
		else
		{
			SetLifeSpan(DeathDespawnDelay);
		}
	}

	// Calculate hit direction and multicast directional death animation
	const EFCDeathDirection DeathDir = CalculateHitDirection(Killer);
	Multicast_PlayDeathAnimation(DeathDir);
}

AActor* AFCMobCharacter::AttemptDropChest()
{
	if (!HasAuthority() || !DropChestClass || DropChestChance <= 0.0f)
	{
		return nullptr;
	}

	const float Roll = FMath::FRand();
	if (Roll > DropChestChance)
	{
		return nullptr;
	}

	return SpawnDropChest();
}

AActor* AFCMobCharacter::SpawnDropChest()
{
	if (!HasAuthority() || !DropChestClass || !GetWorld())
	{
		return nullptr;
	}

	FVector SpawnLocation = GetActorLocation() + DropChestOffset;

	if (bSnapChestToGround)
	{
		FHitResult FloorHit;
		const FVector TraceStart = SpawnLocation + FVector(0.0f, 0.0f, 50.0f);
		const FVector TraceEnd = SpawnLocation - FVector(0.0f, 0.0f, 500.0f);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MobChestFloorTrace), false, this);

		if (GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
		{
			SpawnLocation = FloorHit.Location;
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParams.Instigator = GetInstigator();

	AActor* SpawnedChest = GetWorld()->SpawnActor<AActor>(DropChestClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (SpawnedChest)
	{
		OnChestDropped.Broadcast(this, SpawnedChest);
	}

	return SpawnedChest;
}

void AFCMobCharacter::PlayDeathAnimation(EFCDeathDirection Direction)
{
	UAnimSequence* AnimToPlay = GetDeathAnimationForDirection(Direction);
	if (!AnimToPlay)
	{
		return;
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->Stop();
		MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		MeshComp->PlayAnimation(AnimToPlay, /*bLooping=*/false);
	}
}

void AFCMobCharacter::Multicast_PlayDeathAnimation_Implementation(EFCDeathDirection Direction)
{
	PlayDeathAnimation(Direction);
}

UAnimSequence* AFCMobCharacter::GetDeathAnimationForDirection(EFCDeathDirection Direction) const
{
	switch (Direction)
	{
	case EFCDeathDirection::Front:
	{
		TArray<UAnimSequence*, TInlineAllocator<3>> FrontAnims;
		if (DeathAnim_Front)
		{
			FrontAnims.Add(DeathAnim_Front);
		}
		if (DeathAnim_Front_02)
		{
			FrontAnims.Add(DeathAnim_Front_02);
		}
		if (DeathAnim_Front_03)
		{
			FrontAnims.Add(DeathAnim_Front_03);
		}

		if (FrontAnims.Num() > 0)
		{
			const int32 RandIndex = FMath::RandRange(0, FrontAnims.Num() - 1);
			return FrontAnims[RandIndex];
		}
		return DeathAnim_Front;
	}
	case EFCDeathDirection::Back:
		return DeathAnim_Back;
	case EFCDeathDirection::Left:
		return DeathAnim_Left;
	case EFCDeathDirection::Right:
		return DeathAnim_Right;
	default:
		return DeathAnim_Front;
	}
}

void AFCMobCharacter::HandleDamageTaken(float DamageAmount, AActor* DamageCauser, const FHitResult& HitResult)
{
	Super::HandleDamageTaken(DamageAmount, DamageCauser, HitResult);

	if (bIsDead)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (HitReactionCooldown > 0.0f && (CurrentTime - LastHitReactTime < HitReactionCooldown))
	{
		return;
	}
	LastHitReactTime = CurrentTime;

	const EFCDeathDirection HitDir = CalculateHitDirection(DamageCauser);
	Multicast_PlayHitAnimation(HitDir);
}

void AFCMobCharacter::PlayHitAnimation(EFCDeathDirection Direction)
{
	if (bIsDead)
	{
		return;
	}

	if (HitMontage)
	{
		PlayAnimMontage(HitMontage, HitPlayRate);
		return;
	}

	UAnimSequence* AnimToPlay = GetHitAnimationForDirection(Direction);
	if (!AnimToPlay)
	{
		return;
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
		{
			AnimInstance->PlaySlotAnimationAsDynamicMontage(
				AnimToPlay,
				FName(TEXT("DefaultSlot")),
				/*BlendInTime=*/0.05f,
				/*BlendOutTime=*/0.15f,
				HitPlayRate
			);
		}
	}
}

void AFCMobCharacter::Multicast_PlayHitAnimation_Implementation(EFCDeathDirection Direction)
{
	PlayHitAnimation(Direction);
}

UAnimSequence* AFCMobCharacter::GetHitAnimationForDirection(EFCDeathDirection Direction) const
{
	switch (Direction)
	{
	case EFCDeathDirection::Front:
		return HitAnim_Front ? HitAnim_Front.Get() : DeathAnim_Front.Get();
	case EFCDeathDirection::Back:
		return HitAnim_Back ? HitAnim_Back.Get() : DeathAnim_Back.Get();
	case EFCDeathDirection::Left:
		return HitAnim_Left ? HitAnim_Left.Get() : DeathAnim_Left.Get();
	case EFCDeathDirection::Right:
		return HitAnim_Right ? HitAnim_Right.Get() : DeathAnim_Right.Get();
	default:
		return HitAnim_Front ? HitAnim_Front.Get() : DeathAnim_Front.Get();
	}
}

void AFCMobCharacter::SetOwningSpawner(AFCMobSpawnerBase* InSpawner)
{
	OwningSpawner = InSpawner;
}

AFCMobSpawnerBase* AFCMobCharacter::GetOwningSpawner() const
{
	return OwningSpawner.Get();
}

void AFCMobCharacter::SetMovementSpeed(float NewSpeed)
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = FMath::Max(0.0f, NewSpeed);
	}
}

void AFCMobCharacter::SetPatrolSpeed()
{
	SetMovementSpeed(PatrolSpeed);
}

void AFCMobCharacter::SetChaseSpeed()
{
	SetMovementSpeed(ChaseSpeed);
}

