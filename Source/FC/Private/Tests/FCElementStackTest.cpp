#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Combat/Element/FCElementComponent.h"
#include "Combat/FCCombatUtils.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Combat/Projectile/FCProjectileBase.h"
#include "Combat/Projectile/FCProjectileDataAsset.h"
#include "AbilitySystem/Abilities/FCGA_SpawnProjectile.h"
#include "Data/Card/FCCardTypes.h"
#include "Data/FCGameplayTags.h"
#include "UI/ViewModel/FCElementOverheadViewModel.h"
#include "Components/WidgetComponent.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCElementStackTest, "FC.Combat.ElementStackSystem", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCElementStackTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Basic Element Stack Accumulation & Query Integrity
	// =========================================================================
	AActor* ElementOwnerActor = NewObject<AActor>();
	UFCElementComponent* ElementComp = NewObject<UFCElementComponent>(ElementOwnerActor);
	TestNotNull(TEXT("UFCElementComponent should be instantiable"), ElementComp);

	if (ElementComp)
	{
		TestEqual(TEXT("Initial total element stacks must be 0"), ElementComp->GetTotalElementStacks(), 0);
		TestEqual(TEXT("Default MaxTotalStacks must be 7"), ElementComp->GetMaxTotalStacks(), 7);

		ElementComp->AddElementStacks({ FCGameplayTags::Element_Fire.GetTag(), FCGameplayTags::Element_Earth.GetTag() });

		TestEqual(TEXT("Total stacks after Fireball should be 2"), ElementComp->GetTotalElementStacks(), 2);
		TestEqual(TEXT("Fire stack count should be 1"), ElementComp->GetElementCount(FCGameplayTags::Element_Fire), 1);
		TestEqual(TEXT("Earth stack count should be 1"), ElementComp->GetElementCount(FCGameplayTags::Element_Earth), 1);
		TestEqual(TEXT("Water stack count should be 0"), ElementComp->GetElementCount(FCGameplayTags::Element_Water), 0);

		TestTrue(TEXT("HasElementStack for Fire (1) must be true"), ElementComp->HasElementStack(FCGameplayTags::Element_Fire, 1));
		TestFalse(TEXT("HasElementStack for Fire (2) must be false"), ElementComp->HasElementStack(FCGameplayTags::Element_Fire, 2));
		TestTrue(TEXT("HasElementStacks for {Fire, Earth} must be true"), ElementComp->HasElementStacks({ FCGameplayTags::Element_Fire.GetTag(), FCGameplayTags::Element_Earth.GetTag() }));
		TestFalse(TEXT("HasElementStacks for {Fire, Water} must be false"), ElementComp->HasElementStacks({ FCGameplayTags::Element_Fire.GetTag(), FCGameplayTags::Element_Water.GetTag() }));
	}

	// =========================================================================
	// Test 2: Maximum 7 Stacks Cap & FIFO Overflow Policy
	// =========================================================================
	if (ElementComp)
	{
		ElementComp->AddElementStacks({
			FCGameplayTags::Element_Water.GetTag(),
			FCGameplayTags::Element_Wind.GetTag(),
			FCGameplayTags::Element_Lightning.GetTag(),
			FCGameplayTags::Element_Holy.GetTag(),
			FCGameplayTags::Element_Dark.GetTag()
		});
		TestEqual(TEXT("Total stacks at cap must be 7"), ElementComp->GetTotalElementStacks(), 7);

		const TArray<FGameplayTag> CurrentStacks = ElementComp->GetAllElementStacks();
		TestTrue(TEXT("Oldest stack is Fire"), CurrentStacks.Num() == 7 && CurrentStacks[0] == FCGameplayTags::Element_Fire);

		ElementComp->AddElementStack(FCGameplayTags::Element_Fire, 1);
		TestEqual(TEXT("Total stacks must remain capped at 7"), ElementComp->GetTotalElementStacks(), 7);

		const TArray<FGameplayTag> FifoStacks = ElementComp->GetAllElementStacks();
		TestTrue(TEXT("After FIFO overflow, first element must be Earth"), FifoStacks.Num() == 7 && FifoStacks[0] == FCGameplayTags::Element_Earth);
		TestTrue(TEXT("After FIFO overflow, last element must be Fire"), FifoStacks.Num() == 7 && FifoStacks.Last() == FCGameplayTags::Element_Fire);
	}

	// =========================================================================
	// Test 3: Clamp Overflow Policy
	// =========================================================================
	if (ElementComp)
	{
		ElementComp->SetOverflowPolicy(EFCElementOverflowPolicy::Clamp);
		TestEqual(TEXT("Overflow policy should be Clamp"), (uint8)ElementComp->GetOverflowPolicy(), (uint8)EFCElementOverflowPolicy::Clamp);

		const bool bAdded = ElementComp->AddElementStack(FCGameplayTags::Element_Water, 1);
		TestFalse(TEXT("Adding beyond 7 in Clamp mode must return false"), bAdded);
		TestEqual(TEXT("Total stacks must stay 7"), ElementComp->GetTotalElementStacks(), 7);
	}

	// =========================================================================
	// Test 4: Atomic Element Consumption (Single & Multi)
	// =========================================================================
	if (ElementComp)
	{
		ElementComp->ClearAllElementStacks();
		TestEqual(TEXT("Stacks after clear must be 0"), ElementComp->GetTotalElementStacks(), 0);

		ElementComp->AddElementStacks({
			FCGameplayTags::Element_Fire.GetTag(),
			FCGameplayTags::Element_Fire.GetTag(),
			FCGameplayTags::Element_Earth.GetTag()
		});
		TestEqual(TEXT("Total stacks should be 3"), ElementComp->GetTotalElementStacks(), 3);
		TestEqual(TEXT("Fire count should be 2"), ElementComp->GetElementCount(FCGameplayTags::Element_Fire), 2);
		TestEqual(TEXT("Earth count should be 1"), ElementComp->GetElementCount(FCGameplayTags::Element_Earth), 1);

		const bool bFailedConsume = ElementComp->ConsumeElementStacks({ FCGameplayTags::Element_Fire.GetTag(), FCGameplayTags::Element_Water.GetTag() });
		TestFalse(TEXT("Consume missing element must fail"), bFailedConsume);
		TestEqual(TEXT("Atomic check: Fire count must still be 2"), ElementComp->GetElementCount(FCGameplayTags::Element_Fire), 2);
		TestEqual(TEXT("Atomic check: Total stacks must remain 3"), ElementComp->GetTotalElementStacks(), 3);

		const bool bSuccessConsume = ElementComp->ConsumeElementStacks({ FCGameplayTags::Element_Fire.GetTag(), FCGameplayTags::Element_Earth.GetTag() });
		TestTrue(TEXT("Consume existing {Fire, Earth} must succeed"), bSuccessConsume);
		TestEqual(TEXT("Remaining Fire count should be 1"), ElementComp->GetElementCount(FCGameplayTags::Element_Fire), 1);
		TestEqual(TEXT("Remaining Earth count should be 0"), ElementComp->GetElementCount(FCGameplayTags::Element_Earth), 0);
		TestEqual(TEXT("Total stacks should now be 1"), ElementComp->GetTotalElementStacks(), 1);

		const bool bSingleConsume = ElementComp->ConsumeElementStack(FCGameplayTags::Element_Fire, 1);
		TestTrue(TEXT("Single consume Fire should succeed"), bSingleConsume);
		TestEqual(TEXT("Total stacks should now be 0"), ElementComp->GetTotalElementStacks(), 0);
	}

	// =========================================================================
	// Test 5: Character & CombatUtils Polymorphic Trait Integration
	// =========================================================================
	AFCMobCharacter* MobCharacter = NewObject<AFCMobCharacter>();
	TestNotNull(TEXT("AFCMobCharacter should be instantiable"), MobCharacter);

	if (MobCharacter)
	{
		UFCElementComponent* MobElementComp = MobCharacter->GetElementComponent();
		TestNotNull(TEXT("MobCharacter must have UFCElementComponent attached"), MobElementComp);

		if (MobElementComp)
		{
			UFCCombatUtils::ApplyAttackCardHitTraits(
				nullptr,
				MobCharacter,
				FCGameplayTags::Class_Mage,
				FCGameplayTags::Card_Type_Attack,
				{ FCGameplayTags::Element_Fire.GetTag(), FCGameplayTags::Element_Earth.GetTag() }
			);

			TestEqual(TEXT("Mob must receive Fire stack"), MobElementComp->GetElementCount(FCGameplayTags::Element_Fire), 1);
			TestEqual(TEXT("Mob must receive Earth stack"), MobElementComp->GetElementCount(FCGameplayTags::Element_Earth), 1);
			TestEqual(TEXT("Mob total element stacks should be 2"), MobElementComp->GetTotalElementStacks(), 2);

			UFCCombatUtils::ApplyAttackCardHitTraits(
				nullptr,
				MobCharacter,
				FCGameplayTags::Class_Mage,
				FCGameplayTags::Card_Type_Skill,
				{ FCGameplayTags::Element_Fire.GetTag() }
			);
			TestEqual(TEXT("Skill card must not add element stacks"), MobElementComp->GetElementCount(FCGameplayTags::Element_Fire), 1);

			UFCCombatUtils::ApplyAttackCardHitTraits(
				nullptr,
				MobCharacter,
				FCGameplayTags::Class_Neutral,
				FCGameplayTags::Card_Type_Attack,
				{ FCGameplayTags::Element_Fire.GetTag() }
			);
			TestEqual(TEXT("Neutral card must not add element stacks"), MobElementComp->GetElementCount(FCGameplayTags::Element_Fire), 1);
		}
	}

	// =========================================================================
	// Test 6: Projectile & Ability Element Configurations (Data-Driven)
	// =========================================================================
	UFCProjectileDataAsset* FireballProjData = NewObject<UFCProjectileDataAsset>();
	FireballProjData->SourceClassTag = FCGameplayTags::Class_Mage;
	FireballProjData->SourceCardTypeTag = FCGameplayTags::Card_Type_Attack;
	FireballProjData->ProjectileElementTags = { FCGameplayTags::Element_Fire.GetTag(), FCGameplayTags::Element_Earth.GetTag() };

	AFCProjectileBase* FireballProj = NewObject<AFCProjectileBase>();
	TestNotNull(TEXT("AFCProjectileBase should be instantiable"), FireballProj);
	if (FireballProj)
	{
		FireballProj->InitializeFromDataAsset(FireballProjData);
		TestTrue(TEXT("Fireball projectile source class must be Mage"), FireballProj->GetSourceClassTag() == FCGameplayTags::Class_Mage);
		TestTrue(TEXT("Fireball projectile source card type must be Attack"), FireballProj->GetSourceCardTypeTag() == FCGameplayTags::Card_Type_Attack);
		TestTrue(TEXT("Fireball projectile elements must contain Fire"), FireballProj->GetProjectileElementTags().Contains(FCGameplayTags::Element_Fire.GetTag()));
		TestTrue(TEXT("Fireball projectile elements must contain Earth"), FireballProj->GetProjectileElementTags().Contains(FCGameplayTags::Element_Earth.GetTag()));
	}

	// =========================================================================
	// Test 7: AoE / Multi-Target FilterAndConsumeElements
	// =========================================================================
	AFCMobCharacter* TargetA = NewObject<AFCMobCharacter>();
	AFCMobCharacter* TargetB = NewObject<AFCMobCharacter>();
	AFCMobCharacter* TargetC = NewObject<AFCMobCharacter>();

	if (TargetA && TargetB && TargetC)
	{
		TargetA->GetElementComponent()->AddElementStack(FCGameplayTags::Element_Fire, 1);
		TargetB->GetElementComponent()->AddElementStack(FCGameplayTags::Element_Earth, 1);
		TargetC->GetElementComponent()->AddElementStacks({ FCGameplayTags::Element_Fire.GetTag(), FCGameplayTags::Element_Earth.GetTag() });

		TArray<AActor*> AllTargets = { TargetA, TargetB, TargetC };
		TArray<AActor*> AffectedTargets = UFCCombatUtils::FilterAndConsumeElements(AllTargets, { FCGameplayTags::Element_Fire.GetTag() });

		TestEqual(TEXT("Affected targets count should be 2"), AffectedTargets.Num(), 2);
		TestTrue(TEXT("TargetA must be in affected list"), AffectedTargets.Contains(TargetA));
		TestFalse(TEXT("TargetB must NOT be in affected list"), AffectedTargets.Contains(TargetB));
		TestTrue(TEXT("TargetC must be in affected list"), AffectedTargets.Contains(TargetC));

		TestEqual(TEXT("TargetA Fire stack should be consumed"), TargetA->GetElementComponent()->GetElementCount(FCGameplayTags::Element_Fire), 0);
		TestEqual(TEXT("TargetB Earth stack should remain untouched"), TargetB->GetElementComponent()->GetElementCount(FCGameplayTags::Element_Earth), 1);
		TestEqual(TEXT("TargetC Fire stack should be consumed"), TargetC->GetElementComponent()->GetElementCount(FCGameplayTags::Element_Fire), 0);
		TestEqual(TEXT("TargetC Earth stack should be preserved"), TargetC->GetElementComponent()->GetElementCount(FCGameplayTags::Element_Earth), 1);
	}

	// =========================================================================
	// Test 8: Card Gameplay Data ConsumedElementTags Specification
	// =========================================================================
	FFCCardGameplayData CardData;
	CardData.CardId = FName("Card_FireConsumeSpell");
	CardData.ConsumedElementTags = { FCGameplayTags::Element_Fire.GetTag(), FCGameplayTags::Element_Fire.GetTag() };

	TestTrue(TEXT("Card with ConsumedElementTags must RequireElementConsumption"), CardData.RequiresElementConsumption());
	TestEqual(TEXT("Card should require 2 consumed elements"), CardData.ConsumedElementTags.Num(), 2);

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

			MobElemComp->AddElementStacks({ FCGameplayTags::Element_Fire.GetTag(), FCGameplayTags::Element_Water.GetTag() });
			TestEqual(TEXT("OverheadVM TotalStacks should be 2"), OverheadVM->TotalStacks, 2);
			TestTrue(TEXT("OverheadVM bHasAnyStack should be true"), OverheadVM->bHasAnyStack);
			TestEqual(TEXT("OverheadVM Visibility should be Visible"), OverheadVM->GetVisibilityBasedOnStacks(), ESlateVisibility::Visible);
			TestEqual(TEXT("OverheadVM Fire count should be 1"), OverheadVM->GetElementCount(FCGameplayTags::Element_Fire), 1);
			TestEqual(TEXT("OverheadVM Water count should be 1"), OverheadVM->GetElementCount(FCGameplayTags::Element_Water), 1);
			TestEqual(TEXT("OverheadVM CurrentStacks size should be 2"), OverheadVM->CurrentStacks.Num(), 2);

			if (OverheadVM->CurrentStacks.Num() == 2)
			{
				TestTrue(TEXT("Stack 0 element is Fire"), OverheadVM->CurrentStacks[0] == FCGameplayTags::Element_Fire);
				TestTrue(TEXT("Stack 1 element is Water"), OverheadVM->CurrentStacks[1] == FCGameplayTags::Element_Water);
			}

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
