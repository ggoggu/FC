#include "Map/FCDestructibleWall.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"

namespace
{
	void FitMeshComponentToBounds(UStaticMeshComponent* MeshComp, const FVector& DesiredSize, float BaseZOffset)
	{
		if (!MeshComp || DesiredSize.X <= KINDA_SMALL_NUMBER || DesiredSize.Y <= KINDA_SMALL_NUMBER || DesiredSize.Z <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		FBox MeshBounds(FVector(-50.0f, -50.0f, -50.0f), FVector(50.0f, 50.0f, 50.0f));
		if (const UStaticMesh* StaticMesh = MeshComp->GetStaticMesh())
		{
			const FBox AssetBounds = StaticMesh->GetBoundingBox();
			if (AssetBounds.IsValid)
			{
				MeshBounds = AssetBounds;
			}
		}

		const FVector MeshSize = MeshBounds.GetSize();
		if (MeshSize.X <= KINDA_SMALL_NUMBER || MeshSize.Y <= KINDA_SMALL_NUMBER || MeshSize.Z <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const FVector MeshCenter = MeshBounds.GetCenter();
		const FVector TargetScale(
			DesiredSize.X / MeshSize.X,
			DesiredSize.Y / MeshSize.Y,
			DesiredSize.Z / MeshSize.Z
		);
		const FVector TargetRelLocation(
			-MeshCenter.X * TargetScale.X,
			-MeshCenter.Y * TargetScale.Y,
			BaseZOffset - MeshBounds.Min.Z * TargetScale.Z
		);

		MeshComp->SetRelativeScale3D(TargetScale);
		MeshComp->SetRelativeLocation(TargetRelLocation);
	}
}

AFCDestructibleWall::AFCDestructibleWall()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	IntactWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IntactWallMesh"));
	IntactWallMesh->SetupAttachment(SceneRoot);
	IntactWallMesh->SetCollisionProfileName(TEXT("BlockAll"));

	PassageFloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PassageFloorMesh"));
	PassageFloorMesh->SetupAttachment(SceneRoot);
	PassageFloorMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void AFCDestructibleWall::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyWallDimensions();
}

void AFCDestructibleWall::BeginPlay()
{
	Super::BeginPlay();

	ApplyWallDimensions();

	if (bIsDestroyed)
	{
		OnPlayDestructionCosmetics();
	}
}

void AFCDestructibleWall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFCDestructibleWall, bIsDestroyed, COND_None);
	DOREPLIFETIME_CONDITION(AFCDestructibleWall, WallDimensions, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AFCDestructibleWall, FloorThickness, COND_InitialOnly);
}

void AFCDestructibleWall::SetWallDimensions(const FVector& InDimensions, float InFloorThickness)
{
	if (!HasAuthority())
	{
		return;
	}

	if (InDimensions.ContainsNaN() || !FMath::IsFinite(InFloorThickness))
	{
		return;
	}

	WallDimensions = FVector(
		FMath::Max(0.0f, InDimensions.X),
		FMath::Max(0.0f, InDimensions.Y),
		FMath::Max(0.0f, InDimensions.Z)
	);
	FloorThickness = FMath::Max(0.0f, InFloorThickness);
	ApplyWallDimensions();
}

void AFCDestructibleWall::ApplyWallDimensions()
{
	if (WallDimensions.X <= KINDA_SMALL_NUMBER || WallDimensions.Y <= KINDA_SMALL_NUMBER || WallDimensions.Z <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float ClampedFloorHeight = FMath::Clamp(FloorThickness, 0.0f, FMath::Max(0.0f, WallDimensions.Z - 10.0f));

	if (PassageFloorMesh && ClampedFloorHeight > KINDA_SMALL_NUMBER)
	{
		FitMeshComponentToBounds(PassageFloorMesh, FVector(WallDimensions.X, WallDimensions.Y, ClampedFloorHeight), 0.0f);
	}

	if (IntactWallMesh)
	{
		const float WallBodyHeight = FMath::Max(10.0f, WallDimensions.Z - ClampedFloorHeight);
		FitMeshComponentToBounds(IntactWallMesh, FVector(WallDimensions.X, WallDimensions.Y, WallBodyHeight), ClampedFloorHeight);
	}
}

void AFCDestructibleWall::OnRep_WallDimensions()
{
	ApplyWallDimensions();
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
