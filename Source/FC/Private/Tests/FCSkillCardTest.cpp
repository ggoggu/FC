#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AbilitySystem/Abilities/FCGA_Fireball.h"
#include "AbilitySystem/Effects/FCGE_AttackBuff.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardTypes.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCSkillCardTest, "FC.Combat.SkillCard", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCSkillCardTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Verify Card Catalog Definitions (Skill vs Attack classification)
	// =========================================================================
	UGameInstance* DummyGameInstance = NewObject<UGameInstance>();
	TestNotNull(TEXT("GameInstance should be instantiable"), DummyGameInstance);

	if (DummyGameInstance)
	{
		UFCCardSubsystem* Subsystem = NewObject<UFCCardSubsystem>(DummyGameInstance);
		TestNotNull(TEXT("Card Subsystem should be instantiable"), Subsystem);

		if (Subsystem)
		{
			// Verify Skill Card: Card_AttackBuff (1-minute +1 Attack Power)
			UFCCardDataAsset* AttackBuffAsset = Subsystem->GetCardDataAsset(FName("Card_AttackBuff"));
			TestNotNull(TEXT("Card_AttackBuff should resolve from catalog"), AttackBuffAsset);

			if (AttackBuffAsset)
			{
				TestEqual(TEXT("AttackBuff Card Type must be Skill"), (uint8)AttackBuffAsset->GameplayData.CardType, (uint8)EFCCardType::Skill);
				TestEqual(TEXT("AttackBuff Target Type must be Self"), (uint8)AttackBuffAsset->GameplayData.TargetType, (uint8)EFCCardTargetType::Self);
				TestEqual(TEXT("AttackBuff Mana Cost should be 1"), AttackBuffAsset->GameplayData.BaseManaCost, 1);
				TestEqual(TEXT("AttackBuff Base Value should be 1.0"), AttackBuffAsset->GameplayData.BaseValue, 1.0f);
				TestTrue(TEXT("AttackBuff should have UFCGE_AttackBuff effect class"), AttackBuffAsset->GameplayData.CardEffectClasses.Contains(UFCGE_AttackBuff::StaticClass()));
				TestEqual(TEXT("AttackBuff Card Name should match"), AttackBuffAsset->DisplayData.CardName.ToString(), FString(TEXT("공격력 강화")));
			}

			// Verify Attack Card: Card_Fireball (Direct projectile damage)
			UFCCardDataAsset* FireballAsset = Subsystem->GetCardDataAsset(FName("Card_Fireball"));
			TestNotNull(TEXT("Card_Fireball should resolve from catalog"), FireballAsset);

			if (FireballAsset)
			{
				TestEqual(TEXT("Fireball Card Type must be Attack"), (uint8)FireballAsset->GameplayData.CardType, (uint8)EFCCardType::Attack);
				TestEqual(TEXT("Fireball Target Type must be DirectionalAoE"), (uint8)FireballAsset->GameplayData.TargetType, (uint8)EFCCardTargetType::DirectionalAoE);
				TestTrue(TEXT("Fireball Ability should be UFCGA_Fireball"), FireballAsset->GameplayData.CardAbilityClass == UFCGA_Fireball::StaticClass());
			}
		}
	}

	// =========================================================================
	// Test 2: Verify UFCGE_AttackBuff GameplayEffect CDO Defaults
	// =========================================================================
	UFCGE_AttackBuff* BuffCDO = GetMutableDefault<UFCGE_AttackBuff>();
	TestNotNull(TEXT("UFCGE_AttackBuff CDO should exist"), BuffCDO);
	if (BuffCDO)
	{
		TestEqual(TEXT("Buff duration policy must be HasDuration"), (uint8)BuffCDO->DurationPolicy, (uint8)EGameplayEffectDurationType::HasDuration);
		TestEqual(TEXT("Buff should have 1 modifier"), BuffCDO->Modifiers.Num(), 1);

		if (BuffCDO->Modifiers.Num() > 0)
		{
			const FGameplayModifierInfo& ModInfo = BuffCDO->Modifiers[0];
			TestTrue(TEXT("Modifier must target AttackPower attribute"), ModInfo.Attribute == UFCAttributeSet::GetAttackPowerAttribute());
			TestEqual(TEXT("Modifier op must be Additive"), (uint8)ModInfo.ModifierOp, (uint8)EGameplayModOp::Additive);
		}
	}

	// =========================================================================
	// Test 3: AttributeSet AttackPower Behavior & Clamping
	// =========================================================================
	AActor* TestActor = NewObject<AActor>();
	TestNotNull(TEXT("TestActor should be instantiable"), TestActor);

	if (TestActor)
	{
		UFCAttributeSet* AttrSet = NewObject<UFCAttributeSet>(TestActor);
		TestNotNull(TEXT("AttrSet should be instantiable"), AttrSet);

		if (AttrSet)
		{
			TestEqual(TEXT("Initial AttackPower must be 0.0"), AttrSet->GetAttackPower(), 0.0f);

			AttrSet->InitAttackPower(1.0f);
			TestEqual(TEXT("AttackPower after modification should be 1.0"), AttrSet->GetAttackPower(), 1.0f);

			// Test clamping (cannot drop below 0.0)
			float NegativeVal = -5.0f;
			AttrSet->PreAttributeChange(UFCAttributeSet::GetAttackPowerAttribute(), NegativeVal);
			TestEqual(TEXT("AttackPower PreAttributeChange should clamp to 0.0"), NegativeVal, 0.0f);
		}
	}

	return true;
}

#endif
