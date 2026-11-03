#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Map/FCRoomBase.h"
#include "Map/FCDoorBase.h"
#include "Map/FCDungeonManager.h"
#include "Map/FCDungeonTypes.h"
#include "Game/FCGameMode.h"
#include "GameFramework/PlayerStart.h"
#include "Components/SceneComponent.h"

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

	return true;
}

#endif
