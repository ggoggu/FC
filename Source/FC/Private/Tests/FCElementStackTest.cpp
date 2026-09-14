#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Combat/Element/FCElementComponent.h"
#include "Combat/FCCombatUtils.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Combat/Projectile/FCFireballProjectile.h"
#include "AbilitySystem/Abilities/FCGA_Fireball.h"
#include "Data/Card/FCCardTypes.h"
#include "UI/ViewModel/FCElementOverheadViewModel.h"
#include "UI/ViewModel/FCElementStackItemViewModel.h"
#include "Components/WidgetComponent.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCElementStackTest, "FC.Combat.ElementStackSystem", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCElementStackTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Basic Element Stack Accumulation & Query Integrity
	// =========================================================================
	UFCElementComponent* ElementComp = NewObject<UFCElementComponent>();
	TestNotNull(TEXT("UFCElementComponent should be instantiable"), ElementComp);

	if (ElementComp)
	{
		TestEqual(TEXT("Initial total element stacks must be 0"), ElementComp->GetTotalElementStacks(), 0);
		TestEqual(TEXT("Default MaxTotalStacks must be 7"), ElementComp->GetMaxTotalStacks(), 7);

		// Fireball hit simulation: Add Fire and Earth stacks (+1 each)
		ElementComp->AddElementStacks({ EFCElement::Fire, EFCElement::Earth });

		TestEqual(TEXT("Total stacks after Fireball should be 2"), ElementComp->GetTotalElementStacks(), 2);
		TestEqual(TEXT("Fire stack count should be 1"), ElementComp->GetElementCount(EFCElement::Fire), 1);
		TestEqual(TEXT("Earth stack count should be 1"), ElementComp->GetElementCount(EFCElement::Earth), 1);
		TestEqual(TEXT("Water stack count should be 0"), ElementComp->GetElementCount(EFCElement::Water), 0);

		TestTrue(TEXT("HasElementStack for Fire (1) must be true"), ElementComp->HasElementStack(EFCElement::Fire, 1));
		TestFalse(TEXT("HasElementStack for Fire (2) must be false"), ElementComp->HasElementStack(EFCElement::Fire, 2));
		TestTrue(TEXT("HasElementStacks for {Fire, Earth} must be true"), ElementComp->HasElementStacks({ EFCElement::Fire, EFCElement::Earth }));
		TestFalse(TEXT("HasElementStacks for {Fire, Water} must be false"), ElementComp->HasElementStacks({ EFCElement::Fire, EFCElement::Water }));
	}

	// =========================================================================
	// Test 2: Maximum 7 Stacks Cap & FIFO Overflow Policy
	// =========================================================================
	if (ElementComp)
	{
		// Currently has [Fire, Earth] (2 stacks). Add 5 more to reach 7.
		ElementComp->AddElementStacks({ EFCElement::Water, EFCElement::Wind, EFCElement::Lightning, EFCElement::Holy, EFCElement::Dark });
		TestEqual(TEXT("Total stacks at cap must be 7"), ElementComp->GetTotalElementStacks(), 7);

		// Stacks order: [Fire, Earth, Water, Wind, Lightning, Holy, Dark]
		const TArray<EFCElement>& CurrentStacks = ElementComp->GetAllElementStacks();
		TestEqual(TEXT("Oldest stack is Fire"), (uint8)CurrentStacks[0], (uint8)EFCElement::Fire);

		// In FIFO mode (default): Adding a new Fire stack should evict the oldest stack (Fire) and add Fire at the end
		ElementComp->AddElementStack(EFCElement::Fire, 1);
		TestEqual(TEXT("Total stacks must remain capped at 7"), ElementComp->GetTotalElementStacks(), 7);

		// The new order should start with Earth (oldest) and end with Fire (newest)
		const TArray<EFCElement>& FifoStacks = ElementComp->GetAllElementStacks();
		TestEqual(TEXT("After FIFO overflow, first element must be Earth"), (uint8)FifoStacks[0], (uint8)EFCElement::Earth);
		TestEqual(TEXT("After FIFO overflow, last element must be Fire"), (uint8)FifoStacks.Last(), (uint8)EFCElement::Fire);
	}

	// =========================================================================
	// Test 3: Clamp Overflow Policy
	// =========================================================================
	if (ElementComp)
	{
		ElementComp->SetOverflowPolicy(EFCElementOverflowPolicy::Clamp);
		TestEqual(TEXT("Overflow policy should be Clamp"), (uint8)ElementComp->GetOverflowPolicy(), (uint8)EFCElementOverflowPolicy::Clamp);

		// With 7 stacks present, trying to add in Clamp mode should fail
		const bool bAdded = ElementComp->AddElementStack(EFCElement::Water, 1);
		TestFalse(TEXT("Adding beyond 7 in Clamp mode must return false"), bAdded);
		TestEqual(TEXT("Total stacks must stay 7"), ElementComp->GetTotalElementStacks(), 7);
	}

	// =========================================================================
	// Test 4: Atomic Element Consumption (Single & Multi)
	// =========================================================================
	if (ElementComp)
	{
		// Reset to known state: [Fire, Fire, Earth]
		ElementComp->ClearAllElementStacks();
		TestEqual(TEXT("Stacks after clear must be 0"), ElementComp->GetTotalElementStacks(), 0);

		ElementComp->AddElementStacks({ EFCElement::Fire, EFCElement::Fire, EFCElement::Earth });
		TestEqual(TEXT("Total stacks should be 3"), ElementComp->GetTotalElementStacks(), 3);
		TestEqual(TEXT("Fire count should be 2"), ElementComp->GetElementCount(EFCElement::Fire), 2);
		TestEqual(TEXT("Earth count should be 1"), ElementComp->GetElementCount(EFCElement::Earth), 1);

		// 1. Failure Case: Attempt to consume elements where one is missing (Atomic preservation)
		// Needs {Fire, Water} - Water is missing!
		const bool bFailedConsume = ElementComp->ConsumeElementStacks({ EFCElement::Fire, EFCElement::Water });
		TestFalse(TEXT("Consume missing element must fail"), bFailedConsume);
		TestEqual(TEXT("Atomic check: Fire count must still be 2"), ElementComp->GetElementCount(EFCElement::Fire), 2);
		TestEqual(TEXT("Atomic check: Total stacks must remain 3"), ElementComp->GetTotalElementStacks(), 3);

		// 2. Success Case: Consume 1 Fire and 1 Earth
		const bool bSuccessConsume = ElementComp->ConsumeElementStacks({ EFCElement::Fire, EFCElement::Earth });
		TestTrue(TEXT("Consume existing {Fire, Earth} must succeed"), bSuccessConsume);
		TestEqual(TEXT("Remaining Fire count should be 1"), ElementComp->GetElementCount(EFCElement::Fire), 1);
		TestEqual(TEXT("Remaining Earth count should be 0"), ElementComp->GetElementCount(EFCElement::Earth), 0);
		TestEqual(TEXT("Total stacks should now be 1"), ElementComp->GetTotalElementStacks(), 1);

		// 3. Single consume
		const bool bSingleConsume = ElementComp->ConsumeElementStack(EFCElement::Fire, 1);
		TestTrue(TEXT("Single consume Fire should succeed"), bSingleConsume);
		TestEqual(TEXT("Total stacks should now be 0"), ElementComp->GetTotalElementStacks(), 0);
	}

	// =========================================================================
	// Test 5: Character & CombatUtils Integration
	// =========================================================================
	AFCMobCharacter* MobCharacter = NewObject<AFCMobCharacter>();
	TestNotNull(TEXT("AFCMobCharacter should be instantiable"), MobCharacter);

	if (MobCharacter)
	{
		UFCElementComponent* MobElementComp = MobCharacter->GetElementComponent();
		TestNotNull(TEXT("MobCharacter must have UFCElementComponent attached"), MobElementComp);

		if (MobElementComp)
		{
			// 1. Hit with Mage Attack Card (Fire + Earth)
			UFCCombatUtils::ApplyAttackCardHitTraits(
				nullptr,
				MobCharacter,
				EFCCharacterClass::Mage,
				EFCCardType::Attack,
				{ EFCElement::Fire, EFCElement::Earth }
			);

			TestEqual(TEXT("Mob must receive Fire stack"), MobElementComp->GetElementCount(EFCElement::Fire), 1);
			TestEqual(TEXT("Mob must receive Earth stack"), MobElementComp->GetElementCount(EFCElement::Earth), 1);
			TestEqual(TEXT("Mob total element stacks should be 2"), MobElementComp->GetTotalElementStacks(), 2);

			// 2. Hit with Mage Skill Card -> Should NOT add element stacks
			UFCCombatUtils::ApplyAttackCardHitTraits(
				nullptr,
				MobCharacter,
				EFCCharacterClass::Mage,
				EFCCardType::Skill,
				{ EFCElement::Fire }
			);
			TestEqual(TEXT("Skill card must not add element stacks"), MobElementComp->GetElementCount(EFCElement::Fire), 1);

			// 3. Hit with Neutral Attack Card -> Should NOT add element stacks
			UFCCombatUtils::ApplyAttackCardHitTraits(
				nullptr,
				MobCharacter,
				EFCCharacterClass::Neutral,
				EFCCardType::Attack,
				{ EFCElement::Fire }
			);
			TestEqual(TEXT("Neutral card must not add element stacks"), MobElementComp->GetElementCount(EFCElement::Fire), 1);
		}
	}

	// =========================================================================
	// Test 6: Projectile & Ability Element Configurations
	// =========================================================================
	AFCFireballProjectile* FireballProj = GetMutableDefault<AFCFireballProjectile>();
	TestNotNull(TEXT("AFCFireballProjectile CDO should exist"), FireballProj);
	if (FireballProj)
	{
		TestEqual(TEXT("Fireball projectile source class must be Mage"), (uint8)FireballProj->GetSourceClass(), (uint8)EFCCharacterClass::Mage);
		TestEqual(TEXT("Fireball projectile source card type must be Attack"), (uint8)FireballProj->GetSourceCardType(), (uint8)EFCCardType::Attack);
		TestTrue(TEXT("Fireball projectile elements must contain Fire"), FireballProj->GetProjectileElements().Contains(EFCElement::Fire));
		TestTrue(TEXT("Fireball projectile elements must contain Earth"), FireballProj->GetProjectileElements().Contains(EFCElement::Earth));
	}

	// =========================================================================
	// Test 7: AoE / Multi-Target FilterAndConsumeElements
	// =========================================================================
	AFCMobCharacter* TargetA = NewObject<AFCMobCharacter>();
	AFCMobCharacter* TargetB = NewObject<AFCMobCharacter>();
	AFCMobCharacter* TargetC = NewObject<AFCMobCharacter>();

	if (TargetA && TargetB && TargetC)
	{
		// TargetA has Fire
		TargetA->GetElementComponent()->AddElementStack(EFCElement::Fire, 1);
		// TargetB has Earth
		TargetB->GetElementComponent()->AddElementStack(EFCElement::Earth, 1);
		// TargetC has Fire + Earth
		TargetC->GetElementComponent()->AddElementStacks({ EFCElement::Fire, EFCElement::Earth });

		TArray<AActor*> AllTargets = { TargetA, TargetB, TargetC };
		TArray<AActor*> AffectedTargets = UFCCombatUtils::FilterAndConsumeElements(AllTargets, { EFCElement::Fire });

		// TargetA and TargetC should be affected and their Fire stack consumed
		TestEqual(TEXT("Affected targets count should be 2"), AffectedTargets.Num(), 2);
		TestTrue(TEXT("TargetA must be in affected list"), AffectedTargets.Contains(TargetA));
		TestFalse(TEXT("TargetB must NOT be in affected list"), AffectedTargets.Contains(TargetB));
		TestTrue(TEXT("TargetC must be in affected list"), AffectedTargets.Contains(TargetC));

		TestEqual(TEXT("TargetA Fire stack should be consumed"), TargetA->GetElementComponent()->GetElementCount(EFCElement::Fire), 0);
		TestEqual(TEXT("TargetB Earth stack should remain untouched"), TargetB->GetElementComponent()->GetElementCount(EFCElement::Earth), 1);
		TestEqual(TEXT("TargetC Fire stack should be consumed"), TargetC->GetElementComponent()->GetElementCount(EFCElement::Fire), 0);
		TestEqual(TEXT("TargetC Earth stack should be preserved"), TargetC->GetElementComponent()->GetElementCount(EFCElement::Earth), 1);
	}

	// =========================================================================
	// Test 8: Card Gameplay Data ConsumedElements Specification
	// =========================================================================
	FFCCardGameplayData CardData;
	CardData.CardId = FName("Card_FireConsumeSpell");
	CardData.ConsumedElements = { EFCElement::Fire, EFCElement::Fire };

	TestTrue(TEXT("Card with ConsumedElements must RequireElementConsumption"), CardData.RequiresElementConsumption());
	TestEqual(TEXT("Card should require 2 consumed elements"), CardData.ConsumedElements.Num(), 2);

	// =========================================================================
	// Test 9: Element Overhead ViewModel & Mob Overhead Widget Component
	// =========================================================================
	AFCMobCharacter* TestMob = NewObject<AFCMobCharacter>();
	TestNotNull(TEXT("AFCMobCharacter must instantiate"), TestMob);
	if (TestMob)
	{
		TestNotNull(TEXT("AFCMobCharacter must have OverheadWidgetComponent"), TestMob->GetOverheadWidgetComponent());
		if (UWidgetComponent* WidgetComp = TestMob->GetOverheadWidgetComponent())
		{
			TestEqual(TEXT("OverheadWidgetComponent space should be Screen"), WidgetComp->GetWidgetSpace(), EWidgetSpace::Screen);
		}

		UFCElementComponent* MobElemComp = TestMob->GetElementComponent();
		TestNotNull(TEXT("Mob must have ElementComponent"), MobElemComp);

		UFCElementOverheadViewModel* OverheadVM = NewObject<UFCElementOverheadViewModel>();
		TestNotNull(TEXT("UFCElementOverheadViewModel must instantiate"), OverheadVM);
		if (OverheadVM && MobElemComp)
		{
			OverheadVM->BindToElementComponent(MobElemComp);
			TestEqual(TEXT("Initial OverheadVM TotalStacks should be 0"), OverheadVM->TotalStacks, 0);
			TestFalse(TEXT("OverheadVM bHasAnyStack should be false"), OverheadVM->bHasAnyStack);
			TestEqual(TEXT("OverheadVM Visibility should be Collapsed"), OverheadVM->GetVisibilityBasedOnStacks(), ESlateVisibility::Collapsed);

			// Add Fire and Water stacks to mob
			MobElemComp->AddElementStacks({ EFCElement::Fire, EFCElement::Water });
			TestEqual(TEXT("OverheadVM TotalStacks should be 2"), OverheadVM->TotalStacks, 2);
			TestTrue(TEXT("OverheadVM bHasAnyStack should be true"), OverheadVM->bHasAnyStack);
			TestEqual(TEXT("OverheadVM Visibility should be Visible"), OverheadVM->GetVisibilityBasedOnStacks(), ESlateVisibility::Visible);
			TestEqual(TEXT("OverheadVM FireCount should be 1"), OverheadVM->FireCount, 1);
			TestEqual(TEXT("OverheadVM WaterCount should be 1"), OverheadVM->WaterCount, 1);
			TestEqual(TEXT("OverheadVM StackList size should be 2"), OverheadVM->StackList.Num(), 2);

			if (OverheadVM->StackList.Num() == 2)
			{
				TestEqual(TEXT("Stack 0 element is Fire"), OverheadVM->StackList[0]->Element, EFCElement::Fire);
				TestEqual(TEXT("Stack 1 element is Water"), OverheadVM->StackList[1]->Element, EFCElement::Water);
			}

			// Clear stacks
			MobElemComp->ClearAllElementStacks();
			TestEqual(TEXT("OverheadVM TotalStacks after clear should be 0"), OverheadVM->TotalStacks, 0);
			TestFalse(TEXT("OverheadVM bHasAnyStack after clear should be false"), OverheadVM->bHasAnyStack);
			TestEqual(TEXT("OverheadVM Visibility after clear should be Collapsed"), OverheadVM->GetVisibilityBasedOnStacks(), ESlateVisibility::Collapsed);

			OverheadVM->UnbindFromElementComponent();
		}
	}

	return true;
}

#endif
