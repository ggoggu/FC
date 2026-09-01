#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AbilitySystem/Abilities/FCGA_Fireball.h"
#include "AbilitySystem/Abilities/FCGA_SpawnProjectile.h"
#include "Combat/Projectile/FCFireballProjectile.h"
#include "Combat/Projectile/FCProjectileBase.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardTypes.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCFireballCardTest, "FC.Combat.FireballCard", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCFireballCardTest::RunTest(const FString& Parameters)
{
	// Test 1: Verify Fireball Card Catalog Resolution & Data Integrity
	UGameInstance* DummyGameInstance = NewObject<UGameInstance>();
	TestNotNull(TEXT("GameInstance should be instantiable"), DummyGameInstance);

	if (DummyGameInstance)
	{
		UFCCardSubsystem* Subsystem = NewObject<UFCCardSubsystem>(DummyGameInstance);
		TestNotNull(TEXT("Card Subsystem should be instantiable"), Subsystem);

		if (Subsystem)
		{
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

			UFCCardDataAsset* FireballAsset = Subsystem->GetCardDataAsset(FName("Card_Fireball"));
			TestNotNull(TEXT("Card_Fireball should resolve from catalog"), FireballAsset);

			if (FireballAsset)
			{
				TestEqual(TEXT("Fireball Mana Cost should be 2"), FireballAsset->GameplayData.BaseManaCost, 2);
				TestEqual(TEXT("Fireball Base Damage should be 1.0"), FireballAsset->GameplayData.BaseValue, 1.0f);
				TestEqual(TEXT("Fireball Card Type should be Attack"), (uint8)FireballAsset->GameplayData.CardType, (uint8)EFCCardType::Attack);
				TestTrue(TEXT("Fireball Ability should be UFCGA_Fireball"), FireballAsset->GameplayData.CardAbilityClass == UFCGA_Fireball::StaticClass());
				TestEqual(TEXT("Fireball Card Name should match"), FireballAsset->DisplayData.CardName.ToString(), FString(TEXT("파이어 볼")));

				// Class & Multi-Element Affinities (Mage: Fire + Earth)
				TestEqual(TEXT("Fireball RequiredClass must be Mage"), (uint8)FireballAsset->GameplayData.RequiredClass, (uint8)EFCCharacterClass::Mage);
				TestTrue(TEXT("Fireball must be able to hold elements"), FireballAsset->GameplayData.CanHaveElements());
				TestTrue(TEXT("Fireball must have Fire element"), FireballAsset->GameplayData.HasElement(EFCElement::Fire));
				TestTrue(TEXT("Fireball must have Earth element"), FireballAsset->GameplayData.HasElement(EFCElement::Earth));
				TestFalse(TEXT("Fireball must not be Neutral"), FireballAsset->GameplayData.IsNeutral());
				TestEqual(TEXT("Fireball trait text should format properly"), FireballAsset->GameplayData.GetFormattedTraitText().ToString(), FString(TEXT("화염 / 대지")));
			}
		}
	}

	// Test 2: Verify Fireball Projectile Class Defaults
	AFCFireballProjectile* FireballCDO = GetMutableDefault<AFCFireballProjectile>();
	TestNotNull(TEXT("AFCFireballProjectile CDO should exist"), FireballCDO);
	if (FireballCDO)
	{
		TestEqual(TEXT("Fireball projectile damage must be 1.0"), FireballCDO->GetDamage(), 1.0f);
		TestTrue(TEXT("Fireball must replicate"), FireballCDO->GetIsReplicated());
	}

	return true;
}

#endif
