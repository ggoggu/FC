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
			UFCCardDataAsset* FireballAsset = Subsystem->GetCardDataAsset(FName("Card_Fireball"));
			TestNotNull(TEXT("Card_Fireball should resolve from catalog"), FireballAsset);

			if (FireballAsset)
			{
				TestEqual(TEXT("Fireball Mana Cost should be 1"), FireballAsset->GameplayData.BaseManaCost, 1);
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
