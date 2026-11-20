#include "Map/FCRoomBase.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/ChildActorComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Character/FCCharacterBase.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Gameplay/Spawner/FCMobSpawnerBase.h"
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

	DirectionDoorClasses.Init(nullptr, 4);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DoorOffset = 3000.0f;

	// Sockets based on DoorOffset (defaults to 3000 cm half-extent for 6000x6000 cm grid)
	NorthDoorSocket = CreateDefaultSubobject<USceneComponent>(TEXT("NorthDoorSocket"));
	NorthDoorSocket->SetupAttachment(SceneRoot);
	NorthDoorSocket->SetRelativeLocation(FVector(0.0f, DoorOffset, 0.0f));
	NorthDoorSocket->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));

	SouthDoorSocket = CreateDefaultSubobject<USceneComponent>(TEXT("SouthDoorSocket"));
	SouthDoorSocket->SetupAttachment(SceneRoot);
	SouthDoorSocket->SetRelativeLocation(FVector(0.0f, -DoorOffset, 0.0f));
	SouthDoorSocket->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	EastDoorSocket = CreateDefaultSubobject<USceneComponent>(TEXT("EastDoorSocket"));
	EastDoorSocket->SetupAttachment(SceneRoot);
	EastDoorSocket->SetRelativeLocation(FVector(DoorOffset, 0.0f, 0.0f));
	EastDoorSocket->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

	WestDoorSocket = CreateDefaultSubobject<USceneComponent>(TEXT("WestDoorSocket"));
	WestDoorSocket->SetupAttachment(SceneRoot);
	WestDoorSocket->SetRelativeLocation(FVector(-DoorOffset, 0.0f, 0.0f));
	WestDoorSocket->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	PlayerSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("PlayerSpawnPoint"));
	PlayerSpawnPoint->SetupAttachment(SceneRoot);
	PlayerSpawnPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));

	RoomBoundsTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("RoomBoundsTrigger"));
	RoomBoundsTrigger->SetupAttachment(SceneRoot);
	RoomBoundsTrigger->SetBoxExtent(FVector(3000.0f, 3000.0f, 300.0f));
	RoomBoundsTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f));
	RoomBoundsTrigger->SetCollisionProfileName(TEXT("Trigger"));
}

void AFCRoomBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (bSyncDoorOffsetToTrigger && !bTriggerInsetApplied && RoomBoundsTrigger)
	{
		DoorOffset = RoomBoundsTrigger->GetUnscaledBoxExtent().X;
	}
	UpdateDoorSockets();
}

void AFCRoomBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (RoomBoundsTrigger)
		{
			RoomBoundsTrigger->OnComponentBeginOverlap.AddDynamic(this, &AFCRoomBase::OnRoomBoundsBeginOverlap);
		}

		FindAndRegisterChildSpawners();
	}
}

void AFCRoomBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	CleanupSpawnedActors();
}

void AFCRoomBase::Destroyed()
{
	CleanupSpawnedActors();
	Super::Destroyed();
}

void AFCRoomBase::CleanupSpawnedActors()
{
	if (!HasAuthority())
	{
		return;
	}

	for (TObjectPtr<AFCMobSpawnerBase>& Spawner : ChildSpawners)
	{
		if (Spawner)
		{
			Spawner->OnAllMobsDefeated.RemoveAll(this);
			Spawner->OnMobSpawned.RemoveAll(this);
		}
	}
	ChildSpawners.Empty();
	ActiveSpawnersRemaining.Empty();

	if (!bManageDoorsExternally)
	{
		for (TObjectPtr<AFCDoorBase>& Door : ConnectedDoors)
		{
			if (IsValid(Door) && !Door->IsActorBeingDestroyed())
			{
				Door->Destroy();
			}
		}
	}
	else
	{
		for (TObjectPtr<AFCDoorBase>& Door : ConnectedDoors)
		{
			if (IsValid(Door) && !Door->IsActorBeingDestroyed())
			{
				Door->SetDoorLockedBySource(this, false);
			}
		}
	}
	ConnectedDoors.Empty();

	for (TObjectPtr<AFCDestructibleWall>& SecretWall : SecretWalls)
	{
		if (IsValid(SecretWall) && !SecretWall->IsActorBeingDestroyed())
		{
			SecretWall->Destroy();
		}
	}
	SecretWalls.Empty();

	for (TObjectPtr<AFCWallBase>& Wall : SpawnedWalls)
	{
		if (IsValid(Wall) && !Wall->IsActorBeingDestroyed())
		{
			Wall->Destroy();
		}
	}
	SpawnedWalls.Empty();
}

void AFCRoomBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFCRoomBase, RoomData, COND_None);
	DOREPLIFETIME_CONDITION(AFCRoomBase, RoomState, COND_None);
	DOREPLIFETIME_CONDITION(AFCRoomBase, bIsDiscovered, COND_None);
	DOREPLIFETIME_CONDITION(AFCRoomBase, bIsVisited, COND_None);
	DOREPLIFETIME_CONDITION(AFCRoomBase, DoorOffset, COND_None);
	DOREPLIFETIME_CONDITION(AFCRoomBase, bManageDoorsExternally, COND_None);
}

void AFCRoomBase::InitializeRoom(const FFCRoomData& InRoomData)
{
	if (!HasAuthority())
	{
		return;
	}

	ServerRoomData = InRoomData;
	if (bIsDiscovered)
	{
		RoomData = ServerRoomData;
	}

	// Start Room is discovered and cleared by default
	if (ServerRoomData.RoomType == EFCRoomType::Start)
	{
		SetDiscovered(true);
		SetVisited(true);
		SetRoomState(EFCRoomState::Cleared);
	}

	if (bSyncDoorOffsetToTrigger && !bManageDoorsExternally && !bTriggerInsetApplied && RoomBoundsTrigger)
	{
		DoorOffset = RoomBoundsTrigger->GetUnscaledBoxExtent().X;
	}

	ApplyTriggerBoundaryInset();
	UpdateDoorSockets();

	OnSetupDoorsAndWalls(ServerRoomData.DoorMask);
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
		for (TObjectPtr<AFCMobSpawnerBase>& Spawner : ChildSpawners)
		{
			if (Spawner)
			{
				Spawner->StopSpawning();
			}
		}
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
	RoomData = bIsDiscovered ? ServerRoomData : FFCRoomData();
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
		if (IsValid(Door) && !Door->IsActorBeingDestroyed())
		{
			Door->SetDoorLockedBySource(this, bLock);
		}
	}
}

void AFCRoomBase::RegisterChildSpawner(AFCMobSpawnerBase* Spawner)
{
	if (!HasAuthority() || !Spawner || ChildSpawners.Contains(Spawner))
	{
		return;
	}

	ChildSpawners.Add(Spawner);

	// Stop spawner if room is not active yet to prevent premature spawning before player enters
	if (RoomState != EFCRoomState::Active)
	{
		Spawner->StopSpawning();
	}

	Spawner->OnAllMobsDefeated.AddDynamic(this, &AFCRoomBase::OnChildSpawnerAllMobsDefeated);
	Spawner->OnMobSpawned.AddDynamic(this, &AFCRoomBase::OnChildSpawnerMobSpawned);
}

void AFCRoomBase::FindAndRegisterChildSpawners()
{
	if (!HasAuthority())
	{
		return;
	}

	TArray<UChildActorComponent*> ChildActorComponents;
	GetComponents<UChildActorComponent>(ChildActorComponents);

	for (UChildActorComponent* Comp : ChildActorComponents)
	{
		if (!Comp)
		{
			continue;
		}

		if (AFCMobSpawnerBase* Spawner = Cast<AFCMobSpawnerBase>(Comp->GetChildActor()))
		{
			RegisterChildSpawner(Spawner);
		}
	}

	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);
	for (AActor* AttachedActor : AttachedActors)
	{
		if (AFCMobSpawnerBase* Spawner = Cast<AFCMobSpawnerBase>(AttachedActor))
		{
			RegisterChildSpawner(Spawner);
		}
	}
}

void AFCRoomBase::StartRoomSpawners()
{
	ActiveSpawnersRemaining.Empty();

	for (TObjectPtr<AFCMobSpawnerBase>& Spawner : ChildSpawners)
	{
		if (Spawner)
		{
			Spawner->StartSpawning();
			if (Spawner->GetActiveMobCount() > 0)
			{
				ActiveSpawnersRemaining.Add(Spawner);
			}
		}
	}
}

void AFCRoomBase::OnChildSpawnerMobSpawned(AFCMobCharacter* Mob)
{
	if (!HasAuthority() || !Mob)
	{
		return;
	}

	AFCMobSpawnerBase* Spawner = Mob->GetOwningSpawner();
	if (Spawner && !ActiveSpawnersRemaining.Contains(Spawner))
	{
		ActiveSpawnersRemaining.Add(Spawner);
	}
}

void AFCRoomBase::OnChildSpawnerAllMobsDefeated(AFCMobSpawnerBase* Spawner)
{
	if (!HasAuthority())
	{
		return;
	}

	ActiveSpawnersRemaining.Remove(Spawner);
	CheckWaveCleared();
}

void AFCRoomBase::OnMonsterDeath(AActor* DeadActor, AActor* Killer)
{
	if (!HasAuthority())
	{
		return;
	}

	ActiveMonsters.RemoveAll([DeadActor](const TWeakObjectPtr<APawn>& Monster)
	{
		return !Monster.IsValid() || Monster.Get() == DeadActor;
	});

	CheckWaveCleared();
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

	// Clean up previous locally owned doors (never destroy shared doors owned externally)
	if (!bManageDoorsExternally)
	{
		for (TObjectPtr<AFCDoorBase>& Door : ConnectedDoors)
		{
			if (IsValid(Door) && !Door->IsActorBeingDestroyed())
			{
				Door->Destroy();
			}
		}
		ConnectedDoors.Empty();
	}

	// Clean up previous walls
	for (TObjectPtr<AFCWallBase>& Wall : SpawnedWalls)
	{
		if (IsValid(Wall) && !Wall->IsActorBeingDestroyed())
		{
			Wall->Destroy();
		}
	}
	SpawnedWalls.Empty();

	TSubclassOf<AFCDoorBase> DoorClassToSpawn = DefaultDoorClass ? DefaultDoorClass : TSubclassOf<AFCDoorBase>(AFCDoorBase::StaticClass());
	TSubclassOf<AFCWallBase> WallClassToSpawn = DefaultWallClass ? DefaultWallClass : TSubclassOf<AFCWallBase>(AFCWallBase::StaticClass());

	const FTransform ActorWorldTransform = GetActorTransform();
	const float RoomSpan = DoorOffset * 2.0f;
	const float EffectiveWallThickness = FMath::Max(10.0f, WallThickness);
	const float EffectiveWallHeight = FMath::Max(100.0f, WallHeight);
	const float WallCenterDist = DoorOffset + EffectiveWallThickness * 0.5f;
	const float LintelBottomZ = FloorThickness + DoorOpeningHeight;
	const float LintelHeight = FMath::Max(0.0f, EffectiveWallHeight - LintelBottomZ);

	auto SpawnSizedWall = [&](const FVector& LocalPos, const FRotator& LocalRot, const FVector& Dimensions, float BaseZOffset)
	{
		if (Dimensions.X <= KINDA_SMALL_NUMBER || Dimensions.Y <= KINDA_SMALL_NUMBER || Dimensions.Z <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const FTransform WorldTransform = FTransform(LocalRot, LocalPos, FVector::OneVector) * ActorWorldTransform;
		AFCWallBase* NewWall = World->SpawnActorDeferred<AFCWallBase>(
			WallClassToSpawn,
			WorldTransform,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);
		if (NewWall)
		{
			NewWall->SetWallDimensions(Dimensions, BaseZOffset);
			UGameplayStatics::FinishSpawningActor(NewWall, WorldTransform);
			SpawnedWalls.Add(NewWall);
		}
	};

	// Spawn 4 non-overlapping corner pillars strictly filling [+-DoorOffset, +-(DoorOffset + WallThickness)]
	const FVector PillarDimensions(EffectiveWallThickness, EffectiveWallThickness, EffectiveWallHeight);
	SpawnSizedWall(FVector(WallCenterDist,  WallCenterDist, 0.0f), FRotator::ZeroRotator, PillarDimensions, 0.0f);
	SpawnSizedWall(FVector(-WallCenterDist, WallCenterDist, 0.0f), FRotator::ZeroRotator, PillarDimensions, 0.0f);
	SpawnSizedWall(FVector(WallCenterDist, -WallCenterDist, 0.0f), FRotator::ZeroRotator, PillarDimensions, 0.0f);
	SpawnSizedWall(FVector(-WallCenterDist,-WallCenterDist, 0.0f), FRotator::ZeroRotator, PillarDimensions, 0.0f);

	auto SpawnBoundary = [&](uint8 DirectionBit, const FVector& OutwardDir, const FVector& LateralDir, const FRotator& FaceRot)
	{
		const FVector WallCenter = OutwardDir * WallCenterDist;

		if (DoorMask & DirectionBit)
		{
			TSubclassOf<AFCDoorBase> DirDoorClass = GetDirectionDoorClass(DirectionBit);
			if (!DirDoorClass)
			{
				DirDoorClass = DoorClassToSpawn;
			}

			float OpeningWidth = 300.0f;
			if (DirDoorClass)
			{
				if (const AFCDoorBase* DoorCDO = GetDefault<AFCDoorBase>(DirDoorClass))
				{
					OpeningWidth = DoorCDO->GetDoorWidth();
				}
			}
			OpeningWidth = FMath::Clamp(OpeningWidth, 50.0f, FMath::Max(50.0f, RoomSpan - 100.0f));

			// Left and right flanking wall segments cleanly framing the doorway opening without overlap
			const float SideSegmentLength = (RoomSpan - OpeningWidth) * 0.5f;
			if (SideSegmentLength > KINDA_SMALL_NUMBER)
			{
				const float SideCenterOffset = (OpeningWidth + SideSegmentLength) * 0.5f;
				const FVector SideDimensions(EffectiveWallThickness, SideSegmentLength, EffectiveWallHeight);
				SpawnSizedWall(WallCenter - LateralDir * SideCenterOffset, FaceRot, SideDimensions, 0.0f);
				SpawnSizedWall(WallCenter + LateralDir * SideCenterOffset, FaceRot, SideDimensions, 0.0f);
			}

			// Header/lintel wall above the doorway opening
			if (LintelHeight > KINDA_SMALL_NUMBER)
			{
				const FVector LintelDimensions(EffectiveWallThickness, OpeningWidth, LintelHeight);
				SpawnSizedWall(WallCenter, FaceRot, LintelDimensions, LintelBottomZ);
			}

			if (!bManageDoorsExternally)
			{
				const FTransform SocketTransform = GetDoorSocketTransform(DirectionBit);
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				AFCDoorBase* NewDoor = World->SpawnActor<AFCDoorBase>(DirDoorClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator(), SpawnParams);
				if (NewDoor)
				{
					ConnectedDoors.Add(NewDoor);
				}
			}
		}
		else if (GetRoomData().SecretDoorMask & DirectionBit)
		{
			// Secret passage - boundary is handled by AFCDestructibleWall spanning [-DoorOffset, +DoorOffset]
		}
		else
		{
			// Non-door direction: spawn standard solid wall strictly between corner pillars
			const FVector WallDimensions(EffectiveWallThickness, RoomSpan, EffectiveWallHeight);
			SpawnSizedWall(WallCenter, FaceRot, WallDimensions, 0.0f);
		}
	};

	SpawnBoundary(EFCDungeonDoor::North, FVector(0.0f,  1.0f, 0.0f), FVector(-1.0f,  0.0f, 0.0f), FRotator(0.0f,  90.0f, 0.0f));
	SpawnBoundary(EFCDungeonDoor::South, FVector(0.0f, -1.0f, 0.0f), FVector( 1.0f,  0.0f, 0.0f), FRotator(0.0f, -90.0f, 0.0f));
	SpawnBoundary(EFCDungeonDoor::East,  FVector( 1.0f, 0.0f, 0.0f), FVector( 0.0f,  1.0f, 0.0f), FRotator(0.0f,   0.0f, 0.0f));
	SpawnBoundary(EFCDungeonDoor::West,  FVector(-1.0f, 0.0f, 0.0f), FVector( 0.0f, -1.0f, 0.0f), FRotator(0.0f, 180.0f, 0.0f));
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

void AFCRoomBase::HandleCombatTriggered_Implementation()
{
	const FFCRoomData& CurrentRoomData = GetRoomData();

	// Non-combat rooms clear immediately
	if (CurrentRoomData.RoomType == EFCRoomType::Start ||
		CurrentRoomData.RoomType == EFCRoomType::Shop ||
		CurrentRoomData.RoomType == EFCRoomType::Treasure ||
		CurrentRoomData.RoomType == EFCRoomType::Secret)
	{
		SetRoomState(EFCRoomState::Cleared);
		return;
	}

	// Normal and Boss rooms lock doors and begin combat
	LockAllDoors(true);

	// Ensure any newly created or late-initialized child spawners are registered
	FindAndRegisterChildSpawners();

	bool bHasCombatContent = false;

	if (ChildSpawners.Num() > 0)
	{
		StartRoomSpawners();
		if (ActiveSpawnersRemaining.Num() > 0)
		{
			bHasCombatContent = true;
		}
	}

	if (MonsterClasses.Num() > 0)
	{
		SpawnMonsterWave();
		if (ActiveMonsters.Num() > 0)
		{
			bHasCombatContent = true;
		}
	}

	if (!bHasCombatContent && ActiveSpawnersRemaining.Num() == 0 && ActiveMonsters.Num() == 0)
	{
		SetRoomState(EFCRoomState::Cleared);
	}
}

void AFCRoomBase::SpawnMonsterWave_Implementation()
{
	UWorld* World = GetWorld();
	if (!World || MonsterClasses.Num() == 0)
	{
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
			if (AFCCharacterBase* Char = Cast<AFCCharacterBase>(Monster))
			{
				Char->OnDeath.AddDynamic(this, &AFCRoomBase::OnMonsterDeath);
			}
			ActiveMonsters.Add(Monster);
		}
	}
}

void AFCRoomBase::OnMonsterDestroyed(AActor* DestroyedActor)
{
	if (!HasAuthority())
	{
		return;
	}

	ActiveMonsters.RemoveAll([DestroyedActor](const TWeakObjectPtr<APawn>& Monster)
	{
		return !Monster.IsValid() || Monster.Get() == DestroyedActor;
	});

	CheckWaveCleared();
}

void AFCRoomBase::CheckWaveCleared()
{
	ActiveMonsters.RemoveAll([](const TWeakObjectPtr<APawn>& Monster)
	{
		return !Monster.IsValid();
	});

	ActiveSpawnersRemaining.RemoveAll([](const TWeakObjectPtr<AFCMobSpawnerBase>& Spawner)
	{
		return !Spawner.IsValid();
	});

	if (ActiveMonsters.Num() == 0 && ActiveSpawnersRemaining.Num() == 0)
	{
		SetRoomState(EFCRoomState::Cleared);
	}
}

void AFCRoomBase::OnRep_RoomData()
{
	UpdateDoorSockets();
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

void AFCRoomBase::SetDoorOffset(float NewDoorOffset)
{
	if (!HasAuthority() || !FMath::IsFinite(NewDoorOffset))
	{
		return;
	}

	DoorOffset = FMath::Max(100.0f, NewDoorOffset);
	if (bTriggerInsetApplied)
	{
		ApplyTriggerBoundaryInset();
	}
	UpdateDoorSockets();
}

void AFCRoomBase::SetWallThickness(float NewWallThickness)
{
	if (!HasAuthority() || !FMath::IsFinite(NewWallThickness))
	{
		return;
	}

	WallThickness = FMath::Max(10.0f, NewWallThickness);
}

void AFCRoomBase::ApplyTriggerBoundaryInset()
{
	if (!RoomBoundsTrigger)
	{
		return;
	}

	const FVector CurrentExtent = RoomBoundsTrigger->GetUnscaledBoxExtent();
	const float InsetExtent = FMath::Max(100.0f, DoorOffset - FMath::Max(0.0f, TriggerBoundaryInset));
	RoomBoundsTrigger->SetBoxExtent(FVector(InsetExtent, InsetExtent, CurrentExtent.Z));
	bTriggerInsetApplied = true;
}

void AFCRoomBase::UpdateDoorSockets()
{
	if (NorthDoorSocket)
	{
		NorthDoorSocket->SetRelativeLocation(FVector(0.0f, GetDoorOffsetForDirection(EFCDungeonDoor::North), 0.0f));
	}
	if (SouthDoorSocket)
	{
		SouthDoorSocket->SetRelativeLocation(FVector(0.0f, -GetDoorOffsetForDirection(EFCDungeonDoor::South), 0.0f));
	}
	if (EastDoorSocket)
	{
		EastDoorSocket->SetRelativeLocation(FVector(GetDoorOffsetForDirection(EFCDungeonDoor::East), 0.0f, 0.0f));
	}
	if (WestDoorSocket)
	{
		WestDoorSocket->SetRelativeLocation(FVector(-GetDoorOffsetForDirection(EFCDungeonDoor::West), 0.0f, 0.0f));
	}
}

void AFCRoomBase::SetDirectionDoorClass(uint8 DirectionBit, TSubclassOf<AFCDoorBase> InDoorClass)
{
	if (!HasAuthority())
	{
		return;
	}

	const int32 Idx = EFCDungeonDoor::DirectionBitToIndex(DirectionBit);
	if (Idx != INDEX_NONE)
	{
		if (DirectionDoorClasses.Num() < 4)
		{
			DirectionDoorClasses.Init(nullptr, 4);
		}
		DirectionDoorClasses[Idx] = InDoorClass;
		UpdateDoorSockets();
	}
}

TSubclassOf<AFCDoorBase> AFCRoomBase::GetDirectionDoorClass(uint8 DirectionBit) const
{
	const int32 Idx = EFCDungeonDoor::DirectionBitToIndex(DirectionBit);
	if (DirectionDoorClasses.IsValidIndex(Idx) && DirectionDoorClasses[Idx])
	{
		return DirectionDoorClasses[Idx];
	}
	return DefaultDoorClass;
}

float AFCRoomBase::GetDoorOffsetForDirection(uint8 DirectionBit) const
{
	// When doors are managed externally (e.g. by AFCDungeonManager), CellSize (6300cm)
	// already allocates the DoorWidth (300cm) gap between room boundaries.
	// Therefore, sockets remain flush with the room boundary (DoorOffset = 3000cm).
	if (bManageDoorsExternally)
	{
		return DoorOffset;
	}

	// For standalone rooms spawning their own doors internally:
	if (GetRoomData().DoorMask & DirectionBit)
	{
		float CustomDoorWidth = 300.0f;
		if (TSubclassOf<AFCDoorBase> DoorClass = GetDirectionDoorClass(DirectionBit))
		{
			if (const AFCDoorBase* DoorCDO = GetDefault<AFCDoorBase>(DoorClass))
			{
				CustomDoorWidth = DoorCDO->GetDoorWidth();
			}
		}
		// Adjust door socket so the gap between adjacent room door sockets equals CustomDoorWidth
		return FMath::Max(100.0f, DoorOffset - CustomDoorWidth * 0.5f);
	}

	// Where a wall is located (non-door direction), remain strictly at DoorOffset (flush with room boundary)
	return DoorOffset;
}

void AFCRoomBase::RegisterConnectedDoor(AFCDoorBase* Door)
{
	if (!HasAuthority())
	{
		return;
	}

	if (Door && !ConnectedDoors.Contains(Door))
	{
		ConnectedDoors.Add(Door);
	}
}

void AFCRoomBase::RegisterSecretWall(AFCDestructibleWall* Wall)
{
	if (!HasAuthority())
	{
		return;
	}

	if (Wall && !SecretWalls.Contains(Wall))
	{
		SecretWalls.Add(Wall);
	}
}

void AFCRoomBase::OnRep_DoorOffset()
{
	UpdateDoorSockets();
}
