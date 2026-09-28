#include "Map/FCRoomBase.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Map/FCDoorBase.h"
#include "Map/FCDestructibleWall.h"
#include "Map/FCWallBase.h"
#include "Net/UnrealNetwork.h"

AFCRoomBase::AFCRoomBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	DefaultDoorClass = AFCDoorBase::StaticClass();
	DefaultSecretWallClass = AFCDestructibleWall::StaticClass();
	DefaultWallClass = AFCWallBase::StaticClass();

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Sockets based on 2400x2400 cm grid
	NorthDoorSocket = CreateDefaultSubobject<USceneComponent>(TEXT("NorthDoorSocket"));
	NorthDoorSocket->SetupAttachment(SceneRoot);
	NorthDoorSocket->SetRelativeLocation(FVector(0.0f, 1200.0f, 0.0f));
	NorthDoorSocket->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));

	SouthDoorSocket = CreateDefaultSubobject<USceneComponent>(TEXT("SouthDoorSocket"));
	SouthDoorSocket->SetupAttachment(SceneRoot);
	SouthDoorSocket->SetRelativeLocation(FVector(0.0f, -1200.0f, 0.0f));
	SouthDoorSocket->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	EastDoorSocket = CreateDefaultSubobject<USceneComponent>(TEXT("EastDoorSocket"));
	EastDoorSocket->SetupAttachment(SceneRoot);
	EastDoorSocket->SetRelativeLocation(FVector(1200.0f, 0.0f, 0.0f));
	EastDoorSocket->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

	WestDoorSocket = CreateDefaultSubobject<USceneComponent>(TEXT("WestDoorSocket"));
	WestDoorSocket->SetupAttachment(SceneRoot);
	WestDoorSocket->SetRelativeLocation(FVector(-1200.0f, 0.0f, 0.0f));
	WestDoorSocket->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	PlayerSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("PlayerSpawnPoint"));
	PlayerSpawnPoint->SetupAttachment(SceneRoot);
	PlayerSpawnPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));

	RoomBoundsTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("RoomBoundsTrigger"));
	RoomBoundsTrigger->SetupAttachment(SceneRoot);
	RoomBoundsTrigger->SetBoxExtent(FVector(1000.0f, 1000.0f, 300.0f));
	RoomBoundsTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f));
	RoomBoundsTrigger->SetCollisionProfileName(TEXT("Trigger"));
}

void AFCRoomBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && RoomBoundsTrigger)
	{
		RoomBoundsTrigger->OnComponentBeginOverlap.AddDynamic(this, &AFCRoomBase::OnRoomBoundsBeginOverlap);
	}
}

void AFCRoomBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (HasAuthority())
	{
		for (TObjectPtr<AFCDoorBase>& Door : ConnectedDoors)
		{
			if (Door)
			{
				Door->Destroy();
			}
		}
		ConnectedDoors.Empty();

		for (TObjectPtr<AFCWallBase>& Wall : SpawnedWalls)
		{
			if (Wall)
			{
				Wall->Destroy();
			}
		}
		SpawnedWalls.Empty();
	}
}

void AFCRoomBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFCRoomBase, RoomData, COND_None);
	DOREPLIFETIME_CONDITION(AFCRoomBase, RoomState, COND_None);
	DOREPLIFETIME_CONDITION(AFCRoomBase, bIsDiscovered, COND_None);
	DOREPLIFETIME_CONDITION(AFCRoomBase, bIsVisited, COND_None);
}

void AFCRoomBase::InitializeRoom(const FFCRoomData& InRoomData)
{
	if (!HasAuthority())
	{
		return;
	}

	RoomData = InRoomData;

	// Start Room is discovered and cleared by default
	if (RoomData.RoomType == EFCRoomType::Start)
	{
		SetDiscovered(true);
		SetVisited(true);
		SetRoomState(EFCRoomState::Cleared);
	}

	OnSetupDoorsAndWalls(RoomData.DoorMask);
}

void AFCRoomBase::SetRoomState(EFCRoomState NewState)
{
	if (!HasAuthority() || RoomState == NewState)
	{
		return;
	}

	RoomState = NewState;

	if (RoomState == EFCRoomState::Active)
	{
		HandleCombatTriggered();
	}
	else if (RoomState == EFCRoomState::Cleared)
	{
		LockAllDoors(false);
	}

	OnRoomStateChanged.Broadcast(this, RoomState);
}

void AFCRoomBase::SetDiscovered(bool bNewDiscovered)
{
	if (!HasAuthority() || bIsDiscovered == bNewDiscovered)
	{
		return;
	}

	bIsDiscovered = bNewDiscovered;
	OnRoomDiscoveryChanged.Broadcast(this, bIsDiscovered);
}

void AFCRoomBase::SetVisited(bool bNewVisited)
{
	if (!HasAuthority() || bIsVisited == bNewVisited)
	{
		return;
	}

	bIsVisited = bNewVisited;
	if (bIsVisited)
	{
		SetDiscovered(true);
	}
}

void AFCRoomBase::LockAllDoors(bool bLock)
{
	if (!HasAuthority())
	{
		return;
	}

	for (TObjectPtr<AFCDoorBase>& Door : ConnectedDoors)
	{
		if (Door)
		{
			Door->SetDoorLocked(bLock);
		}
	}
}

FTransform AFCRoomBase::GetDoorSocketTransform(uint8 DirectionBit) const
{
	switch (DirectionBit)
	{
	case EFCDungeonDoor::North:
		return NorthDoorSocket ? NorthDoorSocket->GetComponentTransform() : GetActorTransform();
	case EFCDungeonDoor::South:
		return SouthDoorSocket ? SouthDoorSocket->GetComponentTransform() : GetActorTransform();
	case EFCDungeonDoor::East:
		return EastDoorSocket ? EastDoorSocket->GetComponentTransform() : GetActorTransform();
	case EFCDungeonDoor::West:
		return WestDoorSocket ? WestDoorSocket->GetComponentTransform() : GetActorTransform();
	default:
		return GetActorTransform();
	}
}

FTransform AFCRoomBase::GetPlayerSpawnTransform() const
{
	if (PlayerSpawnPoint)
	{
		if (PlayerSpawnPoint->IsRegistered())
		{
			return PlayerSpawnPoint->GetComponentTransform();
		}
		return PlayerSpawnPoint->GetRelativeTransform() * GetActorTransform();
	}
	return GetActorTransform();
}

void AFCRoomBase::OnSetupDoorsAndWalls_Implementation(uint8 DoorMask)
{
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Clean up previous doors
	for (TObjectPtr<AFCDoorBase>& Door : ConnectedDoors)
	{
		if (Door)
		{
			Door->Destroy();
		}
	}
	ConnectedDoors.Empty();

	// Clean up previous walls
	for (TObjectPtr<AFCWallBase>& Wall : SpawnedWalls)
	{
		if (Wall)
		{
			Wall->Destroy();
		}
	}
	SpawnedWalls.Empty();

	TSubclassOf<AFCDoorBase> DoorClassToSpawn = DefaultDoorClass ? DefaultDoorClass : TSubclassOf<AFCDoorBase>(AFCDoorBase::StaticClass());
	TSubclassOf<AFCWallBase> WallClassToSpawn = DefaultWallClass ? DefaultWallClass : TSubclassOf<AFCWallBase>(AFCWallBase::StaticClass());

	auto SpawnBoundary = [&](uint8 DirectionBit)
	{
		const FTransform SocketTransform = GetDoorSocketTransform(DirectionBit);
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (DoorMask & DirectionBit)
		{
			AFCDoorBase* NewDoor = World->SpawnActor<AFCDoorBase>(DoorClassToSpawn, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator(), SpawnParams);
			if (NewDoor)
			{
				ConnectedDoors.Add(NewDoor);
			}
		}
		else if (RoomData.SecretDoorMask & DirectionBit)
		{
			// Secret passage - boundary is handled by AFCDestructibleWall
		}
		else
		{
			// Non-door direction: spawn standard solid wall
			AFCWallBase* NewWall = World->SpawnActor<AFCWallBase>(WallClassToSpawn, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator(), SpawnParams);
			if (NewWall)
			{
				SpawnedWalls.Add(NewWall);
			}
		}
	};

	SpawnBoundary(EFCDungeonDoor::North);
	SpawnBoundary(EFCDungeonDoor::South);
	SpawnBoundary(EFCDungeonDoor::East);
	SpawnBoundary(EFCDungeonDoor::West);
}

void AFCRoomBase::OnRoomBoundsBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	APawn* OverlappingPawn = Cast<APawn>(OtherActor);
	if (!OverlappingPawn || !OverlappingPawn->IsPlayerControlled())
	{
		return;
	}

	SetVisited(true);

	if (RoomState == EFCRoomState::Unvisited)
	{
		SetRoomState(EFCRoomState::Active);
	}
}

void AFCRoomBase::HandleCombatTriggered()
{
	// Non-combat rooms clear immediately
	if (RoomData.RoomType == EFCRoomType::Start ||
		RoomData.RoomType == EFCRoomType::Shop ||
		RoomData.RoomType == EFCRoomType::Treasure ||
		RoomData.RoomType == EFCRoomType::Secret)
	{
		SetRoomState(EFCRoomState::Cleared);
		return;
	}

	// Normal and Boss rooms lock doors and begin monster wave
	LockAllDoors(true);
	SpawnMonsterWave();
}

void AFCRoomBase::SpawnMonsterWave()
{
	UWorld* World = GetWorld();
	if (!World || MonsterClasses.Num() == 0)
	{
		// No monsters configured: auto-clear room
		SetRoomState(EFCRoomState::Cleared);
		return;
	}

	ActiveMonsters.Empty();

	const FVector RoomCenter = GetActorLocation();

	for (int32 i = 0; i < MonsterWaveCount; ++i)
	{
		const int32 ClassIdx = FMath::RandRange(0, MonsterClasses.Num() - 1);
		TSubclassOf<APawn> MonsterClass = MonsterClasses[ClassIdx];
		if (!MonsterClass)
		{
			continue;
		}

		// Random offset within 600 radius
		const float RandX = FMath::RandRange(-600.0f, 600.0f);
		const float RandY = FMath::RandRange(-600.0f, 600.0f);
		const FVector SpawnLoc = RoomCenter + FVector(RandX, RandY, 50.0f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APawn* Monster = World->SpawnActor<APawn>(MonsterClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
		if (Monster)
		{
			Monster->OnDestroyed.AddDynamic(this, &AFCRoomBase::OnMonsterDestroyed);
			ActiveMonsters.Add(Monster);
		}
	}

	if (ActiveMonsters.Num() == 0)
	{
		SetRoomState(EFCRoomState::Cleared);
	}
}

void AFCRoomBase::OnMonsterDestroyed(AActor* DestroyedActor)
{
	if (!HasAuthority())
	{
		return;
	}

	CheckWaveCleared();
}

void AFCRoomBase::CheckWaveCleared()
{
	ActiveMonsters.RemoveAll([](const TWeakObjectPtr<APawn>& Monster)
	{
		return !Monster.IsValid();
	});

	if (ActiveMonsters.Num() == 0)
	{
		SetRoomState(EFCRoomState::Cleared);
	}
}

void AFCRoomBase::OnSecretWallDestroyed(AFCDestructibleWall* Wall)
{
	// Discovered by explosion
	SetDiscovered(true);
}

void AFCRoomBase::OnRep_RoomData()
{
	OnSetupDoorsAndWalls(RoomData.DoorMask);
}

void AFCRoomBase::OnRep_RoomState(EFCRoomState OldState)
{
	OnRoomStateChanged.Broadcast(this, RoomState);
}

void AFCRoomBase::OnRep_IsDiscovered()
{
	OnRoomDiscoveryChanged.Broadcast(this, bIsDiscovered);
}

void AFCRoomBase::OnRep_IsVisited()
{
	// UI update triggers
}
