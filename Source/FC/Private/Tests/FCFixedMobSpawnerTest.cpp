#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Gameplay/Spawner/FCMobSpawnerTypes.h"
#include "Gameplay/Spawner/FCMobSpawnerBase.h"
#include "Gameplay/Spawner/FCFixedMobSpawner.h"
#include "Character/Mob/FCMobCharacter.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCFixedMobSpawnerTest, "FC.Spawner.FixedMobSpawner", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCFixedMobSpawnerTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: FCFixedMobSpawner Default Configuration
	// =========================================================================
	{
		AFCFixedMobSpawner* Spawner = NewObject<AFCFixedMobSpawner>();
		TestNotNull(TEXT("AFCFixedMobSpawner should be instantiated"), Spawner);

		if (Spawner)
		{
			TestEqual(TEXT("Initial SlotCount must be 0"), Spawner->GetSlotCount(), 0);
			TestFalse(TEXT("Default IsSpawnerActive must be false prior to BeginPlay"), Spawner->IsSpawnerActive());
			TestEqual(TEXT("Default ActiveMobCount must be 0"), Spawner->GetActiveMobCount(), 0);
		}
	}

	// =========================================================================
	// Test 2: Slot Addition & Exact Count Configuration
	// =========================================================================
	{
		AFCFixedMobSpawner* Spawner = NewObject<AFCFixedMobSpawner>();
		TestNotNull(TEXT("AFCFixedMobSpawner should be instantiated"), Spawner);

		if (Spawner)
		{
			// Add Slot 0: Front Melee Guard
			FFCFixedMobSpawnSlot Slot0;
			Slot0.SlotTag = FName(TEXT("MeleeGuard"));
			Slot0.RelativeTransform = FTransform(FRotator(0.0f, 0.0f, 0.0f), FVector(200.0f, 0.0f, 0.0f));
			int32 Idx0 = Spawner->AddSpawnSlot(Slot0);
			TestEqual(TEXT("Slot 0 index should be 0"), Idx0, 0);

			// Add Slot 1: Left Archer
			FFCFixedMobSpawnSlot Slot1;
			Slot1.SlotTag = FName(TEXT("Archer"));
			Slot1.RelativeTransform = FTransform(FRotator(0.0f, 45.0f, 0.0f), FVector(0.0f, -150.0f, 0.0f));
			Slot1.bRespawnOnDeath = true;
			Slot1.RespawnDelay = 4.0f;
			int32 Idx1 = Spawner->AddSpawnSlot(Slot1);
			TestEqual(TEXT("Slot 1 index should be 1"), Idx1, 1);

			// Add Slot 2: Right Archer
			FFCFixedMobSpawnSlot Slot2;
			Slot2.SlotTag = FName(TEXT("Archer"));
			Slot2.RelativeTransform = FTransform(FRotator(0.0f, -45.0f, 0.0f), FVector(0.0f, 150.0f, 0.0f));
			int32 Idx2 = Spawner->AddSpawnSlot(Slot2);
			TestEqual(TEXT("Slot 2 index should be 2"), Idx2, 2);

			// Verify Exact Slot Count
			TestEqual(TEXT("Configured slot count must be exactly 3"), Spawner->GetSlotCount(), 3);

			// Verify Slot Queries
			TestFalse(TEXT("Slot 0 should not be alive before spawn"), Spawner->IsSlotAlive(0));
			TestFalse(TEXT("Slot 1 should not be alive before spawn"), Spawner->IsSlotAlive(1));
			TestFalse(TEXT("Slot 2 should not be alive before spawn"), Spawner->IsSlotAlive(2));
			TestFalse(TEXT("Slot 1 should not be pending respawn initially"), Spawner->IsSlotPendingRespawn(1));

			// Verify Transform Math
			FTransform OutTransform;
			bool bGotTransform = Spawner->GetSlotWorldTransform(0, OutTransform);
			TestTrue(TEXT("GetSlotWorldTransform should succeed for slot 0"), bGotTransform);
			TestEqual(TEXT("Slot 0 relative X should be 200"), OutTransform.GetLocation().X, 200.0);

			// Clear slots
			Spawner->ClearSpawnSlots();
			TestEqual(TEXT("Slot count must be 0 after ClearSpawnSlots"), Spawner->GetSlotCount(), 0);
		}
	}

	// =========================================================================
	// Test 3: Slot Mob Ownership & Death Mapping
	// =========================================================================
	{
		AFCFixedMobSpawner* Spawner = NewObject<AFCFixedMobSpawner>();
		AFCMobCharacter* Mob = NewObject<AFCMobCharacter>();

		if (Spawner && Mob)
		{
			FFCFixedMobSpawnSlot Slot;
			Slot.SlotTag = FName(TEXT("Boss"));
			Spawner->AddSpawnSlot(Slot);

			TestEqual(TEXT("FindSlotIndexForMob should return INDEX_NONE for unlinked mob"), Spawner->FindSlotIndexForMob(Mob), INDEX_NONE);
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
