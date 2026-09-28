#include "Map/FCDestructibleWall.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"

AFCDestructibleWall::AFCDestructibleWall()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	IntactWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IntactWallMesh"));
	IntactWallMesh->SetupAttachment(SceneRoot);
	IntactWallMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void AFCDestructibleWall::BeginPlay()
{
	Super::BeginPlay();

	if (bIsDestroyed)
	{
		OnPlayDestructionCosmetics();
	}
}

void AFCDestructibleWall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFCDestructibleWall, bIsDestroyed, COND_None);
}

bool AFCDestructibleWall::ReceiveBombDamage_Implementation(float DamageAmount, const FVector& HitLocation, AActor* DamageCauser)
{
	if (!HasAuthority() || bIsDestroyed)
	{
		return false;
	}

	bIsDestroyed = true;

	if (IntactWallMesh)
	{
		IntactWallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	OnPlayDestructionCosmetics();
	OnWallDestroyed.Broadcast(this);

	return true;
}

void AFCDestructibleWall::OnPlayDestructionCosmetics_Implementation()
{
	if (IntactWallMesh)
	{
		IntactWallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		IntactWallMesh->SetVisibility(false);
	}

	if (DestructionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DestructionSound, GetActorLocation());
	}

	if (DestructionVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, DestructionVFX, GetActorLocation());
	}
}

void AFCDestructibleWall::OnRep_IsDestroyed()
{
	OnPlayDestructionCosmetics();
	OnWallDestroyed.Broadcast(this);
}
