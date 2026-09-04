#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Gameplay/Spawner/FCMobSpawnerTypes.h"
#include "Gameplay/Spawner/FCMobSpawnerBase.h"
#include "Gameplay/Spawner/FCMobCampSpawner.h"
#include "Gameplay/Spawner/FCMobSpawnSubsystem.h"
#include "Character/Mob/FCMobCharacter.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCMobCampSpawnerTest, "FC.Spawner.MobCampSpawnerAndSubsystem", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCMobCampSpawnerTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Spawner Location Types & Struct Defaults
	// =========================================================================
	{
		TestEqual(TEXT("EFCMobSpawnLocationType::RandomInRadius value"), static_cast<uint8>(EFCMobSpawnLocationType::RandomInRadius), 0);
		TestEqual(TEXT("EFCMobSpawnLocationType::AtSpawner value"), static_cast<uint8>(EFCMobSpawnLocationType::AtSpawner), 1);
		TestEqual(TEXT("EFCMobSpawnLocationType::DesignatedSpawnPoints value"), static_cast<uint8>(EFCMobSpawnLocationType::DesignatedSpawnPoints), 2);

		FFCMobSpawnEntry Entry;
		TestNull(TEXT("Default MobClass should be null"), Entry.MobClass.Get());
		TestEqual(TEXT("Default Weight should be 1.0"), Entry.Weight, 1.0f);
	}

	// =========================================================================
	// Test 2: FCMobCampSpawner Default Configuration & Range Parameters
	// =========================================================================
	{
		AFCMobCampSpawner* CampSpawner = NewObject<AFCMobCampSpawner>();
		TestNotNull(TEXT("AFCMobCampSpawner should be instantiated"), CampSpawner);

		if (CampSpawner)
		{
			// Verify Population Range Defaults
			TestEqual(TEXT("Default MinMobCount must be 3"), CampSpawner->GetMinMobCount(), 3);
			TestEqual(TEXT("Default MaxMobCount must be 5"), CampSpawner->GetMaxMobCount(), 5);
			TestEqual(TEXT("Default TargetMobCount must be 3"), CampSpawner->GetTargetMobCount(), 3);

			// Verify Spawner Base Defaults
			TestEqual(TEXT("Default SpawnRadius must be 800.0"), CampSpawner->GetSpawnRadius(), 800.0f);
			TestFalse(TEXT("Default IsSpawnerActive must be false prior to BeginPlay"), CampSpawner->IsSpawnerActive());
			TestEqual(TEXT("Default ActiveMobCount must be 0"), CampSpawner->GetActiveMobCount(), 0);

			// Verify Pending flags
			TestFalse(TEXT("IsWipeRespawnPending must be false initially"), CampSpawner->IsWipeRespawnPending());
			TestFalse(TEXT("IsReplenishPending must be false initially"), CampSpawner->IsReplenishPending());
		}
	}

	// =========================================================================
	// Test 3: FCMobCharacter OwningSpawner Linkage & Death Lifecycle
	// =========================================================================
	{
		AFCMobCharacter* Mob = NewObject<AFCMobCharacter>();
		TestNotNull(TEXT("AFCMobCharacter should be instantiated"), Mob);

		AFCMobCampSpawner* CampSpawner = NewObject<AFCMobCampSpawner>();
		TestNotNull(TEXT("AFCMobCampSpawner should be instantiated"), CampSpawner);

		if (Mob && CampSpawner)
		{
			TestNull(TEXT("Initial OwningSpawner should be null"), Mob->GetOwningSpawner());

			Mob->SetOwningSpawner(CampSpawner);
			TestEqual(TEXT("OwningSpawner must match assigned spawner"), Mob->GetOwningSpawner(), static_cast<AFCMobSpawnerBase*>(CampSpawner));

			TestFalse(TEXT("Mob should not be dead initially"), Mob->IsDead());
			TestEqual(TEXT("Default DeathDespawnDelay should be 5.0"), Mob->GetDeathDespawnDelay(), 5.0f);

			// Trigger Die
			Mob->Die(nullptr);
			TestTrue(TEXT("Mob must be dead after Die() call"), Mob->IsDead());
		}
	}

	// =========================================================================
	// Test 4: FCMobSpawnSubsystem Registration & Query Logic
	// =========================================================================
	{
		UFCMobSpawnSubsystem* Subsystem = NewObject<UFCMobSpawnSubsystem>();
		TestNotNull(TEXT("UFCMobSpawnSubsystem should be instantiated"), Subsystem);

		AFCMobCampSpawner* SpawnerA = NewObject<AFCMobCampSpawner>();
		AFCMobCampSpawner* SpawnerB = NewObject<AFCMobCampSpawner>();

		if (Subsystem && SpawnerA && SpawnerB)
		{
			SpawnerA->Tags.Add(FName(TEXT("DungeonZone")));
			SpawnerB->Tags.Add(FName(TEXT("FieldZone")));

			Subsystem->RegisterSpawner(SpawnerA);
			Subsystem->RegisterSpawner(SpawnerB);

			TArray<AFCMobSpawnerBase*> AllSpawners;
			Subsystem->GetAllSpawners(AllSpawners);
			TestEqual(TEXT("Total registered spawners must be 2"), AllSpawners.Num(), 2);

			TArray<AFCMobSpawnerBase*> DungeonSpawners;
			Subsystem->FindSpawnersByTag(FName(TEXT("DungeonZone")), DungeonSpawners);
			TestEqual(TEXT("DungeonZone spawners count must be 1"), DungeonSpawners.Num(), 1);
			if (DungeonSpawners.Num() > 0)
			{
				TestEqual(TEXT("DungeonZone spawner must be SpawnerA"), DungeonSpawners[0], static_cast<AFCMobSpawnerBase*>(SpawnerA));
			}

			// Duplicate registration guard
			Subsystem->RegisterSpawner(SpawnerA);
			Subsystem->GetAllSpawners(AllSpawners);
			TestEqual(TEXT("Spawners count must remain 2 after duplicate registration"), AllSpawners.Num(), 2);

			// Unregister
			Subsystem->UnregisterSpawner(SpawnerA);
			Subsystem->GetAllSpawners(AllSpawners);
			TestEqual(TEXT("Spawners count must be 1 after unregistering SpawnerA"), AllSpawners.Num(), 1);
		}
	}

	return true;
}

#endif
