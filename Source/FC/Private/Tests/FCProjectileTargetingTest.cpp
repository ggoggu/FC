#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Combat/FCCombatUtils.h"
#include "Character/FCCharacterBase.h"
#include "Character/Player/FCPlayerCharacter.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Gameplay/FCChestActor.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardTypes.h"
#include "AbilitySystem/Abilities/FCGA_Fireball.h"
#include "AbilitySystem/Abilities/FCGA_SpawnProjectile.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCProjectileTargetingTest, "FC.Combat.ProjectileTargeting", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCProjectileTargetingTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Verify IsAttackableTarget classification logic
	// =========================================================================
	{
		AFCPlayerCharacter* SourcePlayer = NewObject<AFCPlayerCharacter>();
		TestNotNull(TEXT("SourcePlayer must be instantiable"), SourcePlayer);

		AFCPlayerCharacter* OtherPlayer = NewObject<AFCPlayerCharacter>();
		TestNotNull(TEXT("OtherPlayer must be instantiable"), OtherPlayer);

		AFCMobCharacter* AliveMob = NewObject<AFCMobCharacter>();
		TestNotNull(TEXT("AliveMob must be instantiable"), AliveMob);

		AFCMobCharacter* DeadMob = NewObject<AFCMobCharacter>();
		TestNotNull(TEXT("DeadMob must be instantiable"), DeadMob);
		DeadMob->Die();

		AFCChestActor* UnopenedChest = NewObject<AFCChestActor>();
		TestNotNull(TEXT("UnopenedChest must be instantiable"), UnopenedChest);

		// 1. Alive Mob must be attackable
		TestTrue(TEXT("Alive mob must be attackable"), UFCCombatUtils::IsAttackableTarget(SourcePlayer, AliveMob));

		// 2. Dead Mob must NOT be attackable
		TestFalse(TEXT("Dead mob must not be attackable"), UFCCombatUtils::IsAttackableTarget(SourcePlayer, DeadMob));

		// 3. Unopened Chest must be attackable
		TestTrue(TEXT("Unopened chest must be attackable"), UFCCombatUtils::IsAttackableTarget(SourcePlayer, UnopenedChest));

		// 4. Source player targeting self must NOT be attackable
		TestFalse(TEXT("Player cannot target self"), UFCCombatUtils::IsAttackableTarget(SourcePlayer, SourcePlayer));

		// 5. Source player targeting other player must NOT be attackable (no friendly fire)
		TestFalse(TEXT("Player cannot target ally player"), UFCCombatUtils::IsAttackableTarget(SourcePlayer, OtherPlayer));

		// 6. Null candidate must NOT be attackable
		TestFalse(TEXT("Null actor must not be attackable"), UFCCombatUtils::IsAttackableTarget(SourcePlayer, nullptr));
	}

	// =========================================================================
	// Test 2: Verify Character Rotation Towards Target
	// =========================================================================
	{
		AFCPlayerCharacter* TestChar = NewObject<AFCPlayerCharacter>();
		TestNotNull(TEXT("TestChar must be instantiable"), TestChar);

		if (TestChar)
		{
			TestChar->SetActorLocation(FVector(0.0f, 0.0f, 100.0f));

			// Target directly to the East (+X): Yaw should be 0
			TestChar->RotateTowardsTarget(FVector(1000.0f, 0.0f, 100.0f));
			TestNearlyEqual(TEXT("RotateTowardsTarget East (+X) should have Yaw ~0"), (float)TestChar->GetActorRotation().Yaw, 0.0f, 1.0f);

			// Target directly to the North (+Y): Yaw should be 90
			TestChar->RotateTowardsTarget(FVector(0.0f, 1000.0f, 100.0f));
			TestNearlyEqual(TEXT("RotateTowardsTarget North (+Y) should have Yaw ~90"), (float)TestChar->GetActorRotation().Yaw, 90.0f, 1.0f);

			// Target to the North-East (+X, +Y): Yaw should be 45
			TestChar->RotateTowardsTarget(FVector(1000.0f, 1000.0f, 100.0f));
			TestNearlyEqual(TEXT("RotateTowardsTarget North-East should have Yaw ~45"), (float)TestChar->GetActorRotation().Yaw, 45.0f, 1.0f);

			// Target to the West (-X): Yaw should be 180 or -180
			TestChar->RotateTowardsTarget(FVector(-1000.0f, 0.0f, 100.0f));
			const float WestYaw = (float)FMath::Abs(TestChar->GetActorRotation().Yaw);
			TestNearlyEqual(TEXT("RotateTowardsTarget West (-X) should have |Yaw| ~180"), WestYaw, 180.0f, 1.0f);
		}
	}

	// =========================================================================
	// Test 3: Verify Character CombatTarget and TargetAimLocation Properties
	// =========================================================================
	{
		AFCPlayerCharacter* PlayerChar = NewObject<AFCPlayerCharacter>();
		AFCMobCharacter* MobTarget = NewObject<AFCMobCharacter>();

		TestNotNull(TEXT("PlayerChar must be valid"), PlayerChar);
		TestNotNull(TEXT("MobTarget must be valid"), MobTarget);

		if (PlayerChar && MobTarget)
		{
			TestNull(TEXT("Initial CombatTarget should be null"), PlayerChar->GetCombatTarget());
			TestTrue(TEXT("Initial TargetAimLocation should be zero"), PlayerChar->GetTargetAimLocation().IsZero());

			PlayerChar->SetCombatTarget(MobTarget);
			TestEqual(TEXT("CombatTarget should match MobTarget"), PlayerChar->GetCombatTarget(), (AActor*)MobTarget);

			const FVector TargetLoc(500.0f, 200.0f, 50.0f);
			PlayerChar->SetTargetAimLocation(TargetLoc);
			TestEqual(TEXT("TargetAimLocation should match TargetLoc"), PlayerChar->GetTargetAimLocation(), TargetLoc);

			PlayerChar->SetCombatTarget(nullptr);
			TestNull(TEXT("CombatTarget should clear to null"), PlayerChar->GetCombatTarget());
		}
	}

	// =========================================================================
	// Test 4: Verify Projectile Card Identification & Metadata
	// =========================================================================
	{
		UGameInstance* GI = NewObject<UGameInstance>();
		TestNotNull(TEXT("GameInstance should exist"), GI);

		if (GI)
		{
			UFCCardSubsystem* Subsystem = NewObject<UFCCardSubsystem>(GI);
			TestNotNull(TEXT("CardSubsystem should exist"), Subsystem);

			if (Subsystem)
			{
				// Fireball Card
				UFCCardDataAsset* FireballCard = NewObject<UFCCardDataAsset>(Subsystem);
				FireballCard->GameplayData.CardId = FName("Test_Fireball");
				FireballCard->GameplayData.CardAbilityClass = UFCGA_Fireball::StaticClass();
				FireballCard->GameplayData.bSpawnsProjectile = true;

				TestTrue(TEXT("Fireball card must return SpawnsProjectile = true"), FireballCard->GameplayData.SpawnsProjectile());
				TestTrue(TEXT("Fireball ability class must be UFCGA_Fireball or derived from UFCGA_SpawnProjectile"),
					FireballCard->GameplayData.CardAbilityClass->IsChildOf(UFCGA_SpawnProjectile::StaticClass()));

				// Fire Arrow Card
				UFCCardDataAsset* FireArrowCard = NewObject<UFCCardDataAsset>(Subsystem);
				FireArrowCard->GameplayData.CardId = FName("Test_FireArrow");
				FireArrowCard->GameplayData.CardAbilityClass = UFCGA_SpawnProjectile::StaticClass();
				FireArrowCard->GameplayData.bSpawnsProjectile = true;

				TestTrue(TEXT("Fire Arrow card must return SpawnsProjectile = true"), FireArrowCard->GameplayData.SpawnsProjectile());

				// Non-Projectile Card (Self Buff)
				UFCCardDataAsset* BuffCard = NewObject<UFCCardDataAsset>(Subsystem);
				BuffCard->GameplayData.CardId = FName("Test_Buff");
				BuffCard->GameplayData.TargetType = EFCCardTargetType::Self;
				BuffCard->GameplayData.bSpawnsProjectile = false;

				TestFalse(TEXT("Buff card must return SpawnsProjectile = false"), BuffCard->GameplayData.SpawnsProjectile());
			}
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
