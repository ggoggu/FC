#include "AbilitySystem/Abilities/FCGA_SandWall.h"
#include "Combat/Wall/FCSandWall.h"
#include "Character/FCCharacterBase.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

UFCGA_SandWall::UFCGA_SandWall()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	WallClass = AFCSandWall::StaticClass();
	SpawnForwardDistance = 150.0f;
	WallDuration = 1.0f;
}

FTransform UFCGA_SandWall::CalculateSpawnTransform(const AActor* Avatar) const
{
	if (!Avatar)
	{
		return FTransform::Identity;
	}

	FVector AimDir = Avatar->GetActorForwardVector();
	const FVector BaseLocation = Avatar->GetActorLocation();

	// 1. Resolve targeting vector from active combat target or cursor aim location
	if (const AFCCharacterBase* Char = Cast<AFCCharacterBase>(Avatar))
	{
		if (const AActor* TargetActor = Char->GetCombatTarget())
		{
			const FVector ToTarget = (TargetActor->GetActorLocation() - BaseLocation).GetSafeNormal2D();
			if (!ToTarget.IsNearlyZero())
			{
				AimDir = ToTarget;
			}
		}
		else if (!Char->GetTargetAimLocation().IsZero())
		{
			const FVector ToTarget = (Char->GetTargetAimLocation() - BaseLocation).GetSafeNormal2D();
			if (!ToTarget.IsNearlyZero())
			{
				AimDir = ToTarget;
			}
		}
	}

	// 2. Position the sand wall slightly in front of the caster along target direction
	const FVector SpawnLocation = BaseLocation + (AimDir * SpawnForwardDistance);

	// 3. Orient wall broadside perpendicular to the aim vector (facing the incoming threats)
	FRotator SpawnRotation = AimDir.Rotation();
	SpawnRotation.Pitch = 0.0f;
	SpawnRotation.Roll = 0.0f;

	return FTransform(SpawnRotation, SpawnLocation);
}

void UFCGA_SandWall::ActivateAbility(
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

	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		AActor* Avatar = ActorInfo->AvatarActor.Get();

		if (Avatar->HasAuthority())
		{
			const FTransform SpawnTransform = CalculateSpawnTransform(Avatar);

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = Avatar;
			SpawnParams.Instigator = Cast<APawn>(Avatar);
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			TSubclassOf<AFCSandWall> EffectiveClass = WallClass ? WallClass : TSubclassOf<AFCSandWall>(AFCSandWall::StaticClass());
			if (AFCSandWall* WallActor = GetWorld()->SpawnActor<AFCSandWall>(EffectiveClass, SpawnTransform, SpawnParams))
			{
				WallActor->SetWallDuration(WallDuration);
			}

			// Presentation: Audio & Niagara Feedback
			if (CastSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, CastSound, SpawnTransform.GetLocation());
			}

			if (CastVFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, CastVFX, SpawnTransform.GetLocation(), SpawnTransform.GetRotation().Rotator());
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
