#include "Map/FCWallBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"

AFCWallBase::AFCWallBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
	WallMesh->SetupAttachment(SceneRoot);
	WallMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void AFCWallBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFCWallBase, WallDimensions, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AFCWallBase, WallBaseZOffset, COND_InitialOnly);
}

void AFCWallBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyWallDimensions();
}

void AFCWallBase::BeginPlay()
{
	Super::BeginPlay();
	ApplyWallDimensions();
}

void AFCWallBase::SetWallDimensions(const FVector& InDimensions, float InBaseZOffset)
{
	if (!HasAuthority())
	{
		return;
	}

	if (InDimensions.ContainsNaN() || !FMath::IsFinite(InBaseZOffset))
	{
		return;
	}

	WallDimensions = FVector(
		FMath::Max(0.0f, InDimensions.X),
		FMath::Max(0.0f, InDimensions.Y),
		FMath::Max(0.0f, InDimensions.Z)
	);
	WallBaseZOffset = InBaseZOffset;
	ApplyWallDimensions();
}

void AFCWallBase::ApplyWallDimensions()
{
	if (!WallMesh)
	{
		return;
	}

	if (WallDimensions.X <= KINDA_SMALL_NUMBER || WallDimensions.Y <= KINDA_SMALL_NUMBER || WallDimensions.Z <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	FBox MeshBounds(FVector(-50.0f, -50.0f, -50.0f), FVector(50.0f, 50.0f, 50.0f));
	if (const UStaticMesh* StaticMesh = WallMesh->GetStaticMesh())
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
		WallDimensions.X / MeshSize.X,
		WallDimensions.Y / MeshSize.Y,
		WallDimensions.Z / MeshSize.Z
	);
	const FVector TargetRelLocation(
		-MeshCenter.X * TargetScale.X,
		-MeshCenter.Y * TargetScale.Y,
		WallBaseZOffset - MeshBounds.Min.Z * TargetScale.Z
	);

	WallMesh->SetRelativeScale3D(TargetScale);
	WallMesh->SetRelativeLocation(TargetRelLocation);
}

void AFCWallBase::OnRep_WallDimensions()
{
	ApplyWallDimensions();
}
