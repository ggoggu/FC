#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AbilitySystem/Effects/FCGE_MagicShield.h"
#include "AbilitySystem/Effects/FCGE_Damage.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardTypes.h"
#include "Data/Class/FCClassTypes.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCMagicShieldCardTest, "FC.Combat.MagicShieldCard", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCMagicShieldCardTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Verify Card Catalog Definition for Card_MagicShield
	// =========================================================================
	UGameInstance* DummyGameInstance = NewObject<UGameInstance>();
	TestNotNull(TEXT("GameInstance should be instantiable"), DummyGameInstance);

	if (DummyGameInstance)
	{
		UFCCardSubsystem* Subsystem = NewObject<UFCCardSubsystem>(DummyGameInstance);
		TestNotNull(TEXT("Card Subsystem should be instantiable"), Subsystem);

		if (Subsystem)
		{
			UFCCardDataAsset* MagicShieldAsset = Subsystem->GetCardDataAsset(FName("Card_MagicShield"));
			TestNotNull(TEXT("Card_MagicShield should resolve from catalog"), MagicShieldAsset);

			if (MagicShieldAsset)
			{
				TestEqual(TEXT("MagicShield Card Type must be Skill"), (uint8)MagicShieldAsset->GameplayData.CardType, (uint8)EFCCardType::Skill);
				TestEqual(TEXT("MagicShield Target Type must be Self"), (uint8)MagicShieldAsset->GameplayData.TargetType, (uint8)EFCCardTargetType::Self);
				TestEqual(TEXT("MagicShield Mana Cost should be 1"), MagicShieldAsset->GameplayData.BaseManaCost, 1);
				TestEqual(TEXT("MagicShield RequiredClass must be Mage"), (uint8)MagicShieldAsset->GameplayData.RequiredClass, (uint8)EFCCharacterClass::Mage);
				TestEqual(TEXT("MagicShield must have no elemental affinities (속성 x)"), MagicShieldAsset->GameplayData.Elements.Num(), 0);
				TestEqual(TEXT("MagicShield Base Value should be 20.0"), MagicShieldAsset->GameplayData.BaseValue, 20.0f);
				TestTrue(TEXT("MagicShield should contain UFCGE_MagicShield effect class"), MagicShieldAsset->GameplayData.CardEffectClasses.Contains(UFCGE_MagicShield::StaticClass()));
				TestEqual(TEXT("MagicShield Card Name should match"), MagicShieldAsset->DisplayData.CardName.ToString(), FString(TEXT("매직실드")));

				// Class usability rules
				TestTrue(TEXT("MagicShield must be usable by Mage"), FCClassTraitUtils::CanCardBeUsedByClass(MagicShieldAsset->GameplayData.RequiredClass, EFCCharacterClass::Mage));
				TestFalse(TEXT("MagicShield must not be usable by Warrior"), FCClassTraitUtils::CanCardBeUsedByClass(MagicShieldAsset->GameplayData.RequiredClass, EFCCharacterClass::Warrior));
				TestFalse(TEXT("MagicShield must not be usable by Rogue"), FCClassTraitUtils::CanCardBeUsedByClass(MagicShieldAsset->GameplayData.RequiredClass, EFCCharacterClass::Rogue));
			}
		}
	}

	// =========================================================================
	// Test 2: Verify UFCGE_MagicShield GameplayEffect CDO Defaults
	// =========================================================================
	UFCGE_MagicShield* ShieldCDO = GetMutableDefault<UFCGE_MagicShield>();
	TestNotNull(TEXT("UFCGE_MagicShield CDO should exist"), ShieldCDO);
	if (ShieldCDO)
	{
		TestEqual(TEXT("Shield duration policy must be Instant"), (uint8)ShieldCDO->DurationPolicy, (uint8)EGameplayEffectDurationType::Instant);
		TestEqual(TEXT("Shield GE should have 1 modifier"), ShieldCDO->Modifiers.Num(), 1);

		if (ShieldCDO->Modifiers.Num() > 0)
		{
			const FGameplayModifierInfo& ModInfo = ShieldCDO->Modifiers[0];
			TestTrue(TEXT("Modifier must target Shield attribute"), ModInfo.Attribute == UFCAttributeSet::GetShieldAttribute());
			TestEqual(TEXT("Modifier op must be Additive"), (uint8)ModInfo.ModifierOp, (uint8)EGameplayModOp::Additive);
		}
	}

	// =========================================================================
	// Test 3: AttributeSet Shield Behavior & Damage Absorption
	// =========================================================================
	AActor* TestActor = NewObject<AActor>();
	TestNotNull(TEXT("TestActor should be instantiable"), TestActor);

	if (TestActor)
	{
		UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(TestActor);
		TestNotNull(TEXT("ASC should be instantiable"), ASC);

		UFCAttributeSet* AttrSet = NewObject<UFCAttributeSet>(TestActor);
		TestNotNull(TEXT("AttrSet should be instantiable"), AttrSet);

		if (ASC && AttrSet)
		{
			// Initial defaults
			TestEqual(TEXT("Initial Shield must be 0.0"), AttrSet->GetShield(), 0.0f);
			TestEqual(TEXT("Initial Health must be 100.0"), AttrSet->GetHealth(), 100.0f);

			// Add shield of 20
			AttrSet->InitShield(20.0f);
			TestEqual(TEXT("Shield after initialization should be 20.0"), AttrSet->GetShield(), 20.0f);

			// Simulate incoming damage of 15.0 to Health
			FGameplayEffectSpec DummyDamageSpec;
			FGameplayModifierEvaluatedData EvalData1(UFCAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -15.0f);
			FGameplayEffectModCallbackData ModData1(DummyDamageSpec, EvalData1, *ASC);

			// Health would have been initially decremented to 85.0 by GAS execution before callback
			AttrSet->InitHealth(85.0f);
			AttrSet->PostGameplayEffectExecute(ModData1);

			// Shield should have absorbed 15.0 damage: Shield 20 -> 5, Health 85 + 15 -> 100
			TestEqual(TEXT("Shield should absorb 15 damage and reduce to 5.0"), AttrSet->GetShield(), 5.0f);
			TestEqual(TEXT("Health should remain fully protected at 100.0"), AttrSet->GetHealth(), 100.0f);

			// Simulate second incoming damage of 10.0 to Health
			FGameplayModifierEvaluatedData EvalData2(UFCAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -10.0f);
			FGameplayEffectModCallbackData ModData2(DummyDamageSpec, EvalData2, *ASC);

			// Health temporarily decremented by 10 (100 -> 90) before callback
			AttrSet->InitHealth(90.0f);
			AttrSet->PostGameplayEffectExecute(ModData2);

			// Shield absorbs 5 remaining and depletes to 0; Health absorbs remaining 5 (90 + 5 = 95)
			TestEqual(TEXT("Shield should be fully depleted to 0.0"), AttrSet->GetShield(), 0.0f);
			TestEqual(TEXT("Health should take residual damage and be 95.0"), AttrSet->GetHealth(), 95.0f);

			// Clamping tests
			float NegativeShield = -10.0f;
			AttrSet->PreAttributeChange(UFCAttributeSet::GetShieldAttribute(), NegativeShield);
			TestEqual(TEXT("Shield cannot drop below 0.0"), NegativeShield, 0.0f);

			float OverflowShield = 150.0f;
			AttrSet->PreAttributeChange(UFCAttributeSet::GetShieldAttribute(), OverflowShield);
			TestEqual(TEXT("Shield cannot exceed MaxShield 100.0"), OverflowShield, 100.0f);
		}
	}

	return true;
}

#endif
