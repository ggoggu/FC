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
			// Register Skill Card: Card_AttackBuff
			UFCCardDataAsset* NewBuff = NewObject<UFCCardDataAsset>(Subsystem);
			NewBuff->GameplayData.CardId = FName("Card_AttackBuff");
			NewBuff->GameplayData.BaseManaCost = 2;
			NewBuff->GameplayData.CardType = EFCCardType::Skill;
			NewBuff->GameplayData.TargetType = EFCCardTargetType::Self;
			NewBuff->GameplayData.BaseValue = 1.0f;
			NewBuff->GameplayData.CardEffectClasses.Add(UFCGE_AttackBuff::StaticClass());
			NewBuff->GameplayData.RequiredClass = EFCCharacterClass::Neutral;
			NewBuff->DisplayData.CardName = FText::FromString(TEXT("공격력 강화"));
			NewBuff->DisplayData.CardDescription = FText::FromString(TEXT("1분 동안 자신의 공격력을 1 증가시킵니다."));
			NewBuff->DisplayData.Rarity = EFCCardRarity::Common;
			Subsystem->RegisterCardDataAsset(NewBuff);

			// Register Attack Card: Card_Fireball
			UFCCardDataAsset* NewFireball = NewObject<UFCCardDataAsset>(Subsystem);
			NewFireball->GameplayData.CardId = FName("Card_Fireball");
			NewFireball->GameplayData.BaseManaCost = 2;
			NewFireball->GameplayData.CardType = EFCCardType::Attack;
			NewFireball->GameplayData.TargetType = EFCCardTargetType::DirectionalAoE;
			NewFireball->GameplayData.BaseValue = 1.0f;
			NewFireball->GameplayData.CardAbilityClass = UFCGA_Fireball::StaticClass();
			NewFireball->GameplayData.RequiredClass = EFCCharacterClass::Mage;
			NewFireball->GameplayData.Elements = { EFCElement::Fire, EFCElement::Earth };
			NewFireball->DisplayData.CardName = FText::FromString(TEXT("파이어 볼"));
			NewFireball->DisplayData.CardDescription = FText::FromString(TEXT("전방으로 화염구를 직선 발사하여 적중한 대상에게 1의 피해를 입힙니다."));
			NewFireball->DisplayData.Rarity = EFCCardRarity::Common;
			Subsystem->RegisterCardDataAsset(NewFireball);

			// Verify Skill Card: Card_AttackBuff (1-minute +1 Attack Power)
			UFCCardDataAsset* AttackBuffAsset = Subsystem->GetCardDataAsset(FName("Card_AttackBuff"));
			TestNotNull(TEXT("Card_AttackBuff should resolve from catalog"), AttackBuffAsset);

			if (AttackBuffAsset)
			{
				TestEqual(TEXT("AttackBuff Card Type must be Skill"), (uint8)AttackBuffAsset->GameplayData.CardType, (uint8)EFCCardType::Skill);
				TestEqual(TEXT("AttackBuff Target Type must be Self"), (uint8)AttackBuffAsset->GameplayData.TargetType, (uint8)EFCCardTargetType::Self);
				TestEqual(TEXT("AttackBuff Mana Cost should be 2"), AttackBuffAsset->GameplayData.BaseManaCost, 2);
				TestEqual(TEXT("AttackBuff Base Value should be 1.0"), AttackBuffAsset->GameplayData.BaseValue, 1.0f);
				TestTrue(TEXT("AttackBuff should have UFCGE_AttackBuff effect class"), AttackBuffAsset->GameplayData.CardEffectClasses.Contains(UFCGE_AttackBuff::StaticClass()));
				TestEqual(TEXT("AttackBuff Card Name should match"), AttackBuffAsset->DisplayData.CardName.ToString(), FString(TEXT("공격력 강화")));

				// Neutral Class & Trait Check
				TestEqual(TEXT("AttackBuff RequiredClass must be Neutral"), (uint8)AttackBuffAsset->GameplayData.RequiredClass, (uint8)EFCCharacterClass::Neutral);
				TestTrue(TEXT("AttackBuff must be Neutral"), AttackBuffAsset->GameplayData.IsNeutral());
				TestFalse(TEXT("Neutral card cannot hold elements"), AttackBuffAsset->GameplayData.CanHaveElements());
				TestTrue(TEXT("AttackBuff must be usable by Mage"), FCClassTraitUtils::CanCardBeUsedByClass(AttackBuffAsset->GameplayData.RequiredClass, EFCCharacterClass::Mage));
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
