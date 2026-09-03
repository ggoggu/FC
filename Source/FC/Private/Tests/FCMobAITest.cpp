#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Controller/AI/FCMobAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "AI/FCAITypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCMobAITest, "FC.AI.MobCharacterAndController", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCMobAITest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Blackboard Keys & AI State Enums
	// =========================================================================
	{
		TestEqual(TEXT("TargetActor key name must be 'TargetActor'"), FFCMobBlackboardKeys::TargetActor, FName(TEXT("TargetActor")));
		TestEqual(TEXT("TargetLocation key name must be 'TargetLocation'"), FFCMobBlackboardKeys::TargetLocation, FName(TEXT("TargetLocation")));
		TestEqual(TEXT("LastKnownLocation key name must be 'LastKnownLocation'"), FFCMobBlackboardKeys::LastKnownLocation, FName(TEXT("LastKnownLocation")));
		TestEqual(TEXT("HomeLocation key name must be 'HomeLocation'"), FFCMobBlackboardKeys::HomeLocation, FName(TEXT("HomeLocation")));
		TestEqual(TEXT("AIState key name must be 'AIState'"), FFCMobBlackboardKeys::AIState, FName(TEXT("AIState")));

		TestEqual(TEXT("EFCMobAIState::Idle value"), static_cast<uint8>(EFCMobAIState::Idle), 0);
		TestEqual(TEXT("EFCMobAIState::Patrol value"), static_cast<uint8>(EFCMobAIState::Patrol), 1);
		TestEqual(TEXT("EFCMobAIState::Investigating value"), static_cast<uint8>(EFCMobAIState::Investigating), 2);
		TestEqual(TEXT("EFCMobAIState::Chasing value"), static_cast<uint8>(EFCMobAIState::Chasing), 3);
	}

	// =========================================================================
	// Test 2: FCMobCharacter Humanoid Orientation & Speed Defaults
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

	return true;
}

#endif
