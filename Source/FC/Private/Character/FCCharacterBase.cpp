#include "Character/FCCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Combat/Element/FCElementComponent.h"
#include "Data/FCGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "Net/UnrealNetwork.h"

AFCCharacterBase::AFCCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
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

	DOREPLIFETIME_CONDITION(AFCCharacterBase, bIsDead, COND_None);
	DOREPLIFETIME_CONDITION(AFCCharacterBase, DeathDirection, COND_None);
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
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	bIsDead = true;
	DeathDirection = CalculateHitDirection(Killer);

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

	PlayDeathAnimation(DeathDirection);
}

void AFCCharacterBase::OnRep_IsDead()
{
	if (bIsDead)
	{
		OnDeath.Broadcast(this, nullptr);

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

		PlayDeathAnimation(DeathDirection);
	}
}

void AFCCharacterBase::OnRep_DeathDirection()
{
	if (bIsDead)
	{
		PlayDeathAnimation(DeathDirection);
	}
}

void AFCCharacterBase::PlayDeathAnimation(EFCDeathDirection Direction)
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

UAnimSequence* AFCCharacterBase::GetDeathAnimationForDirection(EFCDeathDirection Direction) const
{
	if (Direction == EFCDeathDirection::Front)
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
			return FrontAnims[FMath::RandRange(0, FrontAnims.Num() - 1)];
		}
		return DeathAnim_Front;
	}

	const TStaticArray<UAnimSequence*, 4> DeathAnims = {
		DeathAnim_Front.Get(),
		DeathAnim_Back.Get(),
		DeathAnim_Left.Get(),
		DeathAnim_Right.Get()
	};
	const int32 DirIdx = static_cast<int32>(Direction);
	return (DirIdx >= 0 && DirIdx < 4) ? DeathAnims[DirIdx] : DeathAnim_Front.Get();
}

void AFCCharacterBase::PlayHitAnimation(EFCDeathDirection Direction)
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

UAnimSequence* AFCCharacterBase::GetHitAnimationForDirection(EFCDeathDirection Direction) const
{
	const TStaticArray<UAnimSequence*, 4> HitAnims = {
		HitAnim_Front.Get(),
		HitAnim_Back.Get(),
		HitAnim_Left.Get(),
		HitAnim_Right.Get()
	};
	const TStaticArray<UAnimSequence*, 4> DeathAnims = {
		DeathAnim_Front.Get(),
		DeathAnim_Back.Get(),
		DeathAnim_Left.Get(),
		DeathAnim_Right.Get()
	};

	const int32 DirIdx = static_cast<int32>(Direction);
	if (DirIdx >= 0 && DirIdx < 4)
	{
		return HitAnims[DirIdx] ? HitAnims[DirIdx] : DeathAnims[DirIdx];
	}

	return HitAnim_Front ? HitAnim_Front.Get() : DeathAnim_Front.Get();
}

void AFCCharacterBase::HandleGameplayCue(UObject* Self, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	if (GameplayCueTag.MatchesTag(FCGameplayTags::GameplayCue_Combat_HitReaction))
	{
		const int32 DirIndex = FMath::Clamp(FMath::RoundToInt(Parameters.RawMagnitude), 0, 3);
		PlayHitAnimation(static_cast<EFCDeathDirection>(DirIndex));
		return;
	}

	IGameplayCueInterface::HandleGameplayCue(Self, GameplayCueTag, EventType, Parameters);
}

void AFCCharacterBase::HandleDamageTaken(float DamageAmount, AActor* DamageCauser, const FHitResult& HitResult)
{
	OnDamageTaken.Broadcast(this, DamageAmount, DamageCauser, HitResult);

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
	const FGameplayTag HitReactionTag = FCGameplayTags::GameplayCue_Combat_HitReaction;
	if (AbilitySystemComponent && AbilitySystemComponent->AbilityActorInfo.IsValid() && HitReactionTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = DamageCauser ? DamageCauser->GetActorLocation() : GetActorLocation();
		CueParams.RawMagnitude = static_cast<float>(HitDir);
		AbilitySystemComponent->ExecuteGameplayCue(HitReactionTag, CueParams);
	}
	else
	{
		PlayHitAnimation(HitDir);
	}
}

void AFCCharacterBase::ApplyHeal(float HealAmount)
{
	if (!HasAuthority() || bIsDead || HealAmount <= 0.0f)
	{
		return;
	}

	if (AttributeSet)
	{
		const float CurrentHealth = AttributeSet->GetHealth();
		const float MaxHealth = AttributeSet->GetMaxHealth();
		const float NewHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);
		const float ActualHealed = NewHealth - CurrentHealth;

		if (ActualHealed > 0.0f)
		{
			if (GetWorld())
			{
				AttributeSet->SetHealth(NewHealth);
			}
			else
			{
				AttributeSet->InitHealth(NewHealth);
			}
			OnHealed.Broadcast(this, ActualHealed);
		}
	}
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
	if (!HasAuthority() || bIsDead)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage <= 0.0f)
	{
		return 0.0f;
	}

	if (AttributeSet)
	{
		FHitResult HitResult;
		FVector ImpulseDir = FVector::ZeroVector;
		DamageEvent.GetBestHitInfo(this, DamageCauser, HitResult, ImpulseDir);
		AActor* InstigatorActor = EventInstigator ? EventInstigator->GetPawn() : DamageCauser;
		AttributeSet->ApplyIncomingDamageValue(ActualDamage, InstigatorActor, DamageCauser, &HitResult);
	}

	return ActualDamage;
}

FGameplayTag AFCCharacterBase::GetCombatTeamTag() const
{
	return FGameplayTag::EmptyTag;
}

bool AFCCharacterBase::CanBeTargetedBy(const AActor* SourceActor) const
{
	if (bIsDead || SourceActor == this)
	{
		return false;
	}

	if (const IFCCombatTargetInterface* SourceTarget = Cast<IFCCombatTargetInterface>(SourceActor))
	{
		const FGameplayTag MyTeam = GetCombatTeamTag();
		const FGameplayTag SourceTeam = SourceTarget->GetCombatTeamTag();
		if (MyTeam.IsValid() && SourceTeam.IsValid() && MyTeam == SourceTeam)
		{
			return false;
		}
	}

	return true;
}

FVector AFCCharacterBase::GetCombatAimLocation() const
{
	if (const USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (MeshComp->DoesSocketExist(FName("spine_03")))
		{
			return MeshComp->GetSocketLocation(FName("spine_03"));
		}
	}
	return GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
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

