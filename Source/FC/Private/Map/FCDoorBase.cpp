#include "Map/FCDoorBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AFCDoorBase::AFCDoorBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DoorWidth = 300.0f;
	PlacementOffsetModifier = 0.0f;

	DoorFrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrameMesh"));
	DoorFrameMesh->SetupAttachment(SceneRoot);
	DoorFrameMesh->SetCollisionProfileName(TEXT("BlockAll"));

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(SceneRoot);
	DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));

	InteractionTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionTrigger"));
	InteractionTrigger->SetupAttachment(SceneRoot);
	InteractionTrigger->SetBoxExtent(FVector(100.0f, DoorWidth * 0.5f, 200.0f));
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

	// Initialize visual/collision state based on initial replicated values
	OnPlayLockEffect(bIsLocked);
	OnPlayOpenEffect(bIsOpen);
}

void AFCDoorBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFCDoorBase, bIsLocked, COND_None);
	DOREPLIFETIME_CONDITION(AFCDoorBase, bIsOpen, COND_None);
	DOREPLIFETIME_CONDITION(AFCDoorBase, DoorWidth, COND_None);
}

void AFCDoorBase::SetDoorLocked(bool bNewLocked)
{
	if (!HasAuthority() || bIsLocked == bNewLocked)
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
	DoorWidth = FMath::Max(50.0f, NewDoorWidth);
	UpdateDoorDimensions();
}

void AFCDoorBase::UpdateDoorDimensions()
{
	if (InteractionTrigger)
	{
		InteractionTrigger->SetBoxExtent(FVector(100.0f, DoorWidth * 0.5f, 200.0f));
	}
}

void AFCDoorBase::OnRep_DoorWidth()
{
	UpdateDoorDimensions();
}
