#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Character/Player/FCPlayerCharacter.h"
#include "Character/FCCharacterBase.h"
#include "Character/Mob/FCMobCharacter.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCPlayerCharacterTest, "FC.Character.PlayerAnimation", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCPlayerCharacterTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Hit Direction, RotateTowardsTarget & Headless Anim Safety
	// =========================================================================
	{
		AFCPlayerCharacter* PlayerChar = NewObject<AFCPlayerCharacter>();
		TestNotNull(TEXT("AFCPlayerCharacter should be instantiated"), PlayerChar);

		if (PlayerChar)
		{
			TestEqual(TEXT("CalculateHitDirection with null instigator should default to Front"),
				PlayerChar->CalculateHitDirection(nullptr), EFCDeathDirection::Front);

			PlayerChar->SetActorLocation(FVector(0.0f, 0.0f, 100.0f));
			PlayerChar->RotateTowardsTarget(FVector(1000.0f, 0.0f, 100.0f));
			TestNearlyEqual(TEXT("RotateTowardsTarget East (+X) should have Yaw ~0"), static_cast<float>(PlayerChar->GetActorRotation().Yaw), 0.0f, 1.0f);

			PlayerChar->RotateTowardsTarget(FVector(0.0f, 1000.0f, 100.0f));
			TestNearlyEqual(TEXT("RotateTowardsTarget North (+Y) should have Yaw ~90"), static_cast<float>(PlayerChar->GetActorRotation().Yaw), 90.0f, 1.0f);

			PlayerChar->RotateTowardsTarget(FVector(1000.0f, 1000.0f, 100.0f));
			TestNearlyEqual(TEXT("RotateTowardsTarget North-East should have Yaw ~45"), static_cast<float>(PlayerChar->GetActorRotation().Yaw), 45.0f, 1.0f);

			PlayerChar->RotateTowardsTarget(FVector(-1000.0f, 0.0f, 100.0f));
			TestNearlyEqual(TEXT("RotateTowardsTarget West (-X) should have |Yaw| ~180"), static_cast<float>(FMath::Abs(PlayerChar->GetActorRotation().Yaw)), 180.0f, 1.0f);

			// Headless no-crash safety for hit/death animations
			FHitResult DummyHit;
			PlayerChar->HandleDamageTaken(10.0f, nullptr, DummyHit);
			PlayerChar->PlayHitAnimation(EFCDeathDirection::Front);
			PlayerChar->PlayDeathAnimation(EFCDeathDirection::Front);
		}
	}

	// =========================================================================
	// Test 4: BP_FCPlayerCharacter Blueprint Class & Default Death Animations
	// =========================================================================
	{
		UClass* PlayerBPClass = StaticLoadClass(AFCPlayerCharacter::StaticClass(), nullptr,
			TEXT("/Game/Character/Player/BP/BP_FCPlayerCharacter.BP_FCPlayerCharacter_C"));
		if (PlayerBPClass)
		{
			AFCPlayerCharacter* CDO = Cast<AFCPlayerCharacter>(PlayerBPClass->GetDefaultObject());
			TestNotNull(TEXT("BP_FCPlayerCharacter CDO should exist"), CDO);
			if (CDO)
			{
				TestNotNull(TEXT("BP_FCPlayerCharacter should have DeathAnim_Front from Death/"),
					CDO->GetDeathAnimationForDirection(EFCDeathDirection::Front));
				TestNotNull(TEXT("BP_FCPlayerCharacter should have DeathAnim_Back from Death/"),
					CDO->GetDeathAnimationForDirection(EFCDeathDirection::Back));
				TestNotNull(TEXT("BP_FCPlayerCharacter should have DeathAnim_Left from Death/"),
					CDO->GetDeathAnimationForDirection(EFCDeathDirection::Left));
				TestNotNull(TEXT("BP_FCPlayerCharacter should have DeathAnim_Right from Death/"),
					CDO->GetDeathAnimationForDirection(EFCDeathDirection::Right));
				TestNotNull(TEXT("BP_FCPlayerCharacter should have HitAnim_Front"),
					CDO->GetHitAnimationForDirection(EFCDeathDirection::Front));
			}
		}
	}

	// =========================================================================
	// Test 5: Health Drain Defaults & Reset
	// =========================================================================
	{
		AFCPlayerCharacter* PlayerChar = NewObject<AFCPlayerCharacter>();
		TestNotNull(TEXT("Player character should be instantiable for drain test"), PlayerChar);

		if (PlayerChar)
		{
			TestEqual(TEXT("Initial drain interval should be 10.0s"), PlayerChar->GetCurrentDrainInterval(), 10.0f);
			TestEqual(TEXT("Default heal on kill should be 5.0"), PlayerChar->GetHealOnKillAmount(), 5.0f);

			// Calling ResetHealthDrain resets interval to initial
			PlayerChar->ResetHealthDrain();
			TestEqual(TEXT("ResetHealthDrain should restore interval to 10.0s"), PlayerChar->GetCurrentDrainInterval(), 10.0f);
		}
	}

	// =========================================================================
	// Test 6: ApplyHeal and MaxHealth Clamping
	// =========================================================================
	{
		AFCPlayerCharacter* PlayerChar = NewObject<AFCPlayerCharacter>();
		TestNotNull(TEXT("Player character should be instantiable for heal test"), PlayerChar);

		if (PlayerChar)
		{
			UFCAttributeSet* AttrSet = PlayerChar->GetAttributeSet();
			TestNotNull(TEXT("Player character should have AttributeSet"), AttrSet);

			if (AttrSet)
			{
				AttrSet->InitMaxHealth(100.0f);
				AttrSet->InitHealth(50.0f);

				// Apply heal 5.0
				PlayerChar->ApplyHeal(5.0f);
				TestEqual(TEXT("Health should increase from 50 to 55"), AttrSet->GetHealth(), 55.0f);

				// Apply heal exceeding MaxHealth
				PlayerChar->ApplyHeal(100.0f);
				TestEqual(TEXT("Health should clamp to MaxHealth 100"), AttrSet->GetHealth(), 100.0f);
			}
		}
	}

	// =========================================================================
	// Test 7: Kill Reward (OnKilledEnemy)
	// =========================================================================
	{
		AFCPlayerCharacter* PlayerChar = NewObject<AFCPlayerCharacter>();
		AFCMobCharacter* MobChar = NewObject<AFCMobCharacter>();
		TestNotNull(TEXT("Player should be valid for kill reward test"), PlayerChar);
		TestNotNull(TEXT("Mob should be valid for kill reward test"), MobChar);

		if (PlayerChar && MobChar)
		{
			UFCAttributeSet* AttrSet = PlayerChar->GetAttributeSet();
			TestNotNull(TEXT("Player should have AttributeSet for kill reward test"), AttrSet);

			if (AttrSet)
			{
				AttrSet->InitMaxHealth(100.0f);
				AttrSet->InitHealth(80.0f);

				TestEqual(TEXT("Mob default HealthRewardOnKill should be 5.0"), MobChar->GetHealthRewardOnKill(), 5.0f);

				// Player kills mob -> recovers 5 health
				PlayerChar->OnKilledEnemy(MobChar);
				TestEqual(TEXT("Health should increase from 80 to 85 after killing mob"), AttrSet->GetHealth(), 85.0f);
			}
		}
	}

	// =========================================================================
	// Test 8: Jump Functionality & Death Invalidation
	// =========================================================================
	{
		AFCPlayerCharacter* PlayerChar = NewObject<AFCPlayerCharacter>();
		TestNotNull(TEXT("Player should be valid for jump test"), PlayerChar);

		if (PlayerChar)
		{
			// Character movement jump parameters
			if (UCharacterMovementComponent* MoveComp = PlayerChar->GetCharacterMovement())
			{
				TestEqual(TEXT("JumpZVelocity should be 500.0f"), MoveComp->JumpZVelocity, 500.0f);
				TestEqual(TEXT("AirControl should be 0.35f"), MoveComp->AirControl, 0.35f);
			}

			// Calling Jump & StopJumping should execute safely
			PlayerChar->Jump();
			PlayerChar->StopJumping();

			// Character can jump when alive
			TestTrue(TEXT("Alive player should be able to jump"), PlayerChar->CanJump());

			// When dead, player should not be able to jump
			PlayerChar->Die();
			TestTrue(TEXT("Player should be dead"), PlayerChar->IsDead());
			TestFalse(TEXT("Dead player should not be able to jump"), PlayerChar->CanJump());

			// Calling Jump while dead should be safely rejected
			PlayerChar->Jump();
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
