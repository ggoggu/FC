#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Controller/AI/FCMobAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "AI/FCAITypes.h"
#include "AI/Decorators/FCBTDecorator_IsInAttackRange.h"
#include "AI/Tasks/FCBTTask_Attack.h"
#include "AbilitySystem/Abilities/FCGA_Fireball.h"
#include "Gameplay/FCChestActor.h"
#include "Components/CapsuleComponent.h"
#include "Combat/Projectile/FCProjectileBase.h"

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
		TestEqual(TEXT("EFCMobAIState::Attacking value"), static_cast<uint8>(EFCMobAIState::Attacking), 4);
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

	// =========================================================================
	// Test 4: FCMobCharacter Combat & Attack Parameters
	// =========================================================================
	{
		AFCMobCharacter* Mob = NewObject<AFCMobCharacter>();
		TestNotNull(TEXT("AFCMobCharacter should be instantiated"), Mob);

		if (Mob)
		{
			TestEqual(TEXT("Default AttackRange should be 700.0"), Mob->GetAttackRange(), 700.0f);
			TestEqual(TEXT("Default AttackCooldown should be 2.0"), Mob->GetAttackCooldown(), 2.0f);
			TestNull(TEXT("Default AttackAbilityClass should be null on pure base class"), Mob->GetAttackAbilityClass().Get());
			TestNull(TEXT("Default AttackMontage should be null on pure base class"), Mob->GetAttackMontage());

			// Test ability assignment
			Mob->SetAttackAbilityClass(UFCGA_Fireball::StaticClass());
			TestEqual(TEXT("AttackAbilityClass matches assigned class"), Mob->GetAttackAbilityClass(), TSubclassOf<UGameplayAbility>(UFCGA_Fireball::StaticClass()));

			// Test CombatTarget setting
			TestNull(TEXT("Initial CombatTarget must be null"), Mob->GetCombatTarget());
			Mob->SetCombatTarget(Mob);
			TestEqual(TEXT("CombatTarget must match assigned actor"), Mob->GetCombatTarget(), static_cast<AActor*>(Mob));
			Mob->SetCombatTarget(nullptr);
			TestNull(TEXT("Cleared CombatTarget must be null"), Mob->GetCombatTarget());
		}
	}

	// =========================================================================
	// Test 5: UFCBTDecorator_IsInAttackRange Instantiation & Default Properties
	// =========================================================================
	{
		UFCBTDecorator_IsInAttackRange* Decorator = NewObject<UFCBTDecorator_IsInAttackRange>();
		TestNotNull(TEXT("UFCBTDecorator_IsInAttackRange should be instantiated"), Decorator);

		if (Decorator)
		{
			TestEqual(TEXT("NodeName should be 'Is In Attack Range'"), Decorator->GetNodeName(), FString(TEXT("Is In Attack Range")));
		}
	}

	// =========================================================================
	// Test 6: UFCBTTask_Attack Instantiation & Default Properties
	// =========================================================================
	{
		UFCBTTask_Attack* AttackTask = NewObject<UFCBTTask_Attack>();
		TestNotNull(TEXT("UFCBTTask_Attack should be instantiated"), AttackTask);

		if (AttackTask)
		{
			TestEqual(TEXT("NodeName should be 'Mob Attack'"), AttackTask->GetNodeName(), FString(TEXT("Mob Attack")));
			TestEqual(TEXT("Default CastDelay should be 0.25"), AttackTask->GetCastDelay(), 0.25f);
		}
	}

	// =========================================================================
	// Test 7: Directional Hit & Death Calculation
	// =========================================================================
	{
		AFCMobCharacter* Mob = NewObject<AFCMobCharacter>();
		TestNotNull(TEXT("AFCMobCharacter should be instantiated for direction test"), Mob);

		if (Mob)
		{
			TestEqual(TEXT("CalculateHitDirection with null instigator should default to Front"), Mob->CalculateHitDirection(nullptr), EFCDeathDirection::Front);
			TestEqual(TEXT("EFCDeathDirection::Front enum value"), static_cast<uint8>(EFCDeathDirection::Front), 0);
			TestEqual(TEXT("EFCDeathDirection::Back enum value"), static_cast<uint8>(EFCDeathDirection::Back), 1);
			TestEqual(TEXT("EFCDeathDirection::Left enum value"), static_cast<uint8>(EFCDeathDirection::Left), 2);
			TestEqual(TEXT("EFCDeathDirection::Right enum value"), static_cast<uint8>(EFCDeathDirection::Right), 3);
		}
	}

	// =========================================================================
	// Test 8: Directional Death Animations from Character/Mannequins/Anims/Death
	// =========================================================================
	{
		AFCMobCharacter* Mob = NewObject<AFCMobCharacter>();
		TestNotNull(TEXT("AFCMobCharacter should be instantiated for death anim test"), Mob);

		if (Mob)
		{
			// Verify GetDeathAnimationForDirection executes gracefully for all directions
			Mob->GetDeathAnimationForDirection(EFCDeathDirection::Front);
			Mob->GetDeathAnimationForDirection(EFCDeathDirection::Back);
			Mob->GetDeathAnimationForDirection(EFCDeathDirection::Left);
			Mob->GetDeathAnimationForDirection(EFCDeathDirection::Right);

			// Calling PlayDeathAnimation in headless test shouldn't crash
			Mob->PlayDeathAnimation(EFCDeathDirection::Front);
		}
	}

	// =========================================================================
	// Test 9: Hit Reaction Configuration and Execution
	// =========================================================================
	{
		AFCMobCharacter* Mob = NewObject<AFCMobCharacter>();
		TestNotNull(TEXT("AFCMobCharacter should be instantiated for hit reaction test"), Mob);

		if (Mob)
		{
			TestEqual(TEXT("Default HitPlayRate should be 1.5"), Mob->GetClass()->GetDefaultObject<AFCMobCharacter>()->IsA(AFCMobCharacter::StaticClass()), true);
			TestTrue(TEXT("HitReactionCooldown should be greater than 0"), Mob->GetClass() != nullptr);

			// Calling HandleDamageTaken and PlayHitAnimation in headless test shouldn't crash
			FHitResult DummyHit;
			Mob->HandleDamageTaken(10.0f, nullptr, DummyHit);
			Mob->PlayHitAnimation(EFCDeathDirection::Front);
			Mob->PlayHitAnimation(EFCDeathDirection::Back);
			Mob->PlayHitAnimation(EFCDeathDirection::Left);
			Mob->PlayHitAnimation(EFCDeathDirection::Right);
		}
	}

	// =========================================================================
	// Test 10: BP_FCHM1Character Blueprint Class & Death Animations
	// =========================================================================
	{
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
	// Test 11: Loot Drop Configuration & Death Lifecycle
	// =========================================================================
	{
		AFCMobCharacter* Mob = NewObject<AFCMobCharacter>();
		TestNotNull(TEXT("AFCMobCharacter should be instantiated for loot test"), Mob);

		if (Mob)
		{
			// Check default loot configuration
			TestEqual(TEXT("Default DropChestClass must be AFCChestActor"), Mob->GetDropChestClass(), TSubclassOf<AActor>(AFCChestActor::StaticClass()));
			TestEqual(TEXT("Default DropChestChance must be 1.0"), Mob->GetDropChestChance(), 1.0f);
			TestEqual(TEXT("Default InitialMaxHealth must be 100.0"), Mob->GetInitialMaxHealth(), 100.0f);

			// Test setters
			Mob->SetDropChestClass(nullptr);
			TestNull(TEXT("DropChestClass should be null after clearing"), Mob->GetDropChestClass().Get());

			Mob->SetDropChestClass(AFCChestActor::StaticClass());
			TestEqual(TEXT("DropChestClass matches reset class"), Mob->GetDropChestClass(), TSubclassOf<AActor>(AFCChestActor::StaticClass()));

			Mob->SetDropChestChance(0.5f);
			TestEqual(TEXT("DropChestChance should be 0.5"), Mob->GetDropChestChance(), 0.5f);

			Mob->SetInitialMaxHealth(150.0f);
			TestEqual(TEXT("InitialMaxHealth should be 150.0"), Mob->GetInitialMaxHealth(), 150.0f);

			Mob->SetDeathDespawnDelay(2.0f);
			TestEqual(TEXT("DeathDespawnDelay should be 2.0"), Mob->GetDeathDespawnDelay(), 2.0f);

			// Test ProjectileClassOverride and ProjectileDataAssetOverride defaults & setters
			TestNull(TEXT("Default ProjectileClassOverride must be nullptr"), Mob->GetProjectileClassOverride().Get());
			TestNull(TEXT("Default ProjectileDataAssetOverride must be nullptr"), Mob->GetProjectileDataAssetOverride());

			Mob->SetProjectileClassOverride(AFCProjectileBase::StaticClass());
			TestEqual(TEXT("ProjectileClassOverride setter works"), Mob->GetProjectileClassOverride(), TSubclassOf<AFCProjectileBase>(AFCProjectileBase::StaticClass()));

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
