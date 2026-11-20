#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AbilitySystem/Abilities/FCGA_SpawnProjectile.h"
#include "Combat/Projectile/FCProjectileBase.h"
#include "Combat/Projectile/FCProjectileDataAsset.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardTypes.h"
#include "Data/FCGameplayTags.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCFireballCardTest, "FC.Combat.FireballCard", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCFireballCardTest::RunTest(const FString& Parameters)
{
	UGameInstance* DummyGameInstance = NewObject<UGameInstance>();
	TestNotNull(TEXT("GameInstance should be instantiable"), DummyGameInstance);

	if (DummyGameInstance)
	{
		UFCCardSubsystem* Subsystem = NewObject<UFCCardSubsystem>(DummyGameInstance);
		TestNotNull(TEXT("Card Subsystem should be instantiable"), Subsystem);

		if (Subsystem)
		{
			UFCProjectileDataAsset* FireballProjData = NewObject<UFCProjectileDataAsset>(Subsystem);
			FireballProjData->Damage = 1.0f;
			FireballProjData->LaunchSpeed = 2500.0f;
			FireballProjData->ProjectileClass = AFCProjectileBase::StaticClass();
			FireballProjData->ProjectileElementTags = { FCGameplayTags::Element_Fire, FCGameplayTags::Element_Earth };

			UFCCardDataAsset* NewFireball = NewObject<UFCCardDataAsset>(Subsystem);
			NewFireball->GameplayData.CardId = FName("Card_Fireball");
			NewFireball->GameplayData.BaseManaCost = 2;
			NewFireball->GameplayData.CardType = EFCCardType::Attack;
			NewFireball->GameplayData.TargetType = EFCCardTargetType::DirectionalAoE;
			NewFireball->GameplayData.bSpawnsProjectile = true;
			NewFireball->GameplayData.ProjectileDataAsset = FireballProjData;
			NewFireball->GameplayData.BaseValue = 1.0f;
			NewFireball->GameplayData.CardAbilityClass = UFCGA_SpawnProjectile::StaticClass();
			NewFireball->GameplayData.RequiredClassTag = FCGameplayTags::Class_Mage;
			NewFireball->GameplayData.ElementTags = { FCGameplayTags::Element_Fire.GetTag(), FCGameplayTags::Element_Earth.GetTag() };
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
				TestTrue(TEXT("Fireball Ability should be UFCGA_SpawnProjectile"), FireballAsset->GameplayData.GetCardAbilityClass() == UFCGA_SpawnProjectile::StaticClass());
				TestEqual(TEXT("Fireball Card Name should match"), FireballAsset->DisplayData.CardName.ToString(), FString(TEXT("파이어 볼")));

				TestTrue(TEXT("Fireball RequiredClassTag must be Mage"), FireballAsset->GameplayData.RequiredClassTag == FCGameplayTags::Class_Mage);
				TestTrue(TEXT("Fireball must be able to hold elements"), FireballAsset->GameplayData.CanHaveElements());
				TestTrue(TEXT("Fireball must have Fire element tag"), FireballAsset->GameplayData.HasElementTag(FCGameplayTags::Element_Fire));
				TestTrue(TEXT("Fireball must have Earth element tag"), FireballAsset->GameplayData.HasElementTag(FCGameplayTags::Element_Earth));
				TestFalse(TEXT("Fireball must not be Neutral"), FireballAsset->GameplayData.IsNeutral());
				TestEqual(TEXT("Fireball trait text should format properly"), FireballAsset->GameplayData.GetFormattedTraitText().ToString(), FString(TEXT("화염 / 대지")));
			}
		}
	}

	// Verify data-driven AFCProjectileBase defaults
	AFCProjectileBase* ProjCDO = GetMutableDefault<AFCProjectileBase>();
	TestNotNull(TEXT("AFCProjectileBase CDO should exist"), ProjCDO);
	if (ProjCDO)
	{
		TestEqual(TEXT("Base projectile damage must default to 1.0"), ProjCDO->GetDamage(), 1.0f);
		TestTrue(TEXT("Projectile must replicate"), ProjCDO->GetIsReplicated());
	}

	UFCGA_SpawnProjectile* SpawnProjGA = NewObject<UFCGA_SpawnProjectile>();
	TestNotNull(TEXT("UFCGA_SpawnProjectile should be instantiable"), SpawnProjGA);

	return true;
}

#endif
