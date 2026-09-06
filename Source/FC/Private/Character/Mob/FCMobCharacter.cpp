#include "Character/Mob/FCMobCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Controller/AI/FCMobAIController.h"
#include "Gameplay/Spawner/FCMobSpawnerBase.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "BrainComponent.h"

AFCMobCharacter::AFCMobCharacter()
{
	// Pure base class: no hardcoded attack ability or montage by default (configured per BP/subclass)
	AttackAbilityClass = nullptr;
	AttackMontage = nullptr;

	// Load default directional death animations from Character/Mannequins/Anims/Death
	static ConstructorHelpers::FObjectFinder<UAnimSequence> FrontDeathFinder(
		TEXT("/Game/Character/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01"));
	if (FrontDeathFinder.Succeeded())
	{
		DeathAnim_Front = FrontDeathFinder.Object;
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

	// Calculate hit direction and multicast directional death animation
	const EFCDeathDirection DeathDir = CalculateHitDirection(Killer);
	Multicast_PlayDeathAnimation(DeathDir);

	// Notify spawner
	if (OwningSpawner.IsValid())
	{
		OwningSpawner->HandleMobDied(this, Killer);
	}

	// Schedule cleanup
	SetLifeSpan(DeathDespawnDelay);
}

EFCDeathDirection AFCMobCharacter::CalculateHitDirection(AActor* InstigatorActor) const
{
	if (!InstigatorActor)
	{
		return EFCDeathDirection::Front;
	}

	const FVector ToInstigator = (InstigatorActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToInstigator.IsNearlyZero())
	{
		return EFCDeathDirection::Front;
	}

	// Convert to character's local coordinate space
	const FVector LocalDir = GetActorRotation().UnrotateVector(ToInstigator);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(LocalDir.Y, LocalDir.X));

	// -45 to +45: Front
	// +45 to +135: Right
	// -135 to -45: Left
	// > 135 or < -135: Back
	if (AngleDegrees >= -45.0f && AngleDegrees <= 45.0f)
	{
		return EFCDeathDirection::Front;
	}
	else if (AngleDegrees > 45.0f && AngleDegrees <= 135.0f)
	{
		return EFCDeathDirection::Right;
	}
	else if (AngleDegrees < -45.0f && AngleDegrees >= -135.0f)
	{
		return EFCDeathDirection::Left;
	}
	else
	{
		return EFCDeathDirection::Back;
	}
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
		return DeathAnim_Front;
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

