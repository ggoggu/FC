#include "Character/Mob/FCMobCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Controller/AI/FCMobAIController.h"

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

void AFCMobCharacter::InitAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
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
