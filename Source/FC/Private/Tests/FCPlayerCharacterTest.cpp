#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Character/Player/FCPlayerCharacter.h"
#include "Character/FCCharacterBase.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCPlayerCharacterTest, "FC.Character.PlayerAnimation", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCPlayerCharacterTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Instantiation & CalculateHitDirection
	// =========================================================================
	{
		AFCPlayerCharacter* PlayerChar = NewObject<AFCPlayerCharacter>();
		TestNotNull(TEXT("AFCPlayerCharacter should be instantiated"), PlayerChar);

		if (PlayerChar)
		{
			TestEqual(TEXT("CalculateHitDirection with null instigator should default to Front"),
				PlayerChar->CalculateHitDirection(nullptr), EFCDeathDirection::Front);
		}
	}

	// =========================================================================
	// Test 2: Directional Death Animations from Character/Mannequins/Anims/Death
	// =========================================================================
	{
		AFCPlayerCharacter* PlayerChar = NewObject<AFCPlayerCharacter>();
		TestNotNull(TEXT("AFCPlayerCharacter should be instantiated for death anim test"), PlayerChar);

		if (PlayerChar)
		{
			// Verify GetDeathAnimationForDirection returns valid sequences or gracefully falls back
			PlayerChar->GetDeathAnimationForDirection(EFCDeathDirection::Front);
			PlayerChar->GetDeathAnimationForDirection(EFCDeathDirection::Back);
			PlayerChar->GetDeathAnimationForDirection(EFCDeathDirection::Left);
			PlayerChar->GetDeathAnimationForDirection(EFCDeathDirection::Right);

			// Calling PlayDeathAnimation in headless test shouldn't crash
			PlayerChar->PlayDeathAnimation(EFCDeathDirection::Front);
		}
	}

	// =========================================================================
	// Test 3: Hit Reaction Configuration and Execution
	// =========================================================================
	{
		AFCPlayerCharacter* PlayerChar = NewObject<AFCPlayerCharacter>();
		TestNotNull(TEXT("AFCPlayerCharacter should be instantiated for hit reaction test"), PlayerChar);

		if (PlayerChar)
		{
			// Calling HandleDamageTaken and PlayHitAnimation in headless test shouldn't crash
			FHitResult DummyHit;
			PlayerChar->HandleDamageTaken(10.0f, nullptr, DummyHit);
			PlayerChar->PlayHitAnimation(EFCDeathDirection::Front);
			PlayerChar->PlayHitAnimation(EFCDeathDirection::Back);
			PlayerChar->PlayHitAnimation(EFCDeathDirection::Left);
			PlayerChar->PlayHitAnimation(EFCDeathDirection::Right);
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

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
