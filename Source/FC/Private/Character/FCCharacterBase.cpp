#include "Character/FCCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Combat/Element/FCElementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DamageEvents.h"
#include "Net/UnrealNetwork.h"

AFCCharacterBase::AFCCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UFCAttributeSet>(TEXT("AttributeSet"));

	ElementComponent = CreateDefaultSubobject<UFCElementComponent>(TEXT("ElementComponent"));
}

void AFCCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFCCharacterBase, bIsDead);
}

UAbilitySystemComponent* AFCCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UFCAttributeSet* AFCCharacterBase::GetAttributeSet() const
{
	return AttributeSet;
}

UFCElementComponent* AFCCharacterBase::GetElementComponent() const
{
	return ElementComponent;
}

void AFCCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AFCCharacterBase::Die(AActor* Killer)
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	OnDeath.Broadcast(this, Killer);

	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}
}

void AFCCharacterBase::OnRep_IsDead()
{
	if (bIsDead)
	{
		if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
		{
			CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);
		}

		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
		}

		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
			MoveComp->DisableMovement();
		}
	}
}

void AFCCharacterBase::HandleDamageTaken(float DamageAmount, AActor* DamageCauser, const FHitResult& HitResult)
{
	OnDamageTaken.Broadcast(this, DamageAmount, DamageCauser, HitResult);
}

EFCDeathDirection AFCCharacterBase::CalculateHitDirection(AActor* InstigatorActor) const
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

float AFCCharacterBase::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage <= 0.0f || bIsDead)
	{
		return 0.0f;
	}

	if (AttributeSet)
	{
		const float CurrentHealth = AttributeSet->GetHealth();
		const float NewHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, AttributeSet->GetMaxHealth());
		AttributeSet->SetHealth(NewHealth);

		if (NewHealth <= 0.0f)
		{
			Die(DamageCauser);
		}
		else
		{
			FHitResult HitResult;
			FVector ImpulseDir = FVector::ZeroVector;
			DamageEvent.GetBestHitInfo(this, DamageCauser, HitResult, ImpulseDir);
			HandleDamageTaken(ActualDamage, DamageCauser, HitResult);
		}
	}

	return ActualDamage;
}

void AFCCharacterBase::SetCombatTarget(AActor* InTarget)
{
	CombatTarget = InTarget;
}

AActor* AFCCharacterBase::GetCombatTarget() const
{
	return CombatTarget.Get();
}

void AFCCharacterBase::SetTargetAimLocation(const FVector& InLocation)
{
	TargetAimLocation = InLocation;
}

FVector AFCCharacterBase::GetTargetAimLocation() const
{
	return TargetAimLocation;
}

void AFCCharacterBase::RotateTowardsTarget(const FVector& InTargetLocation)
{
	const FVector Dir = (InTargetLocation - GetActorLocation()).GetSafeNormal2D();
	if (!Dir.IsNearlyZero())
	{
		const FRotator NewRot = FRotator(0.0f, Dir.Rotation().Yaw, 0.0f);
		SetActorRotation(NewRot);
	}
}

