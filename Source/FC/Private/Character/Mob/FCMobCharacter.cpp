#include "Character/Mob/FCMobCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Controller/AI/FCMobAIController.h"
#include "Gameplay/Spawner/FCMobSpawnerBase.h"
#include "BrainComponent.h"

AFCMobCharacter::AFCMobCharacter()
{
	// Ensure mob rotates towards movement direction, not controller yaw
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 450.0f, 0.0f);
		MoveComp->MaxWalkSpeed = PatrolSpeed;
		MoveComp->bUseAccelerationForPaths = true;
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
	}
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

	// Notify spawner
	if (OwningSpawner.IsValid())
	{
		OwningSpawner->HandleMobDied(this, Killer);
	}

	// Schedule cleanup
	SetLifeSpan(DeathDespawnDelay);
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

