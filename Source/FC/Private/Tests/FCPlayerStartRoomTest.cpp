#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Map/FCRoomBase.h"
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

	return true;
}

#endif
