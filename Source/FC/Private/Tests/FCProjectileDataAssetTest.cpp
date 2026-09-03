#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AbilitySystem/Abilities/FCGA_SpawnProjectile.h"
#include "Combat/Projectile/FCProjectileBase.h"
#include "Combat/Projectile/FCProjectileDataAsset.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardTypes.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCProjectileDataAssetTest, "FC.Combat.ProjectileDataAsset", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCProjectileDataAssetTest::RunTest(const FString& Parameters)
{
	// Test 1: Verify Fire Arrow Card & Projectile Data Asset Catalog Resolution & Data Integrity
	UGameInstance* DummyGameInstance = NewObject<UGameInstance>();
	TestNotNull(TEXT("GameInstance should be instantiable"), DummyGameInstance);

	if (DummyGameInstance)
	{
		UFCCardSubsystem* Subsystem = NewObject<UFCCardSubsystem>(DummyGameInstance);
		TestNotNull(TEXT("Card Subsystem should be instantiable"), Subsystem);

		if (Subsystem)
		{
			// Create and configure Projectile Data Asset
			UFCProjectileDataAsset* FireArrowProjData = NewObject<UFCProjectileDataAsset>(Subsystem);
			FireArrowProjData->ProjectileClass = AFCProjectileBase::StaticClass();
			FireArrowProjData->Damage = 1.0f;
			FireArrowProjData->LaunchSpeed = 3000.0f;
			FireArrowProjData->MaxSpeed = 3000.0f;
			FireArrowProjData->GravityScale = 0.0f;
			FireArrowProjData->ProjectileElements = { EFCElement::Fire };
			FireArrowProjData->SourceClass = EFCCharacterClass::Mage;
			FireArrowProjData->SourceCardType = EFCCardType::Attack;
			FireArrowProjData->LifeSpan = 5.0f;

			// Create and configure Card Data Asset
			UFCCardDataAsset* FireArrowCard = NewObject<UFCCardDataAsset>(Subsystem);
			FireArrowCard->GameplayData.CardId = FName("Card_FireArrow");
			FireArrowCard->GameplayData.BaseManaCost = 1;
			FireArrowCard->GameplayData.CardType = EFCCardType::Attack;
			FireArrowCard->GameplayData.TargetType = EFCCardTargetType::DirectionalAoE;
			FireArrowCard->GameplayData.BaseValue = 1.0f;
			FireArrowCard->GameplayData.RequiredClass = EFCCharacterClass::Mage;
			FireArrowCard->GameplayData.Elements = { EFCElement::Fire };
			FireArrowCard->GameplayData.CardAbilityClass = UFCGA_SpawnProjectile::StaticClass();
			FireArrowCard->GameplayData.ProjectileDataAsset = FireArrowProjData;
			FireArrowCard->DisplayData.CardName = FText::FromString(TEXT("파이어 에로우"));
			FireArrowCard->DisplayData.CardDescription = FText::FromString(TEXT("불 화살을 발사한다."));
			FireArrowCard->DisplayData.Rarity = EFCCardRarity::Common;

			Subsystem->RegisterCardDataAsset(FireArrowCard);

			UFCCardDataAsset* ResolvedCard = Subsystem->GetCardDataAsset(FName("Card_FireArrow"));
			TestNotNull(TEXT("Card_FireArrow should resolve from catalog"), ResolvedCard);

			if (ResolvedCard)
			{
				TestEqual(TEXT("Fire Arrow Mana Cost should be 1"), ResolvedCard->GameplayData.BaseManaCost, 1);
				TestEqual(TEXT("Fire Arrow Base Damage should be 1.0"), ResolvedCard->GameplayData.BaseValue, 1.0f);
				TestEqual(TEXT("Fire Arrow Card Type should be Attack"), (uint8)ResolvedCard->GameplayData.CardType, (uint8)EFCCardType::Attack);
				TestEqual(TEXT("Fire Arrow RequiredClass must be Mage"), (uint8)ResolvedCard->GameplayData.RequiredClass, (uint8)EFCCharacterClass::Mage);
				TestTrue(TEXT("Fire Arrow must have Fire element"), ResolvedCard->GameplayData.HasElement(EFCElement::Fire));
				TestEqual(TEXT("Fire Arrow Card Name should match"), ResolvedCard->DisplayData.CardName.ToString(), FString(TEXT("파이어 에로우")));
				TestEqual(TEXT("Fire Arrow Card Description should match"), ResolvedCard->DisplayData.CardDescription.ToString(), FString(TEXT("불 화살을 발사한다.")));
				TestEqual(TEXT("Fire Arrow trait text should format properly"), ResolvedCard->GameplayData.GetFormattedTraitText().ToString(), FString(TEXT("화염")));

				// Projectile Data Asset Link
				TestNotNull(TEXT("Fire Arrow must have valid ProjectileDataAsset"), ResolvedCard->GameplayData.ProjectileDataAsset.Get());
				if (ResolvedCard->GameplayData.ProjectileDataAsset)
				{
					TestEqual(TEXT("Projectile Launch Speed should be 3000.0"), ResolvedCard->GameplayData.ProjectileDataAsset->LaunchSpeed, 3000.0f);
					TestEqual(TEXT("Projectile Damage should be 1.0"), ResolvedCard->GameplayData.ProjectileDataAsset->Damage, 1.0f);
					TestTrue(TEXT("Projectile Elements should contain Fire"), ResolvedCard->GameplayData.ProjectileDataAsset->ProjectileElements.Contains(EFCElement::Fire));
					TestTrue(TEXT("Projectile Template Class should be valid"), ResolvedCard->GameplayData.ProjectileDataAsset->ProjectileClass != nullptr);
				}
			}
		}
	}

	// Test 2: Verify Data-Driven Initialization on AFCProjectileBase
	UFCProjectileDataAsset* TestProjData = NewObject<UFCProjectileDataAsset>();
	TestProjData->Damage = 2.5f;
	TestProjData->LaunchSpeed = 3200.0f;
	TestProjData->MaxSpeed = 3200.0f;
	TestProjData->GravityScale = 0.0f;
	TestProjData->ProjectileElements = { EFCElement::Fire };
	TestProjData->SourceClass = EFCCharacterClass::Mage;
	TestProjData->SourceCardType = EFCCardType::Attack;
	TestProjData->LifeSpan = 4.0f;

	AFCProjectileBase* TestProj = NewObject<AFCProjectileBase>();
	TestNotNull(TEXT("AFCProjectileBase should instantiate"), TestProj);

	if (TestProj)
	{
		// Set native BP collider setting to 14.0
		if (TestProj->GetCollisionComponent())
		{
			TestProj->GetCollisionComponent()->InitSphereRadius(14.0f);
		}

		TestProj->InitializeFromDataAsset(TestProjData);

		TestEqual(TEXT("Injected projectile damage must be 2.5"), TestProj->GetDamage(), 2.5f);
		TestTrue(TEXT("Injected projectile must replicate"), TestProj->GetIsReplicated());
		TestEqual(TEXT("Injected projectile source class must be Mage"), (uint8)TestProj->GetSourceClass(), (uint8)EFCCharacterClass::Mage);
		TestEqual(TEXT("Injected projectile card type must be Attack"), (uint8)TestProj->GetSourceCardType(), (uint8)EFCCardType::Attack);
		TestTrue(TEXT("Injected projectile element must be Fire"), TestProj->GetProjectileElements().Contains(EFCElement::Fire));

		// Verify that BP component's native collider radius is respected and preserved
		if (TestProj->GetCollisionComponent())
		{
			TestEqual(TEXT("Native BP collision radius must be preserved"), TestProj->GetCollisionComponent()->GetUnscaledSphereRadius(), 14.0f);
		}

		if (TestProj->GetProjectileMovement())
		{
			TestEqual(TEXT("Injected projectile speed must be 3200.0"), TestProj->GetProjectileMovement()->InitialSpeed, 3200.0f);
		}
	}

	// Test 3: Verify UFCGA_SpawnProjectile Ability Class with Data Asset Support
	UFCGA_SpawnProjectile* SpawnGA = NewObject<UFCGA_SpawnProjectile>();
	TestNotNull(TEXT("UFCGA_SpawnProjectile should instantiate"), SpawnGA);

	if (SpawnGA)
	{
		SpawnGA->SetProjectileDataAsset(TestProjData);
		TestEqual(TEXT("UFCGA_SpawnProjectile must store ProjectileDataAsset"), SpawnGA->GetProjectileDataAsset(), (const UFCProjectileDataAsset*)TestProjData);
	}

	return true;
}

#endif
