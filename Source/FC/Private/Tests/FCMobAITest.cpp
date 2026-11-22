#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Controller/AI/FCMobAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Gameplay/FCChestActor.h"
#include "Components/CapsuleComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCMobAITest, "FC.AI.MobCharacterAndController", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCMobAITest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: FCMobCharacter Humanoid Orientation & Speed APIs
	// =========================================================================
	{
		AFCMobCharacter* Mob = NewObject<AFCMobCharacter>();
		TestNotNull(TEXT("AFCMobCharacter should be instantiated"), Mob);

		if (Mob)
		{
			// Verify controller rotation decoupling
			TestFalse(TEXT("bUseControllerRotationPitch must be false"), Mob->bUseControllerRotationPitch);
			TestFalse(TEXT("bUseControllerRotationYaw must be false"), Mob->bUseControllerRotationYaw);
			TestFalse(TEXT("bUseControllerRotationRoll must be false"), Mob->bUseControllerRotationRoll);

			// Verify movement component orientation
			UCharacterMovementComponent* MoveComp = Mob->GetCharacterMovement();
			TestNotNull(TEXT("CharacterMovementComponent must exist"), MoveComp);
			if (MoveComp)
			{
				TestTrue(TEXT("bOrientRotationToMovement must be true"), MoveComp->bOrientRotationToMovement);
				TestEqual(TEXT("RotationRate Yaw must be 450.0"), MoveComp->RotationRate.Yaw, 450.0);
				TestEqual(TEXT("Initial MaxWalkSpeed should equal PatrolSpeed (250.0)"), MoveComp->MaxWalkSpeed, 250.0f);
			}

			// Verify Auto-possess AI configuration
			TestEqual(TEXT("AutoPossessAI must be PlacedInWorldOrSpawned"), Mob->AutoPossessAI, EAutoPossessAI::PlacedInWorldOrSpawned);
			TestEqual(TEXT("AIControllerClass must be AFCMobAIController"), Mob->AIControllerClass, TSubclassOf<AController>(AFCMobAIController::StaticClass()));

			// Test speed adjustment APIs
			TestEqual(TEXT("Default PatrolSpeed should be 250.0"), Mob->GetPatrolSpeed(), 250.0f);
			TestEqual(TEXT("Default ChaseSpeed should be 500.0"), Mob->GetChaseSpeed(), 500.0f);

			Mob->SetChaseSpeed();
			if (MoveComp)
			{
				TestEqual(TEXT("MaxWalkSpeed after SetChaseSpeed must be 500.0"), MoveComp->MaxWalkSpeed, 500.0f);
			}

			Mob->SetPatrolSpeed();
			if (MoveComp)
			{
				TestEqual(TEXT("MaxWalkSpeed after SetPatrolSpeed must be 250.0"), MoveComp->MaxWalkSpeed, 250.0f);
			}

			Mob->SetMovementSpeed(350.0f);
			if (MoveComp)
			{
				TestEqual(TEXT("MaxWalkSpeed after SetMovementSpeed must be 350.0"), MoveComp->MaxWalkSpeed, 350.0f);
			}
		}
	}

	// =========================================================================
	// Test 3: FCMobAIController Perception & Sight Sense Configuration
	// =========================================================================
	{
		AFCMobAIController* AIController = NewObject<AFCMobAIController>();
		TestNotNull(TEXT("AFCMobAIController should be instantiated"), AIController);

		if (AIController)
		{
			UAIPerceptionComponent* PerceptionComp = AIController->GetPerceptionComp();
			TestNotNull(TEXT("MobPerceptionComponent must exist"), PerceptionComp);

			UAISenseConfig_Sight* SightConfig = AIController->GetSightConfig();
			TestNotNull(TEXT("SightConfig must exist"), SightConfig);

			if (SightConfig)
			{
				TestEqual(TEXT("SightRadius should be 1500.0"), SightConfig->SightRadius, 1500.0f);
				TestEqual(TEXT("LoseSightRadius should be 2000.0"), SightConfig->LoseSightRadius, 2000.0f);
				TestEqual(TEXT("PeripheralVisionAngleDegrees should be 60.0"), SightConfig->PeripheralVisionAngleDegrees, 60.0f);
				TestTrue(TEXT("SightConfig must detect enemies"), SightConfig->DetectionByAffiliation.bDetectEnemies);
				TestTrue(TEXT("SightConfig must detect neutrals"), SightConfig->DetectionByAffiliation.bDetectNeutrals);
				TestTrue(TEXT("SightConfig must detect friendlies"), SightConfig->DetectionByAffiliation.bDetectFriendlies);
			}
		}
	}

	// =========================================================================
	// Test 3: Hit Direction Fallback, Headless Anim Safety & BP_FCHM1Character Assets
	// =========================================================================
	{
		AFCMobCharacter* Mob = NewObject<AFCMobCharacter>();
		TestNotNull(TEXT("AFCMobCharacter should be instantiated"), Mob);

		if (Mob)
		{
			TestEqual(TEXT("CalculateHitDirection with null instigator should default to Front"), Mob->CalculateHitDirection(nullptr), EFCDeathDirection::Front);

			FHitResult DummyHit;
			Mob->HandleDamageTaken(10.0f, nullptr, DummyHit);
			Mob->PlayHitAnimation(EFCDeathDirection::Front);
			Mob->PlayDeathAnimation(EFCDeathDirection::Front);
		}

		UClass* MobBPClass = StaticLoadClass(AFCMobCharacter::StaticClass(), nullptr, TEXT("/Game/Character/Enermy/hm1/BP/BP_FCHM1Character.BP_FCHM1Character_C"));
		if (MobBPClass)
		{
			AFCMobCharacter* CDO = Cast<AFCMobCharacter>(MobBPClass->GetDefaultObject());
			TestNotNull(TEXT("BP_FCHM1Character CDO should exist"), CDO);
			if (CDO)
			{
				TestNotNull(TEXT("BP_FCHM1Character should have DeathAnim_Front from Death/"), CDO->GetDeathAnimationForDirection(EFCDeathDirection::Front));
				TestNotNull(TEXT("BP_FCHM1Character should have DeathAnim_Back from Death/"), CDO->GetDeathAnimationForDirection(EFCDeathDirection::Back));
				TestNotNull(TEXT("BP_FCHM1Character should have DeathAnim_Left from Death/"), CDO->GetDeathAnimationForDirection(EFCDeathDirection::Left));
				TestNotNull(TEXT("BP_FCHM1Character should have DeathAnim_Right from Death/"), CDO->GetDeathAnimationForDirection(EFCDeathDirection::Right));
				TestNotNull(TEXT("BP_FCHM1Character should have HitAnim_Front"), CDO->GetHitAnimationForDirection(EFCDeathDirection::Front));
			}
		}
	}

	// =========================================================================
	// Test 4: Loot Drop Guard & Death Collision Disabling
	// =========================================================================
	{
		AFCMobCharacter* Mob = NewObject<AFCMobCharacter>();
		TestNotNull(TEXT("AFCMobCharacter should be instantiated for loot test"), Mob);

		if (Mob)
		{
			TestEqual(TEXT("Default DropChestClass must be AFCChestActor"), Mob->GetDropChestClass(), TSubclassOf<AActor>(AFCChestActor::StaticClass()));

			// AttemptDropChest with 0% chance should return nullptr
			Mob->SetDropChestChance(0.0f);
			TestNull(TEXT("AttemptDropChest with 0 chance must return nullptr"), Mob->AttemptDropChest());

			// Test Die() collision disabling
			TestFalse(TEXT("Mob should not be dead before Die()"), Mob->IsDead());
			Mob->Die(nullptr);
			TestTrue(TEXT("Mob must be dead after Die()"), Mob->IsDead());

			if (UCapsuleComponent* Capsule = Mob->GetCapsuleComponent())
			{
				TestEqual(TEXT("Capsule collision must be NoCollision after death"), Capsule->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			}
			if (USkeletalMeshComponent* Mesh = Mob->GetMesh())
			{
				TestEqual(TEXT("Mesh collision must be NoCollision after death"), Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			}
		}
	}

	return true;
}

#endif
