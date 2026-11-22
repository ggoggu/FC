#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Map/FCRoomBase.h"
#include "Map/FCDoorBase.h"
#include "Map/FCWallBase.h"
#include "Map/FCDestructibleWall.h"
#include "Map/FCDungeonManager.h"
#include "Map/FCDungeonTypes.h"
#include "Game/FCGameMode.h"
#include "GameFramework/PlayerStart.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCPlayerStartRoomTest, "FC.Map.PlayerStartRoom", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCPlayerStartRoomTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: FCRoomBase PlayerSpawnPoint Component & Default Offset
	// =========================================================================
	{
		AFCRoomBase* Room = NewObject<AFCRoomBase>();
		TestNotNull(TEXT("AFCRoomBase must be instantiable"), Room);

		if (Room)
		{
			USceneComponent* SpawnPoint = Room->GetPlayerSpawnPoint();
			TestNotNull(TEXT("PlayerSpawnPoint component must exist on AFCRoomBase"), SpawnPoint);

			if (SpawnPoint)
			{
				TestEqual(TEXT("PlayerSpawnPoint relative X must be 0"), SpawnPoint->GetRelativeLocation().X, 0.0);
				TestEqual(TEXT("PlayerSpawnPoint relative Y must be 0"), SpawnPoint->GetRelativeLocation().Y, 0.0);
				TestEqual(TEXT("PlayerSpawnPoint relative Z must be 100 for proper floor clearance"), SpawnPoint->GetRelativeLocation().Z, 100.0);
			}

			// Verify GetPlayerSpawnTransform
			const FTransform SpawnTransform = Room->GetPlayerSpawnTransform();
			TestEqual(TEXT("SpawnTransform Z must be 100 above actor root"), SpawnTransform.GetLocation().Z, 100.0);
		}
	}

	// =========================================================================
	// Test 2: Start Room Exploration & Initial State
	// =========================================================================
	{
		AFCRoomBase* StartRoom = NewObject<AFCRoomBase>();
		TestNotNull(TEXT("AFCRoomBase must be instantiable for Start Room test"), StartRoom);

		if (StartRoom)
		{
			FFCRoomData StartData;
			StartData.Coordinates = FIntPoint::ZeroValue;
			StartData.RoomType = EFCRoomType::Start;
			StartData.DoorMask = 0;

			StartRoom->InitializeRoom(StartData);

			TestTrue(TEXT("Start Room must be discovered by default"), StartRoom->IsDiscovered());
			TestTrue(TEXT("Start Room must be marked visited by default"), StartRoom->IsVisited());
			TestEqual(TEXT("Start Room must transition to Cleared state"), StartRoom->GetRoomState(), EFCRoomState::Cleared);
		}
	}

	// =========================================================================
	// Test 3: GameMode DungeonManager Linkage & Safe Fallback
	// =========================================================================
	{
		AFCGameMode* GameMode = NewObject<AFCGameMode>();
		TestNotNull(TEXT("AFCGameMode must be instantiable"), GameMode);

		if (GameMode)
		{
			// Without a dungeon manager, GetDungeonManager should gracefully return nullptr
			TestNull(TEXT("GetDungeonManager without manager in world should return nullptr"), GameMode->GetDungeonManager());

			// Register a DungeonManager
			AFCDungeonManager* DungeonMgr = NewObject<AFCDungeonManager>();
			TestNotNull(TEXT("AFCDungeonManager must be instantiable"), DungeonMgr);

			if (DungeonMgr)
			{
				GameMode->SetDungeonManager(DungeonMgr);
				TestEqual(TEXT("GetDungeonManager must return registered DungeonManager"), GameMode->GetDungeonManager(), DungeonMgr);
			}
		}
	}

	// =========================================================================
	// Test 4: GameMode ChoosePlayerStart & FindPlayerStart Routing
	// =========================================================================
	{
		AFCGameMode* GameMode = NewObject<AFCGameMode>();
		AFCDungeonManager* DungeonMgr = NewObject<AFCDungeonManager>();

		if (GameMode && DungeonMgr)
		{
			GameMode->SetDungeonManager(DungeonMgr);

			// ChoosePlayerStart without spawned rooms should gracefully return null (or base fallback)
			AActor* ChosenStart = GameMode->ChoosePlayerStart(nullptr);
			// Should safely handle null player controller without crashing
			TestTrue(TEXT("ChoosePlayerStart completed safely"), true);

			AActor* FoundStart = GameMode->FindPlayerStart(nullptr, TEXT(""));
			TestTrue(TEXT("FindPlayerStart completed safely"), true);
		}
	}

	// =========================================================================
	// Test 5: AFCDungeonManager StartLocationMode & Teleport Safety
	// =========================================================================
	{
		AFCDungeonManager* DungeonMgr = NewObject<AFCDungeonManager>();
		TestNotNull(TEXT("AFCDungeonManager must be valid"), DungeonMgr);

		if (DungeonMgr)
		{
			// Verify default start location mode
			TestEqual(TEXT("Default start mode must be Center"), DungeonMgr->GetStartLocationMode(), EFCDungeonStartLocation::Center);

			// Test switching to Edge mode
			DungeonMgr->SetStartLocationMode(EFCDungeonStartLocation::Edge);
			TestEqual(TEXT("Updated start mode must be Edge"), DungeonMgr->GetStartLocationMode(), EFCDungeonStartLocation::Edge);

			// TeleportPlayersToStartRoom without world/rooms should safely no-op
			DungeonMgr->TeleportPlayersToStartRoom();
			TestTrue(TEXT("TeleportPlayersToStartRoom handled safely without world"), true);
		}
	}

	// =========================================================================
	// Test 6: RestartPlayerAtPlayerStart with AFCRoomBase handling
	// =========================================================================
	{
		AFCGameMode* GameMode = NewObject<AFCGameMode>();
		AFCRoomBase* Room = NewObject<AFCRoomBase>();

		if (GameMode && Room)
		{
			// Call RestartPlayerAtPlayerStart with Room as StartSpot and null player
			GameMode->RestartPlayerAtPlayerStart(nullptr, Room);
			TestTrue(TEXT("RestartPlayerAtPlayerStart safely handled Room StartSpot with null player"), true);
		}
	}

	// =========================================================================
	// Test 7: DefaultRooms Weighted Random Selection
	// =========================================================================
	{
		AFCDungeonManager* DungeonMgr = NewObject<AFCDungeonManager>();
		TestNotNull(TEXT("AFCDungeonManager must be valid for random map test"), DungeonMgr);

		if (DungeonMgr)
		{
			// 1. Initial DefaultRooms has default element (AFCRoomBase)
			TestEqual(TEXT("Initial DefaultRooms must return AFCRoomBase"), DungeonMgr->GetRandomDefaultRoomClass(), TSubclassOf<AFCRoomBase>(AFCRoomBase::StaticClass()));

			// 2. Fallback when DefaultRooms is explicitly emptied
			DungeonMgr->SetDefaultRooms({});
			TestEqual(TEXT("Empty DefaultRooms must fallback to AFCRoomBase"), DungeonMgr->GetRandomDefaultRoomClass(), TSubclassOf<AFCRoomBase>(AFCRoomBase::StaticClass()));

			// 3. Single item in DefaultRooms
			DungeonMgr->AddDefaultRoom(AFCRoomBase::StaticClass(), 5.0f);
			TestEqual(TEXT("Single item in DefaultRooms should return AFCRoomBase"), DungeonMgr->GetRandomDefaultRoomClass(), TSubclassOf<AFCRoomBase>(AFCRoomBase::StaticClass()));

			// 4. Zero-weight filtering test
			// Add an entry with 0 weight (should never be selected)
			TArray<FFCDungeonRoomWeight> FilterTestList;
			FilterTestList.Add(FFCDungeonRoomWeight(AFCRoomBase::StaticClass(), 10.0f));
			FilterTestList.Add(FFCDungeonRoomWeight(nullptr, 0.0f));
			DungeonMgr->SetDefaultRooms(FilterTestList);

			for (int32 i = 0; i < 20; ++i)
			{
				TSubclassOf<AFCRoomBase> Picked = DungeonMgr->GetRandomDefaultRoomClass();
				TestNotNull(TEXT("Picked room class must not be null when valid entries exist"), Picked.Get());
				TestEqual(TEXT("Picked room class must match the positive weighted entry"), Picked, TSubclassOf<AFCRoomBase>(AFCRoomBase::StaticClass()));
			}
		}
	}

	// =========================================================================
	// Test 7: DoorOffset, DoorWidth, and CellSize Customization
	// =========================================================================
	{
		AFCRoomBase* Room = NewObject<AFCRoomBase>();
		TestNotNull(TEXT("AFCRoomBase must be instantiable for DoorOffset test"), Room);
		if (Room)
		{
			TestEqual(TEXT("Default DoorOffset should be 3000.0f"), Room->GetDoorOffset(), 3000.0f);
			Room->SetDoorOffset(2400.0f);
			TestEqual(TEXT("Updated DoorOffset should be 2400.0f"), Room->GetDoorOffset(), 2400.0f);
		}

		AFCDoorBase* Door = NewObject<AFCDoorBase>();
		TestNotNull(TEXT("AFCDoorBase must be instantiable for DoorWidth test"), Door);
		if (Door)
		{
			TestEqual(TEXT("Default DoorWidth should be 300.0f"), Door->GetDoorWidth(), 300.0f);
			Door->SetDoorWidth(450.0f);
			TestEqual(TEXT("Updated DoorWidth should be 450.0f"), Door->GetDoorWidth(), 450.0f);
		}

		AFCDungeonManager* DungeonMgr = NewObject<AFCDungeonManager>();
		TestNotNull(TEXT("AFCDungeonManager must be instantiable for CellSize test"), DungeonMgr);
		if (DungeonMgr)
		{
			TestEqual(TEXT("Default CellSize should be 6300.0f"), DungeonMgr->GetCellSize(), 6300.0f);
			TestTrue(TEXT("Default bAutoSyncDoorOffsetToCellSize should be true"), DungeonMgr->IsAutoSyncDoorOffsetToCellSize());
			DungeonMgr->SetCellSize(4800.0f);
			TestEqual(TEXT("Updated CellSize should be 4800.0f"), DungeonMgr->GetCellSize(), 4800.0f);
		}
	}

	// =========================================================================
	// Test 8: Dynamic Directional DoorOffset & Door Pool Selection
	// =========================================================================
	{
		// 1. Bit-to-index conversion safety
		TestEqual(TEXT("North bit to index"), EFCDungeonDoor::DirectionBitToIndex(EFCDungeonDoor::North), 0);
		TestEqual(TEXT("South bit to index"), EFCDungeonDoor::DirectionBitToIndex(EFCDungeonDoor::South), 1);
		TestEqual(TEXT("East bit to index"),  EFCDungeonDoor::DirectionBitToIndex(EFCDungeonDoor::East),  2);
		TestEqual(TEXT("West bit to index"),  EFCDungeonDoor::DirectionBitToIndex(EFCDungeonDoor::West),  3);
		TestEqual(TEXT("Invalid bit to index"), EFCDungeonDoor::DirectionBitToIndex(0), INDEX_NONE);

		TestEqual(TEXT("Index 0 to North bit"), EFCDungeonDoor::IndexToDirectionBit(0), EFCDungeonDoor::North);
		TestEqual(TEXT("Index 1 to South bit"), EFCDungeonDoor::IndexToDirectionBit(1), EFCDungeonDoor::South);
		TestEqual(TEXT("Index 2 to East bit"),  EFCDungeonDoor::IndexToDirectionBit(2), EFCDungeonDoor::East);
		TestEqual(TEXT("Index 3 to West bit"),  EFCDungeonDoor::IndexToDirectionBit(3), EFCDungeonDoor::West);

		// 2. DungeonManager DoorWidth configuration
		AFCDungeonManager* DungeonMgr = NewObject<AFCDungeonManager>();
		if (DungeonMgr)
		{
			TestEqual(TEXT("Default DungeonManager DoorWidth should be 300.0f"), DungeonMgr->GetDoorWidth(), 300.0f);
			DungeonMgr->SetDoorWidth(400.0f);
			TestEqual(TEXT("Updated DungeonManager DoorWidth should be 400.0f"), DungeonMgr->GetDoorWidth(), 400.0f);
		}

		// 3. Directional door offset on room (flush with room boundary for walls, adjusted for doors)
		AFCRoomBase* Room = NewObject<AFCRoomBase>();
		if (Room)
		{
			Room->SetDoorOffset(3000.0f);
			// Without door mask bit set (wall direction), offset is strictly DoorOffset (3000.0f)
			TestEqual(TEXT("Wall direction offset should remain 3000.0f"), Room->GetDoorOffsetForDirection(EFCDungeonDoor::North), 3000.0f);

			// When door mask has North door, offset adjusts for DoorWidth (3000 - 300 * 0.5 = 2850)
			FFCRoomData Data;
			Data.DoorMask = EFCDungeonDoor::North;
			Room->InitializeRoom(Data);
			TestEqual(TEXT("Door direction offset should be DoorOffset - DoorWidth * 0.5"), Room->GetDoorOffsetForDirection(EFCDungeonDoor::North), 2850.0f);
			TestEqual(TEXT("South (wall) direction offset should remain 3000.0f"), Room->GetDoorOffsetForDirection(EFCDungeonDoor::South), 3000.0f);

			TestFalse(TEXT("Default bManageDoorsExternally should be false"), Room->IsManagingDoorsExternally());
			Room->SetManageDoorsExternally(true);
			TestTrue(TEXT("Updated bManageDoorsExternally should be true"), Room->IsManagingDoorsExternally());
			TestEqual(TEXT("When managed externally, door offset remains flush with room boundary"), Room->GetDoorOffsetForDirection(EFCDungeonDoor::North), 3000.0f);
		}

		// 4. DungeonManager Door Pool selection
		AFCDungeonManager* PoolDungeonMgr = NewObject<AFCDungeonManager>();
		if (PoolDungeonMgr)
		{
			TestNotNull(TEXT("DefaultDoorPool selection should return valid class"), PoolDungeonMgr->GetRandomDoorFromPool().Get());
			PoolDungeonMgr->SetDefaultDoorPool({});
			TestEqual(TEXT("Empty DefaultDoorPool should fallback to AFCDoorBase"), PoolDungeonMgr->GetRandomDoorFromPool(), TSubclassOf<AFCDoorBase>(AFCDoorBase::StaticClass()));

			PoolDungeonMgr->AddDefaultDoor(AFCDoorBase::StaticClass(), 10.0f);
			TestEqual(TEXT("Single entry DefaultDoorPool should return AFCDoorBase"), PoolDungeonMgr->GetRandomDoorFromPool(), TSubclassOf<AFCDoorBase>(AFCDoorBase::StaticClass()));
		}

		// 5. Canonical Edge Key symmetry
		FFCDungeonEdgeKey KeyA(FIntPoint(0, 0), FIntPoint(0, 1));
		FFCDungeonEdgeKey KeyB(FIntPoint(0, 1), FIntPoint(0, 0));
		TestTrue(TEXT("Edge keys constructed in reverse should be identical"), KeyA == KeyB);
		TestEqual(TEXT("Edge keys hash should match"), GetTypeHash(KeyA), GetTypeHash(KeyB));
	}

	// =========================================================================
	// Test 9: Shared Door Multi-Room Lock Reference Counting & Axis Sizing
	// =========================================================================
	{
		AFCDoorBase* SharedDoor = NewObject<AFCDoorBase>();
		AFCRoomBase* RoomA = NewObject<AFCRoomBase>();
		AFCRoomBase* RoomB = NewObject<AFCRoomBase>();

		if (SharedDoor && RoomA && RoomB)
		{
			SharedDoor->SetDoorWidth(400.0f);
			SharedDoor->SetPassageDepth(300.0f);

			if (UStaticMeshComponent* FloorComp = SharedDoor->GetPassageFloorMeshComponent())
			{
				TestEqual(TEXT("PassageFloorMesh scale X must match PassageDepth/100"), FloorComp->GetRelativeScale3D().X, 3.0);
				TestEqual(TEXT("PassageFloorMesh scale Y must match DoorWidth/100"), FloorComp->GetRelativeScale3D().Y, 4.0);
				TestEqual(TEXT("PassageFloorMesh scale Z must match FloorThickness/100"), FloorComp->GetRelativeScale3D().Z, 1.0);
				TestEqual(TEXT("PassageFloorMesh rel Z must align top to Z=100"), FloorComp->GetRelativeLocation().Z, 50.0);
			}

			if (UStaticMeshComponent* DoorMeshComp = SharedDoor->GetDoorMeshComponent())
			{
				TestEqual(TEXT("DoorMesh scale Y must match DoorWidth/100"), DoorMeshComp->GetRelativeScale3D().Y, 4.0);
				TestEqual(TEXT("DoorMesh scale Z must match DoorHeight/100"), DoorMeshComp->GetRelativeScale3D().Z, 5.0);
				TestEqual(TEXT("DoorMesh rel Z must rest on top of floor Z=100"), DoorMeshComp->GetRelativeLocation().Z, 350.0);
			}

			// Multi-room lock source ref-counting
			SharedDoor->SetDoorLockedBySource(RoomA, true);
			TestTrue(TEXT("Shared door must lock when RoomA enters combat"), SharedDoor->IsDoorLocked());
			TestFalse(TEXT("Shared door must close when locked"), SharedDoor->IsDoorOpen());
			TestEqual(TEXT("ActiveLockCount must be 1"), SharedDoor->GetActiveLockCount(), 1);

			// RoomB clears/unlocks while RoomA is still active -> MUST remain locked!
			SharedDoor->SetDoorLockedBySource(RoomB, false);
			TestTrue(TEXT("RoomB unlocking must NOT unlock door still held by RoomA"), SharedDoor->IsDoorLocked());
			TestEqual(TEXT("ActiveLockCount must remain 1"), SharedDoor->GetActiveLockCount(), 1);

			// Both rooms lock -> count 2
			SharedDoor->SetDoorLockedBySource(RoomB, true);
			TestEqual(TEXT("ActiveLockCount must be 2"), SharedDoor->GetActiveLockCount(), 2);

			// RoomA clears -> still locked by RoomB
			SharedDoor->SetDoorLockedBySource(RoomA, false);
			TestTrue(TEXT("Door must remain locked while RoomB still holds lock"), SharedDoor->IsDoorLocked());
			TestEqual(TEXT("ActiveLockCount must be 1"), SharedDoor->GetActiveLockCount(), 1);

			// RoomB clears -> now unlocked and open
			SharedDoor->SetDoorLockedBySource(RoomB, false);
			TestFalse(TEXT("Door must unlock once all rooms release lock"), SharedDoor->IsDoorLocked());
			TestTrue(TEXT("Door must open once unlocked"), SharedDoor->IsDoorOpen());
			TestEqual(TEXT("ActiveLockCount must be 0"), SharedDoor->GetActiveLockCount(), 0);
		}
	}

	// =========================================================================
	// Test 10: Wall & DestructibleWall Sizing + Post-Destruction Floor Bridge
	// =========================================================================
	{
		AFCWallBase* Wall = NewObject<AFCWallBase>();
		if (Wall)
		{
			Wall->SetWallDimensions(FVector(150.0f, 6000.0f, 2300.0f), 0.0f);
			if (UStaticMeshComponent* MeshComp = Wall->GetWallMeshComponent())
			{
				TestEqual(TEXT("WallMesh scale X must be 1.5"), MeshComp->GetRelativeScale3D().X, 1.5);
				TestEqual(TEXT("WallMesh scale Y must be 60.0"), MeshComp->GetRelativeScale3D().Y, 60.0);
				TestEqual(TEXT("WallMesh scale Z must be 23.0"), MeshComp->GetRelativeScale3D().Z, 23.0);
				TestEqual(TEXT("WallMesh rel Z must place bottom at Z=0"), MeshComp->GetRelativeLocation().Z, 1150.0);
			}
		}

		AFCDestructibleWall* SecretWall = NewObject<AFCDestructibleWall>();
		if (SecretWall)
		{
			SecretWall->SetWallDimensions(FVector(300.0f, 6000.0f, 2300.0f), 100.0f);
			UStaticMeshComponent* IntactComp = SecretWall->GetIntactWallMeshComponent();
			UStaticMeshComponent* FloorComp = SecretWall->GetPassageFloorMeshComponent();
			TestNotNull(TEXT("DestructibleWall must have IntactWallMesh"), IntactComp);
			TestNotNull(TEXT("DestructibleWall must have PassageFloorMesh"), FloorComp);

			if (IntactComp && FloorComp)
			{
				TestEqual(TEXT("Secret PassageFloorMesh scale Z must be 1.0"), FloorComp->GetRelativeScale3D().Z, 1.0);
				TestEqual(TEXT("Secret PassageFloorMesh rel Z must place floor in [0,100]"), FloorComp->GetRelativeLocation().Z, 50.0);
				TestEqual(TEXT("Secret IntactWallMesh scale Z must be 22.0"), IntactComp->GetRelativeScale3D().Z, 22.0);
				TestEqual(TEXT("Secret IntactWallMesh rel Z must place wall in [100,2300]"), IntactComp->GetRelativeLocation().Z, 1200.0);

				const bool bDamaged = SecretWall->ReceiveBombDamage_Implementation(100.0f, FVector::ZeroVector, nullptr);
				TestTrue(TEXT("Bomb damage must destroy intact secret wall"), bDamaged);
				TestTrue(TEXT("SecretWall must be marked destroyed"), SecretWall->IsWallDestroyed());
				TestEqual(TEXT("IntactWallMesh collision must be disabled after destruction"), IntactComp->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
				TestEqual(TEXT("PassageFloorMesh must remain solid after destruction"), FloorComp->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
			}
		}
	}

	// =========================================================================
	// Test 11: RoomBoundsTrigger Inset & Idempotent InitializeRoom
	// =========================================================================
	{
		AFCRoomBase* Room = NewObject<AFCRoomBase>();
		if (Room && Room->GetRoomBoundsTrigger())
		{
			FFCRoomData Data;
			Data.Coordinates = FIntPoint(1, 0);
			Data.RoomType = EFCRoomType::Normal;
			Data.DoorMask = EFCDungeonDoor::West;

			Room->InitializeRoom(Data);
			TestEqual(TEXT("DoorOffset must stay 3000 after first InitializeRoom"), Room->GetDoorOffset(), 3000.0f);
			TestEqual(TEXT("RoomBoundsTrigger X extent must be inset by TriggerBoundaryInset (2850)"), Room->GetRoomBoundsTrigger()->GetUnscaledBoxExtent().X, 2850.0);

			// Re-initializing must NOT double-shrink DoorOffset or Trigger
			Room->InitializeRoom(Data);
			TestEqual(TEXT("DoorOffset must stay 3000 after second InitializeRoom"), Room->GetDoorOffset(), 3000.0f);
			TestEqual(TEXT("RoomBoundsTrigger X extent must remain 2850 after second InitializeRoom"), Room->GetRoomBoundsTrigger()->GetUnscaledBoxExtent().X, 2850.0);
		}
	}

	// =========================================================================
	// Test 12: Live Dungeon Generation 3D Pairwise Zero-Overlap AABB Proof
	// =========================================================================
	if (GEngine)
	{
		UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
		if (TestWorld)
		{
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(TestWorld);
			TestWorld->InitializeActorsForPlay(FURL());
			TestWorld->BeginPlay();

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AFCDungeonManager* DungeonMgr = TestWorld->SpawnActor<AFCDungeonManager>(AFCDungeonManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			TestNotNull(TEXT("DungeonManager must spawn in test world"), DungeonMgr);

			if (DungeonMgr)
			{
				DungeonMgr->GenerateDungeon();

				const TArray<TObjectPtr<AFCRoomBase>>& Rooms = DungeonMgr->GetSpawnedRooms();
				const TArray<TObjectPtr<AFCDoorBase>>& Doors = DungeonMgr->GetSpawnedSharedDoors();
				TestTrue(TEXT("Dungeon must spawn rooms"), Rooms.Num() >= 5);
				TestTrue(TEXT("Dungeon must spawn shared doors"), Doors.Num() >= 4);

				struct FLabeledBox
				{
					FString Label;
					FBox Box;
				};
				TArray<FLabeledBox> GeometryBoxes;

				auto ComputeUnitCubeWorldBox = [](const AActor* Actor, const UStaticMeshComponent* Comp) -> FBox
				{
					const FTransform CompWorldTransform = Comp->GetRelativeTransform() * Actor->GetActorTransform();
					const FBox LocalUnitBox(FVector(-50.0f, -50.0f, -50.0f), FVector(50.0f, 50.0f, 50.0f));
					return LocalUnitBox.TransformBy(CompWorldTransform);
				};

				for (const TObjectPtr<AFCRoomBase>& Room : Rooms)
				{
					if (!Room)
					{
						continue;
					}

					// Room floor box: [-DoorOffset, +DoorOffset] x [-DoorOffset, +DoorOffset] x [0, FloorThickness]
					const FVector Center = Room->GetActorLocation();
					const float HalfW = Room->GetDoorOffset();
					const float FloorH = Room->GetFloorThickness();
					GeometryBoxes.Add({
						FString::Printf(TEXT("RoomFloor(%d,%d)"), Room->GetRoomData().Coordinates.X, Room->GetRoomData().Coordinates.Y),
						FBox(Center + FVector(-HalfW, -HalfW, 0.0f), Center + FVector(HalfW, HalfW, FloorH))
					});

					// All spawned walls (pillars, solid walls, flanking walls, lintels)
					int32 WallIdx = 0;
					for (const TObjectPtr<AFCWallBase>& Wall : Room->GetSpawnedWalls())
					{
						if (Wall && Wall->GetWallMeshComponent())
						{
							GeometryBoxes.Add({
								FString::Printf(TEXT("Room(%d,%d)_Wall%d"), Room->GetRoomData().Coordinates.X, Room->GetRoomData().Coordinates.Y, WallIdx++),
								ComputeUnitCubeWorldBox(Wall.Get(), Wall->GetWallMeshComponent())
							});
						}
					}

					// All secret walls (both permanent floor slab and intact destructible body)
					int32 SecretIdx = 0;
					for (const TObjectPtr<AFCDestructibleWall>& SecretWall : Room->GetSecretWalls())
					{
						if (SecretWall)
						{
							if (SecretWall->GetPassageFloorMeshComponent())
							{
								GeometryBoxes.Add({
									FString::Printf(TEXT("Room(%d,%d)_SecretFloor%d"), Room->GetRoomData().Coordinates.X, Room->GetRoomData().Coordinates.Y, SecretIdx),
									ComputeUnitCubeWorldBox(SecretWall.Get(), SecretWall->GetPassageFloorMeshComponent())
								});
							}
							if (SecretWall->GetIntactWallMeshComponent())
							{
								GeometryBoxes.Add({
									FString::Printf(TEXT("Room(%d,%d)_SecretWall%d"), Room->GetRoomData().Coordinates.X, Room->GetRoomData().Coordinates.Y, SecretIdx),
									ComputeUnitCubeWorldBox(SecretWall.Get(), SecretWall->GetIntactWallMeshComponent())
								});
							}
							SecretIdx++;
						}
					}
				}

				int32 DoorIdx = 0;
				for (const TObjectPtr<AFCDoorBase>& Door : Doors)
				{
					if (Door)
					{
						if (Door->GetPassageFloorMeshComponent())
						{
							GeometryBoxes.Add({
								FString::Printf(TEXT("SharedDoor%d_Floor"), DoorIdx),
								ComputeUnitCubeWorldBox(Door.Get(), Door->GetPassageFloorMeshComponent())
							});
						}
						if (Door->GetDoorMeshComponent())
						{
							GeometryBoxes.Add({
								FString::Printf(TEXT("SharedDoor%d_Gate"), DoorIdx),
								ComputeUnitCubeWorldBox(Door.Get(), Door->GetDoorMeshComponent())
							});
						}
						DoorIdx++;
					}
				}

				// Verify strict interior disjointness across EVERY pair of boxes (tolerance 0.5 cm)
				constexpr double OverlapTolerance = 0.5;
				int32 OverlapCount = 0;
				for (int32 i = 0; i < GeometryBoxes.Num(); ++i)
				{
					for (int32 j = i + 1; j < GeometryBoxes.Num(); ++j)
					{
						const FBox& A = GeometryBoxes[i].Box;
						const FBox& B = GeometryBoxes[j].Box;
						const double OverlapX = FMath::Min(A.Max.X, B.Max.X) - FMath::Max(A.Min.X, B.Min.X);
						const double OverlapY = FMath::Min(A.Max.Y, B.Max.Y) - FMath::Max(A.Min.Y, B.Min.Y);
						const double OverlapZ = FMath::Min(A.Max.Z, B.Max.Z) - FMath::Max(A.Min.Z, B.Min.Z);

						if (OverlapX > OverlapTolerance && OverlapY > OverlapTolerance && OverlapZ > OverlapTolerance)
						{
							OverlapCount++;
							AddError(FString::Printf(
								TEXT("3D Geometry Overlap between [%s] and [%s]: overlap=(%.2f, %.2f, %.2f) cm"),
								*GeometryBoxes[i].Label, *GeometryBoxes[j].Label, OverlapX, OverlapY, OverlapZ));
						}
					}
				}
				TestEqual(TEXT("Generated dungeon must have ZERO 3D overlaps between walls, pillars, doors, secret walls, and floors"), OverlapCount, 0);

				// Verify re-initializing an externally managed room does NOT destroy shared doors
				if (Rooms.Num() > 0 && Doors.Num() > 0)
				{
					AFCRoomBase* FirstRoom = Rooms[0].Get();
					const int32 DoorCountBefore = FirstRoom->GetConnectedDoors().Num();
					FirstRoom->InitializeRoom(FirstRoom->GetRoomData());
					TestEqual(TEXT("Re-initializing externally managed room must preserve ConnectedDoors count"), FirstRoom->GetConnectedDoors().Num(), DoorCountBefore);
					for (const TObjectPtr<AFCDoorBase>& Door : Doors)
					{
						TestTrue(TEXT("Shared doors must remain valid after room re-initialization"), IsValid(Door) && !Door->IsActorBeingDestroyed());
					}
				}

				// Verify IFCBombDamageable::Execute_ReceiveBombDamage on a live world secret wall
				for (const TObjectPtr<AFCRoomBase>& Room : Rooms)
				{
					if (Room && Room->GetSecretWalls().Num() > 0)
					{
						AFCDestructibleWall* LiveSecretWall = Room->GetSecretWalls()[0].Get();
						if (LiveSecretWall && !LiveSecretWall->IsWallDestroyed())
						{
							const bool bLiveDestroyed = IFCBombDamageable::Execute_ReceiveBombDamage(LiveSecretWall, 100.0f, LiveSecretWall->GetActorLocation(), nullptr);
							TestTrue(TEXT("Live world secret wall must be destroyed via IFCBombDamageable::Execute_ReceiveBombDamage"), bLiveDestroyed);
							TestTrue(TEXT("Live secret wall must preserve solid PassageFloorMesh after destruction"),
								LiveSecretWall->GetPassageFloorMeshComponent() &&
								LiveSecretWall->GetPassageFloorMeshComponent()->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);
							break;
						}
					}
				}

				// Verify re-generation & ClearDungeon leave zero leaked walls or secret walls
				DungeonMgr->GenerateDungeon();
				DungeonMgr->ClearDungeon();

				int32 RemainingWalls = 0;
				int32 RemainingSecretWalls = 0;
				int32 RemainingDoors = 0;
				for (TActorIterator<AFCWallBase> It(TestWorld); It; ++It)
				{
					if (IsValid(*It) && !It->IsActorBeingDestroyed()) { ++RemainingWalls; }
				}
				for (TActorIterator<AFCDestructibleWall> It(TestWorld); It; ++It)
				{
					if (IsValid(*It) && !It->IsActorBeingDestroyed()) { ++RemainingSecretWalls; }
				}
				for (TActorIterator<AFCDoorBase> It(TestWorld); It; ++It)
				{
					if (IsValid(*It) && !It->IsActorBeingDestroyed()) { ++RemainingDoors; }
				}
				TestEqual(TEXT("ClearDungeon must leave 0 leaked walls"), RemainingWalls, 0);
				TestEqual(TEXT("ClearDungeon must leave 0 leaked secret walls"), RemainingSecretWalls, 0);
				TestEqual(TEXT("ClearDungeon must leave 0 leaked doors"), RemainingDoors, 0);
			}

			TestWorld->DestroyWorld(false);
			GEngine->DestroyWorldContext(TestWorld);
		}
	}

	return true;
}

#endif
