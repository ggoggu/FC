#include "Map/FCDungeonManager.h"
#include "Containers/Queue.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Pawn.h"
#include "Game/FCGameMode.h"
#include "Map/FCDestructibleWall.h"
#include "Map/FCRoomBase.h"
#include "Math/RandomStream.h"
#include "Net/UnrealNetwork.h"

AFCDungeonManager::AFCDungeonManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	DefaultRoomClass = AFCRoomBase::StaticClass();
	SecretWallClass = AFCDestructibleWall::StaticClass();
}

void AFCDungeonManager::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (AFCGameMode* GM = World->GetAuthGameMode<AFCGameMode>())
			{
				GM->SetDungeonManager(this);
			}
		}
	}
}

void AFCDungeonManager::BeginPlay()
{
	Super::BeginPlay();

	// Dungeon generation must execute only on the authoritative server
	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (AFCGameMode* GM = World->GetAuthGameMode<AFCGameMode>())
			{
				GM->SetDungeonManager(this);
			}
		}

		if (SpawnedRooms.Num() == 0)
		{
			GenerateDungeon();
		}
		else
		{
			TeleportPlayersToStartRoom();
		}
	}
}

void AFCDungeonManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AFCDungeonManager::GenerateDungeon()
{
	if (!HasAuthority())
	{
		return;
	}

	ClearDungeon();

	// Step 1: 2D Logical Grid & Skeleton
	BuildSkeleton();

	// Step 2: Role Assignment & Secret Room Filter
	AssignRoomRoles();
	CalculateDoorMasks();

	// Step 4: 3D World Instantiation
	SpawnDungeonRooms();

	// Step 5: Secret Wall connections
	SetupSecretPassages();

	// Step 6: Setup PlayerStart in Start Room
	SetupStartRoomPlayerStart();

	// Step 7: Initial Discovery around Start Room
	RevealAdjacentRooms(StartRoomCoord);

	// Step 8: Teleport existing players to Start Room
	TeleportPlayersToStartRoom();

	OnDungeonGenerated.Broadcast();
}

void AFCDungeonManager::ClearDungeon()
{
	if (!HasAuthority())
	{
		return;
	}

	if (bSpawnedOwnPlayerStart && StartRoomPlayerStart.IsValid())
	{
		StartRoomPlayerStart->Destroy();
	}
	StartRoomPlayerStart = nullptr;
	bSpawnedOwnPlayerStart = false;

	for (TObjectPtr<AFCRoomBase>& Room : SpawnedRooms)
	{
		if (Room)
		{
			Room->Destroy();
		}
	}

	SpawnedRooms.Empty();
	RoomGrid.Empty();
}

void AFCDungeonManager::SetupStartRoomPlayerStart()
{
	AFCRoomBase* StartRoom = GetStartRoom();
	if (!StartRoom)
	{
		return;
	}

	const FTransform SpawnTransform = StartRoom->GetPlayerSpawnTransform();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// If we already hold a reference to an active start spot, reposition it
	if (StartRoomPlayerStart.IsValid())
	{
		if (USceneComponent* RootComp = StartRoomPlayerStart->GetRootComponent())
		{
			RootComp->SetMobility(EComponentMobility::Movable);
		}
		StartRoomPlayerStart->SetActorTransform(SpawnTransform);
		return;
	}

	// Check if there is an existing APlayerStart placed in the level
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		APlayerStart* LevelStart = *It;
		if (IsValid(LevelStart))
		{
			StartRoomPlayerStart = LevelStart;
			bSpawnedOwnPlayerStart = false;
			if (USceneComponent* RootComp = LevelStart->GetRootComponent())
			{
				RootComp->SetMobility(EComponentMobility::Movable);
			}
			LevelStart->SetActorTransform(SpawnTransform);
			return;
		}
	}

	// Otherwise spawn a dedicated APlayerStart actor at the Start Room's spawn transform
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APlayerStart* NewStart = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), SpawnTransform.GetLocation(), SpawnTransform.GetRotation().Rotator(), SpawnParams);
	if (NewStart)
	{
		StartRoomPlayerStart = NewStart;
		bSpawnedOwnPlayerStart = true;
	}
}

void AFCDungeonManager::TeleportPlayersToStartRoom()
{
	if (!HasAuthority())
	{
		return;
	}

	AFCRoomBase* StartRoom = GetStartRoom();
	if (!StartRoom)
	{
		return;
	}

	const FTransform BaseTransform = StartRoom->GetPlayerSpawnTransform();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	static const FVector PlayerOffsets[4] = {
		FVector(0.0f, 0.0f, 0.0f),
		FVector(150.0f, 0.0f, 0.0f),
		FVector(-150.0f, 0.0f, 0.0f),
		FVector(0.0f, 150.0f, 0.0f)
	};

	int32 PlayerIndex = 0;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (APawn* PlayerPawn = PC->GetPawn())
			{
				const FVector Offset = PlayerOffsets[PlayerIndex % 4];
				const FVector TargetLocation = BaseTransform.GetLocation() + Offset;

				PlayerPawn->TeleportTo(TargetLocation, BaseTransform.GetRotation().Rotator(), false, true);
				PC->SetControlRotation(BaseTransform.GetRotation().Rotator());
				PlayerIndex++;
			}
		}
	}
}

AFCRoomBase* AFCDungeonManager::GetRoomAt(const FIntPoint& Coord) const
{
	for (const TObjectPtr<AFCRoomBase>& Room : SpawnedRooms)
	{
		if (Room && Room->GetRoomData().Coordinates == Coord)
		{
			return Room.Get();
		}
	}
	return nullptr;
}

void AFCDungeonManager::RevealAdjacentRooms(const FIntPoint& CenterCoord)
{
	if (!HasAuthority())
	{
		return;
	}

	const TArray<FIntPoint> Adjacents = GetAdjacentCoordinates(CenterCoord);
	for (const FIntPoint& Adj : Adjacents)
	{
		AFCRoomBase* NeighborRoom = GetRoomAt(Adj);
		if (NeighborRoom && NeighborRoom->GetRoomData().RoomType != EFCRoomType::Secret)
		{
			NeighborRoom->SetDiscovered(true);
		}
	}
}

void AFCDungeonManager::HandleRoomStateChanged(AFCRoomBase* Room, EFCRoomState NewState)
{
	if (!HasAuthority() || !Room)
	{
		return;
	}

	if (NewState == EFCRoomState::Active || NewState == EFCRoomState::Cleared)
	{
		RevealAdjacentRooms(Room->GetRoomData().Coordinates);
	}
}

void AFCDungeonManager::HandleSecretWallDestroyed(AFCDestructibleWall* Wall)
{
	if (!HasAuthority() || !Wall)
	{
		return;
	}

	AFCRoomBase* SecretRoom = GetRoomAt(Wall->GetLinkedSecretRoomCoord());
	if (SecretRoom)
	{
		SecretRoom->SetDiscovered(true);
	}
}

void AFCDungeonManager::BuildSkeleton()
{
	StartRoomCoord = FIntPoint::ZeroValue;
	FIntPoint CurrentPos(0, 0);

	FFCRoomData StartRoom;
	StartRoom.Coordinates = CurrentPos;
	StartRoom.RoomType = EFCRoomType::Start;
	StartRoom.DistanceFromStart = 0;

	RoomGrid.Add(CurrentPos, StartRoom);

	TArray<FIntPoint> RoomQueue;
	RoomQueue.Add(CurrentPos);

	int32 CurrentRoomCount = 1;

	// Pick a random asymmetry bias per generation for non-compact patterns
	// 0: Vertical tendency, 1: Horizontal tendency, 2: Diagonal/L-shape tendency
	const int32 AsymmetryAxis = FMath::RandRange(0, 2);
	int32 SafetyAttempts = 2000;

	while (CurrentRoomCount < TargetRoomCount && RoomQueue.Num() > 0 && --SafetyAttempts > 0)
	{
		int32 SelectedIndex = 0;

		// 1. Choose room to expand from based on StartLocationMode and LayoutPattern
		const int32 OriginQueueIndex = RoomQueue.IndexOfByKey(FIntPoint::ZeroValue);
		if (StartLocationMode == EFCDungeonStartLocation::Center && OriginQueueIndex != INDEX_NONE && CountAdjacentRooms(FIntPoint::ZeroValue) < 3 && FMath::FRand() < 0.6f)
		{
			// In Center mode, force branches to sprout outward from (0,0) early on to ensure it is a central hub
			SelectedIndex = OriginQueueIndex;
		}
		else if (LayoutPattern == EFCDungeonLayoutPattern::Compact)
		{
			// Uniform selection produces an isotropic, roughly square/circular cluster
			SelectedIndex = FMath::RandRange(0, RoomQueue.Num() - 1);
		}
		else if (LayoutPattern == EFCDungeonLayoutPattern::Asymmetric)
		{
			// 70% chance to expand from recently added rooms (creates outward branches instead of filling center)
			if (FMath::FRand() < 0.7f && RoomQueue.Num() > 1)
			{
				const int32 RecentWindow = FMath::Min(3, RoomQueue.Num());
				SelectedIndex = RoomQueue.Num() - 1 - FMath::RandRange(0, RecentWindow - 1);
			}
			else
			{
				SelectedIndex = FMath::RandRange(0, RoomQueue.Num() - 1);
			}
		}
		else // Sprawling (Long winding arms)
		{
			// 85% chance to continue from the newest room (DFS snake)
			if (FMath::FRand() < 0.85f && RoomQueue.Num() > 1)
			{
				SelectedIndex = RoomQueue.Num() - 1;
			}
			else
			{
				SelectedIndex = FMath::RandRange(0, RoomQueue.Num() - 1);
			}
		}

		const FIntPoint ExpandPos = RoomQueue[SelectedIndex];
		const TArray<FIntPoint> Adjacents = GetAdjacentCoordinates(ExpandPos);

		TArray<FIntPoint> ValidEmptyAdjacents;
		for (const FIntPoint& Adj : Adjacents)
		{
			if (RoomGrid.Contains(Adj))
			{
				continue;
			}

			// Anti-Clustering: if candidate touches 2 or more existing rooms, it forms a square block loop
			if (LayoutPattern != EFCDungeonLayoutPattern::Compact && bPreventClustering)
			{
				const int32 TouchingRooms = CountAdjacentRooms(Adj);
				if (TouchingRooms > 1 && FMath::FRand() < 0.8f)
				{
					continue; // Penalize/skip to prevent square clumping
				}
			}

			ValidEmptyAdjacents.Add(Adj);
		}

		// Fallback: If anti-clustering filtered all neighbors, relax filter to keep generation moving
		if (ValidEmptyAdjacents.Num() == 0)
		{
			for (const FIntPoint& Adj : Adjacents)
			{
				if (!RoomGrid.Contains(Adj))
				{
					ValidEmptyAdjacents.Add(Adj);
				}
			}
		}

		if (ValidEmptyAdjacents.Num() > 0)
		{
			int32 ChosenIndex = 0;

			// Weighted selection for directional asymmetry
			if (LayoutPattern != EFCDungeonLayoutPattern::Compact && ValidEmptyAdjacents.Num() > 1)
			{
				TArray<float> Weights;
				for (const FIntPoint& Candidate : ValidEmptyAdjacents)
				{
					float Weight = 1.0f;
					const int32 DeltaX = FMath::Abs(Candidate.X - ExpandPos.X);
					const int32 DeltaY = FMath::Abs(Candidate.Y - ExpandPos.Y);

					if (AsymmetryAxis == 0)
					{
						// Favor vertical elongation (North / South)
						if (DeltaY > 0) Weight += 2.0f;
					}
					else if (AsymmetryAxis == 1)
					{
						// Favor horizontal elongation (East / West)
						if (DeltaX > 0) Weight += 2.0f;
					}
					else
					{
						// Favor outward growth away from Start room
						const int32 DistFromOrigin = FMath::Abs(Candidate.X) + FMath::Abs(Candidate.Y);
						Weight += (DistFromOrigin * 0.5f);
					}

					// Strongly discourage square corners
					const int32 Touching = CountAdjacentRooms(Candidate);
					if (Touching > 1)
					{
						Weight *= 0.2f;
					}

					Weights.Add(Weight);
				}

				float TotalWeight = 0.0f;
				for (float W : Weights) TotalWeight += W;
				float RandWeight = FMath::FRandRange(0.0f, TotalWeight);

				for (int32 i = 0; i < Weights.Num(); ++i)
				{
					RandWeight -= Weights[i];
					if (RandWeight <= 0.0f)
					{
						ChosenIndex = i;
						break;
					}
				}
			}
			else
			{
				ChosenIndex = FMath::RandRange(0, ValidEmptyAdjacents.Num() - 1);
			}

			const FIntPoint NewPos = ValidEmptyAdjacents[ChosenIndex];

			FFCRoomData NewRoom;
			NewRoom.Coordinates = NewPos;
			NewRoom.RoomType = EFCRoomType::Normal;
			NewRoom.DistanceFromStart = RoomGrid[ExpandPos].DistanceFromStart + 1;

			RoomGrid.Add(NewPos, NewRoom);
			RoomQueue.Add(NewPos);
			CurrentRoomCount++;
		}
		else
		{
			RoomQueue.RemoveAt(SelectedIndex);
		}
	}
}

void AFCDungeonManager::AssignRoomRoles()
{
	// 1. Determine Start Room based on StartLocationMode
	int32 MinX = 0, MaxX = 0, MinY = 0, MaxY = 0;
	for (const auto& Pair : RoomGrid)
	{
		MinX = FMath::Min(MinX, Pair.Key.X);
		MaxX = FMath::Max(MaxX, Pair.Key.X);
		MinY = FMath::Min(MinY, Pair.Key.Y);
		MaxY = FMath::Max(MaxY, Pair.Key.Y);
	}
	const FVector2D BoundingCenter(static_cast<float>(MinX + MaxX) * 0.5f, static_cast<float>(MinY + MaxY) * 0.5f);

	if (StartLocationMode == EFCDungeonStartLocation::Edge)
	{
		// Find dead-end / leaf rooms on the outer perimeter
		TArray<FIntPoint> CandidateStarts;
		for (const auto& Pair : RoomGrid)
		{
			if (CountAdjacentRooms(Pair.Key) == 1)
			{
				CandidateStarts.Add(Pair.Key);
			}
		}

		// Fallback: If no dead-end leaf rooms exist, choose rooms with minimal degree
		if (CandidateStarts.Num() == 0)
		{
			int32 MinNeighbors = 4;
			for (const auto& Pair : RoomGrid)
			{
				MinNeighbors = FMath::Min(MinNeighbors, CountAdjacentRooms(Pair.Key));
			}
			for (const auto& Pair : RoomGrid)
			{
				if (CountAdjacentRooms(Pair.Key) == MinNeighbors)
				{
					CandidateStarts.Add(Pair.Key);
				}
			}
		}

		// Pick the candidate furthest from the geometric bounding center (outer perimeter leaf)
		FIntPoint BestEdgeStart = FIntPoint::ZeroValue;
		float MaxDistSqFromCenter = -1.0f;

		for (const FIntPoint& Candidate : CandidateStarts)
		{
			const float DistSq = FVector2D::DistSquared(FVector2D(Candidate.X, Candidate.Y), BoundingCenter);
			if (DistSq > MaxDistSqFromCenter)
			{
				MaxDistSqFromCenter = DistSq;
				BestEdgeStart = Candidate;
			}
		}

		// Reassign old start (0, 0) to Normal room
		if (RoomGrid.Contains(FIntPoint::ZeroValue))
		{
			RoomGrid[FIntPoint::ZeroValue].RoomType = EFCRoomType::Normal;
		}

		// Set selected edge leaf as Start Room
		StartRoomCoord = BestEdgeStart;
		RoomGrid[StartRoomCoord].RoomType = EFCRoomType::Start;
	}
	else
	{
		// Center mode: Keep (0, 0) as Start Room
		StartRoomCoord = FIntPoint::ZeroValue;
		if (RoomGrid.Contains(StartRoomCoord))
		{
			RoomGrid[StartRoomCoord].RoomType = EFCRoomType::Start;
		}
	}

	// Recalculate true BFS path distances from the chosen Start Room
	RecalculateDistancesFrom(StartRoomCoord);

	// 2. Collect leaf rooms (dead ends) excluding Start Room
	TArray<FIntPoint> LeafRooms;
	for (const auto& Pair : RoomGrid)
	{
		if (Pair.Key == StartRoomCoord || Pair.Value.RoomType == EFCRoomType::Start)
		{
			continue;
		}

		if (CountAdjacentRooms(Pair.Key) == 1)
		{
			LeafRooms.Add(Pair.Key);
		}
	}

	// 3. Assign Boss Room to the furthest leaf room from Start Room
	FIntPoint BossPos = FIntPoint::ZeroValue;
	int32 MaxDist = -1;

	for (const FIntPoint& Leaf : LeafRooms)
	{
		const int32 Dist = RoomGrid[Leaf].DistanceFromStart;
		if (Dist > MaxDist)
		{
			MaxDist = Dist;
			BossPos = Leaf;
		}
	}

	// Fallback if no leaf room found
	if (!RoomGrid.Contains(BossPos) || BossPos == StartRoomCoord)
	{
		for (const auto& Pair : RoomGrid)
		{
			if (Pair.Key != StartRoomCoord && Pair.Value.DistanceFromStart > MaxDist)
			{
				MaxDist = Pair.Value.DistanceFromStart;
				BossPos = Pair.Key;
			}
		}
	}

	if (RoomGrid.Contains(BossPos))
	{
		RoomGrid[BossPos].RoomType = EFCRoomType::Boss;
		LeafRooms.Remove(BossPos);
	}

	// 4. Assign Treasure and Shop
	auto PickSpecialRoom = [&](EFCRoomType Type)
	{
		if (LeafRooms.Num() > 0)
		{
			const int32 Idx = FMath::RandRange(0, LeafRooms.Num() - 1);
			RoomGrid[LeafRooms[Idx]].RoomType = Type;
			LeafRooms.RemoveAt(Idx);
		}
		else
		{
			TArray<FIntPoint> NormalCandidates;
			for (const auto& Pair : RoomGrid)
			{
				if (Pair.Value.RoomType == EFCRoomType::Normal && Pair.Key != StartRoomCoord && Pair.Value.DistanceFromStart >= 2)
				{
					NormalCandidates.Add(Pair.Key);
				}
			}
			if (NormalCandidates.Num() > 0)
			{
				const int32 Idx = FMath::RandRange(0, NormalCandidates.Num() - 1);
				RoomGrid[NormalCandidates[Idx]].RoomType = Type;
			}
		}
	};

	PickSpecialRoom(EFCRoomType::Treasure);
	PickSpecialRoom(EFCRoomType::Shop);

	// 5. Assign Secret Room (Isaac rule: adjacent to >=2 rooms, not adjacent to Boss)
	TArray<FIntPoint> SecretCandidates;

	const int32 SecretMinX = MinX - 1;
	const int32 SecretMaxX = MaxX + 1;
	const int32 SecretMinY = MinY - 1;
	const int32 SecretMaxY = MaxY + 1;

	for (int32 X = SecretMinX; X <= SecretMaxX; ++X)
	{
		for (int32 Y = SecretMinY; Y <= SecretMaxY; ++Y)
		{
			const FIntPoint CheckPos(X, Y);
			if (RoomGrid.Contains(CheckPos))
			{
				continue;
			}

			const int32 AdjCount = CountAdjacentRooms(CheckPos);
			if (AdjCount >= 2)
			{
				if (!HasAdjacentRoomOfType(CheckPos, EFCRoomType::Boss))
				{
					SecretCandidates.Add(CheckPos);
				}
			}
		}
	}

	for (int32 i = 0; i < MaxSecretRooms && SecretCandidates.Num() > 0; ++i)
	{
		const int32 SecretIdx = FMath::RandRange(0, SecretCandidates.Num() - 1);
		const FIntPoint SecretPos = SecretCandidates[SecretIdx];

		FFCRoomData SecretRoom;
		SecretRoom.Coordinates = SecretPos;
		SecretRoom.RoomType = EFCRoomType::Secret;
		SecretRoom.DistanceFromStart = 0;

		RoomGrid.Add(SecretPos, SecretRoom);
		SecretCandidates.RemoveAt(SecretIdx);
	}
}

void AFCDungeonManager::CalculateDoorMasks()
{
	for (auto& Pair : RoomGrid)
	{
		const FIntPoint Pos = Pair.Key;
		const bool bIsCurrentSecret = (Pair.Value.RoomType == EFCRoomType::Secret);
		uint8 NormalMask = 0;
		uint8 SecretMask = 0;

		auto CheckRoom = [&](uint8 DirectionBit, const FIntPoint& NeighborCoord)
		{
			if (const FFCRoomData* Neighbor = RoomGrid.Find(NeighborCoord))
			{
				if (Neighbor->RoomType == EFCRoomType::Secret || bIsCurrentSecret)
				{
					SecretMask |= DirectionBit;
				}
				else
				{
					NormalMask |= DirectionBit;
				}
			}
		};

		CheckRoom(EFCDungeonDoor::North, FIntPoint(Pos.X, Pos.Y + 1));
		CheckRoom(EFCDungeonDoor::South, FIntPoint(Pos.X, Pos.Y - 1));
		CheckRoom(EFCDungeonDoor::East,  FIntPoint(Pos.X + 1, Pos.Y));
		CheckRoom(EFCDungeonDoor::West,  FIntPoint(Pos.X - 1, Pos.Y));

		Pair.Value.DoorMask = bIsCurrentSecret ? 0 : NormalMask;
		Pair.Value.SecretDoorMask = SecretMask;
	}
}

void AFCDungeonManager::SpawnDungeonRooms()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector OriginLocation = GetActorLocation();

	for (const auto& Pair : RoomGrid)
	{
		const FIntPoint& Coord = Pair.Key;
		const FFCRoomData& Data = Pair.Value;

		const FVector WorldLocation = OriginLocation + FVector(Coord.X * CellSize, Coord.Y * CellSize, 0.0f);
		const FRotator WorldRotation = FRotator::ZeroRotator;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		TSubclassOf<AFCRoomBase> RoomClass = GetRoomClassForType(Data.RoomType);
		if (!RoomClass)
		{
			RoomClass = DefaultRoomClass ? DefaultRoomClass : TSubclassOf<AFCRoomBase>(AFCRoomBase::StaticClass());
		}

		AFCRoomBase* SpawnedRoom = World->SpawnActor<AFCRoomBase>(RoomClass, WorldLocation, WorldRotation, SpawnParams);
		if (SpawnedRoom)
		{
			SpawnedRoom->InitializeRoom(Data);
			SpawnedRoom->OnRoomStateChanged.AddDynamic(this, &AFCDungeonManager::HandleRoomStateChanged);
			SpawnedRooms.Add(SpawnedRoom);
		}
	}
}

void AFCDungeonManager::SetupSecretPassages()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<AFCDestructibleWall> WallClass = SecretWallClass ? SecretWallClass : TSubclassOf<AFCDestructibleWall>(AFCDestructibleWall::StaticClass());

	for (const auto& Pair : RoomGrid)
	{
		if (Pair.Value.RoomType != EFCRoomType::Secret)
		{
			continue;
		}

		const FIntPoint SecretCoord = Pair.Key;

		// Check adjacent rooms to place bombable walls on their boundary
		auto CheckAndSpawnSecretWall = [&](const FIntPoint& NeighborCoord, uint8 SocketDirection)
		{
			if (const FFCRoomData* NeighborData = RoomGrid.Find(NeighborCoord))
			{
				if (NeighborData->RoomType != EFCRoomType::Secret)
				{
					AFCRoomBase* NeighborRoom = GetRoomAt(NeighborCoord);
					if (NeighborRoom)
					{
						const FTransform SocketTransform = NeighborRoom->GetDoorSocketTransform(SocketDirection);
						FActorSpawnParameters SpawnParams;
						SpawnParams.Owner = NeighborRoom;
						SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

						AFCDestructibleWall* SecretWall = World->SpawnActor<AFCDestructibleWall>(WallClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator(), SpawnParams);
						if (SecretWall)
						{
							SecretWall->SetLinkedSecretRoomCoord(SecretCoord);
							SecretWall->OnWallDestroyed.AddDynamic(this, &AFCDungeonManager::HandleSecretWallDestroyed);
						}
					}
				}
			}
		};

		CheckAndSpawnSecretWall(FIntPoint(SecretCoord.X, SecretCoord.Y - 1), EFCDungeonDoor::North); // Secret is North of Neighbor (Neighbor is South)
		CheckAndSpawnSecretWall(FIntPoint(SecretCoord.X, SecretCoord.Y + 1), EFCDungeonDoor::South); // Secret is South of Neighbor (Neighbor is North)
		CheckAndSpawnSecretWall(FIntPoint(SecretCoord.X - 1, SecretCoord.Y), EFCDungeonDoor::East);  // Secret is East of Neighbor (Neighbor is West)
		CheckAndSpawnSecretWall(FIntPoint(SecretCoord.X + 1, SecretCoord.Y), EFCDungeonDoor::West);  // Secret is West of Neighbor (Neighbor is East)
	}
}

TSubclassOf<AFCRoomBase> AFCDungeonManager::GetRoomClassForType(EFCRoomType Type) const
{
	switch (Type)
	{
	case EFCRoomType::Start:
		return StartRoomClass ? StartRoomClass : DefaultRoomClass;
	case EFCRoomType::Boss:
		return BossRoomClass ? BossRoomClass : DefaultRoomClass;
	case EFCRoomType::Shop:
		return ShopRoomClass ? ShopRoomClass : DefaultRoomClass;
	case EFCRoomType::Treasure:
		return TreasureRoomClass ? TreasureRoomClass : DefaultRoomClass;
	case EFCRoomType::Secret:
		return SecretRoomClass ? SecretRoomClass : DefaultRoomClass;
	case EFCRoomType::Normal:
	default:
		return DefaultRoomClass;
	}
}

TArray<FIntPoint> AFCDungeonManager::GetAdjacentCoordinates(const FIntPoint& Coord) const
{
	TArray<FIntPoint> Adjacents;
	Adjacents.Add(FIntPoint(Coord.X, Coord.Y + 1)); // North
	Adjacents.Add(FIntPoint(Coord.X, Coord.Y - 1)); // South
	Adjacents.Add(FIntPoint(Coord.X + 1, Coord.Y)); // East
	Adjacents.Add(FIntPoint(Coord.X - 1, Coord.Y)); // West
	return Adjacents;
}

int32 AFCDungeonManager::CountAdjacentRooms(const FIntPoint& Coord) const
{
	int32 Count = 0;
	const TArray<FIntPoint> Adjacents = GetAdjacentCoordinates(Coord);
	for (const FIntPoint& Adj : Adjacents)
	{
		if (RoomGrid.Contains(Adj))
		{
			Count++;
		}
	}
	return Count;
}

bool AFCDungeonManager::HasAdjacentRoomOfType(const FIntPoint& Coord, EFCRoomType Type) const
{
	const TArray<FIntPoint> Adjacents = GetAdjacentCoordinates(Coord);
	for (const FIntPoint& Adj : Adjacents)
	{
		if (const FFCRoomData* Room = RoomGrid.Find(Adj))
		{
			if (Room->RoomType == Type)
			{
				return true;
			}
		}
	}
	return false;
}

void AFCDungeonManager::RecalculateDistancesFrom(const FIntPoint& OriginCoord)
{
	for (auto& Pair : RoomGrid)
	{
		Pair.Value.DistanceFromStart = -1;
	}

	if (!RoomGrid.Contains(OriginCoord))
	{
		return;
	}

	RoomGrid[OriginCoord].DistanceFromStart = 0;

	TQueue<FIntPoint> Queue;
	Queue.Enqueue(OriginCoord);

	while (!Queue.IsEmpty())
	{
		FIntPoint Current;
		Queue.Dequeue(Current);

		const int32 CurrentDist = RoomGrid[Current].DistanceFromStart;
		const TArray<FIntPoint> Neighbors = GetAdjacentCoordinates(Current);

		for (const FIntPoint& Neighbor : Neighbors)
		{
			if (FFCRoomData* NeighborData = RoomGrid.Find(Neighbor))
			{
				if (NeighborData->RoomType != EFCRoomType::Secret && NeighborData->DistanceFromStart == -1)
				{
					NeighborData->DistanceFromStart = CurrentDist + 1;
					Queue.Enqueue(Neighbor);
				}
			}
		}
	}
}
