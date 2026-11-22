#include "Map/FCDoorBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

namespace
{
	void FitDoorMeshToBounds(UStaticMeshComponent* MeshComp, const FVector& DesiredSize, float BaseZOffset)
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

AFCDoorBase::AFCDoorBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DoorFrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrameMesh"));
	DoorFrameMesh->SetupAttachment(SceneRoot);
	DoorFrameMesh->SetCollisionProfileName(TEXT("BlockAll"));

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(SceneRoot);
	DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));

	PassageFloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PassageFloorMesh"));
	PassageFloorMesh->SetupAttachment(SceneRoot);
	PassageFloorMesh->SetCollisionProfileName(TEXT("BlockAll"));

	InteractionTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionTrigger"));
	InteractionTrigger->SetupAttachment(SceneRoot);
	InteractionTrigger->SetBoxExtent(FVector(PassageDepth * 0.5f, DoorWidth * 0.5f, DoorHeight * 0.5f));
	InteractionTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, FloorThickness + DoorHeight * 0.5f));
	InteractionTrigger->SetCollisionProfileName(TEXT("Trigger"));
}

void AFCDoorBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateDoorDimensions();
}

void AFCDoorBase::BeginPlay()
{
	Super::BeginPlay();

	UpdateDoorDimensions();

	// Initialize visual/collision state based on initial replicated values
	OnPlayLockEffect(bIsLocked);
	OnPlayOpenEffect(bIsOpen);
}

void AFCDoorBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFCDoorBase, bIsLocked, COND_None);
	DOREPLIFETIME_CONDITION(AFCDoorBase, bIsOpen, COND_None);
	DOREPLIFETIME_CONDITION(AFCDoorBase, DoorWidth, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AFCDoorBase, PassageDepth, COND_InitialOnly);
}

void AFCDoorBase::SetDoorLocked(bool bNewLocked)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!bNewLocked)
	{
		ActiveLockSources.Empty();
	}

	if (bIsLocked == bNewLocked)
	{
		return;
	}

	bIsLocked = bNewLocked;

	if (bIsLocked && bIsOpen)
	{
		bIsOpen = false;
		OnPlayOpenEffect(false);
		OnDoorOpenChanged.Broadcast(false);
	}

	OnPlayLockEffect(bIsLocked);
	OnDoorLockChanged.Broadcast(bIsLocked);
}

void AFCDoorBase::SetDoorLockedBySource(AActor* LockSource, bool bLock)
{
	if (!HasAuthority())
	{
		return;
	}

	for (auto It = ActiveLockSources.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}

	if (LockSource)
	{
		if (bLock)
		{
			ActiveLockSources.Add(LockSource);
		}
		else
		{
			ActiveLockSources.Remove(LockSource);
		}

		const bool bShouldBeLocked = ActiveLockSources.Num() > 0;
		SetDoorLocked(bShouldBeLocked);
		if (!bShouldBeLocked)
		{
			SetDoorOpen(true);
		}
	}
	else
	{
		SetDoorLocked(bLock);
		if (!bLock)
		{
			SetDoorOpen(true);
		}
	}
}

int32 AFCDoorBase::GetActiveLockCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& Source : ActiveLockSources)
	{
		if (Source.IsValid())
		{
			++Count;
		}
	}
	return Count;
}

void AFCDoorBase::SetDoorOpen(bool bNewOpen)
{
	if (!HasAuthority() || bIsOpen == bNewOpen || (bIsLocked && bNewOpen))
	{
		return;
	}

	bIsOpen = bNewOpen;
	OnPlayOpenEffect(bIsOpen);
	OnDoorOpenChanged.Broadcast(bIsOpen);
}

void AFCDoorBase::OnPlayLockEffect_Implementation(bool bLocked)
{
	if (DoorMesh)
	{
		if (bLocked)
		{
			// When locked, door blocks passage
			DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			DoorMesh->SetVisibility(true);
		}
		else
		{
			// When unlocked, collision and visibility reflect whether the door is open
			DoorMesh->SetCollisionEnabled(bIsOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
			DoorMesh->SetVisibility(!bIsOpen);
		}
	}

	USoundBase* SoundToPlay = bLocked ? LockSound : UnlockSound;
	if (SoundToPlay)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, GetActorLocation());
	}
}

void AFCDoorBase::OnPlayOpenEffect_Implementation(bool bOpen)
{
	if (DoorMesh)
	{
		DoorMesh->SetCollisionEnabled(bOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		DoorMesh->SetVisibility(!bOpen);
	}

	if (bOpen && OpenSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, OpenSound, GetActorLocation());
	}
}

void AFCDoorBase::OnRep_IsLocked()
{
	OnPlayLockEffect(bIsLocked);
	OnDoorLockChanged.Broadcast(bIsLocked);
}

void AFCDoorBase::OnRep_IsOpen()
{
	OnPlayOpenEffect(bIsOpen);
	OnDoorOpenChanged.Broadcast(bIsOpen);
}

void AFCDoorBase::SetDoorWidth(float NewDoorWidth)
{
	if (!HasAuthority() || !FMath::IsFinite(NewDoorWidth))
	{
		return;
	}

	const float ClampedWidth = FMath::Max(50.0f, NewDoorWidth);
	if (FMath::IsNearlyEqual(DoorWidth, ClampedWidth))
	{
		return;
	}

	DoorWidth = ClampedWidth;
	UpdateDoorDimensions();
}

void AFCDoorBase::SetPassageDepth(float NewPassageDepth)
{
	if (!HasAuthority() || !FMath::IsFinite(NewPassageDepth))
	{
		return;
	}

	const float ClampedDepth = FMath::Max(50.0f, NewPassageDepth);
	if (FMath::IsNearlyEqual(PassageDepth, ClampedDepth))
	{
		return;
	}

	PassageDepth = ClampedDepth;
	UpdateDoorDimensions();
}

void AFCDoorBase::UpdateDoorDimensions()
{
	const float ClampedWidth = FMath::Max(50.0f, DoorWidth);
	const float ClampedDepth = FMath::Max(50.0f, PassageDepth);
	const float ClampedHeight = FMath::Max(100.0f, DoorHeight);
	const float ClampedFloor = FMath::Max(10.0f, FloorThickness);
	const float ClampedGateDepth = FMath::Clamp(DoorGateThickness, 10.0f, ClampedDepth);

	if (PassageFloorMesh)
	{
		FitDoorMeshToBounds(PassageFloorMesh, FVector(ClampedDepth, ClampedWidth, ClampedFloor), 0.0f);
	}

	if (DoorMesh)
	{
		FitDoorMeshToBounds(DoorMesh, FVector(ClampedGateDepth, ClampedWidth, ClampedHeight), ClampedFloor);
	}

	if (InteractionTrigger)
	{
		InteractionTrigger->SetBoxExtent(FVector(ClampedDepth * 0.5f, ClampedWidth * 0.5f, ClampedHeight * 0.5f));
		InteractionTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, ClampedFloor + ClampedHeight * 0.5f));
	}
}

void AFCDoorBase::OnRep_DoorWidth()
{
	UpdateDoorDimensions();
}
