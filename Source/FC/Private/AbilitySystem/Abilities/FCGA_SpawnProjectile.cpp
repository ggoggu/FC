#include "AbilitySystem/Abilities/FCGA_SpawnProjectile.h"
#include "Combat/Projectile/FCProjectileBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

UFCGA_SpawnProjectile::UFCGA_SpawnProjectile()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

FTransform UFCGA_SpawnProjectile::GetLaunchTransform(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return FTransform::Identity;
	}

	AActor* Avatar = ActorInfo->AvatarActor.Get();
	const FVector ForwardVector = Avatar->GetActorForwardVector();
	const FVector RightVector = Avatar->GetActorRightVector();
	const FVector UpVector = Avatar->GetActorUpVector();

	const FVector SpawnLocation = Avatar->GetActorLocation()
		+ (ForwardVector * MuzzleOffset.X)
		+ (RightVector * MuzzleOffset.Y)
		+ (UpVector * MuzzleOffset.Z);

	const FRotator SpawnRotation = Avatar->GetActorRotation();

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

	const FTransform LaunchTransform = GetLaunchTransform(ActorInfo);
	const FVector SpawnLocation = LaunchTransform.GetLocation();
	const FRotator SpawnRotation = LaunchTransform.Rotator();

	// Authoritative Projectile Spawning on Server
	if (HasAuthority(&ActivationInfo) && ProjectileClass)
	{
		UWorld* World = GetWorld();
		if (World && ActorInfo->AvatarActor.IsValid())
		{
			AActor* Avatar = ActorInfo->AvatarActor.Get();
			APawn* InstigatorPawn = Cast<APawn>(Avatar);

			AFCProjectileBase* Projectile = World->SpawnActorDeferred<AFCProjectileBase>(
				ProjectileClass,
				LaunchTransform,
				ActorInfo->OwnerActor.Get(),
				InstigatorPawn,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn
			);

			if (Projectile)
			{
				Projectile->SetDamage(BaseDamage);

				if (UProjectileMovementComponent* MoveComp = Projectile->GetProjectileMovement())
				{
					MoveComp->InitialSpeed = LaunchSpeed;
					MoveComp->MaxSpeed = LaunchSpeed;
					MoveComp->ProjectileGravityScale = 0.0f; // Maintain straight-line flight
					MoveComp->Velocity = LaunchTransform.GetRotation().GetForwardVector() * LaunchSpeed;
				}

				UGameplayStatics::FinishSpawningActor(Projectile, LaunchTransform);
			}
		}
	}

	// Presentation Audio & Visuals
	if (CastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastSound, SpawnLocation);
	}

	if (CastVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, CastVFX, SpawnLocation, SpawnRotation);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
