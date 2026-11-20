#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AbilitySystem/Abilities/FCGA_SpawnProjectile.h"
#include "Combat/Projectile/FCProjectileBase.h"
#include "Combat/Projectile/FCProjectileDataAsset.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardTypes.h"
#include "Data/FCGameplayTags.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCProjectileDataAssetTest, "FC.Combat.ProjectileDataAsset", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCProjectileDataAssetTest::RunTest(const FString& Parameters)
{
	UGameInstance* DummyGameInstance = NewObject<UGameInstance>();
	TestNotNull(TEXT("GameInstance should be instantiable"), DummyGameInstance);

	if (DummyGameInstance)
	{
		UFCCardSubsystem* Subsystem = NewObject<UFCCardSubsystem>(DummyGameInstance);
		TestNotNull(TEXT("Card Subsystem should be instantiable"), Subsystem);

		if (Subsystem)
		{
			UFCProjectileDataAsset* FireArrowProjData = NewObject<UFCProjectileDataAsset>(Subsystem);
			FireArrowProjData->ProjectileClass = AFCProjectileBase::StaticClass();
			FireArrowProjData->Damage = 1.0f;
			FireArrowProjData->LaunchSpeed = 3000.0f;
			FireArrowProjData->MaxSpeed = 3000.0f;
			FireArrowProjData->GravityScale = 0.0f;
			FireArrowProjData->ProjectileElementTags = { FCGameplayTags::Element_Fire.GetTag() };
			FireArrowProjData->SourceClassTag = FCGameplayTags::Class_Mage;
			FireArrowProjData->SourceCardTypeTag = FCGameplayTags::Card_Type_Attack;
			FireArrowProjData->LifeSpan = 5.0f;

			UFCCardDataAsset* FireArrowCard = NewObject<UFCCardDataAsset>(Subsystem);
			FireArrowCard->GameplayData.CardId = FName("Card_FireArrow");
			FireArrowCard->GameplayData.BaseManaCost = 1;
			FireArrowCard->GameplayData.CardType = EFCCardType::Attack;
			FireArrowCard->GameplayData.TargetType = EFCCardTargetType::DirectionalAoE;
			FireArrowCard->GameplayData.BaseValue = 1.0f;
			FireArrowCard->GameplayData.RequiredClassTag = FCGameplayTags::Class_Mage;
			FireArrowCard->GameplayData.ElementTags = { FCGameplayTags::Element_Fire.GetTag() };
			FireArrowCard->GameplayData.CardAbilityClass = UFCGA_SpawnProjectile::StaticClass();
			FireArrowCard->GameplayData.bSpawnsProjectile = true;
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
				TestTrue(TEXT("Fire Arrow RequiredClassTag must be Mage"), ResolvedCard->GameplayData.RequiredClassTag == FCGameplayTags::Class_Mage);
				TestTrue(TEXT("Fire Arrow must have Fire element tag"), ResolvedCard->GameplayData.HasElementTag(FCGameplayTags::Element_Fire));
				TestEqual(TEXT("Fire Arrow Card Name should match"), ResolvedCard->DisplayData.CardName.ToString(), FString(TEXT("파이어 에로우")));
				TestEqual(TEXT("Fire Arrow Card Description should match"), ResolvedCard->DisplayData.CardDescription.ToString(), FString(TEXT("불 화살을 발사한다.")));
				TestEqual(TEXT("Fire Arrow trait text should format properly"), ResolvedCard->GameplayData.GetFormattedTraitText().ToString(), FString(TEXT("화염")));

				TestTrue(TEXT("Fire Arrow bSpawnsProjectile must be true"), ResolvedCard->GameplayData.bSpawnsProjectile);
				TestTrue(TEXT("Fire Arrow must spawn projectile"), ResolvedCard->GameplayData.SpawnsProjectile());
				TestNotNull(TEXT("Fire Arrow must have valid ProjectileDataAsset"), ResolvedCard->GameplayData.GetProjectileDataAsset());
				if (const UFCProjectileDataAsset* ProjData = ResolvedCard->GameplayData.GetProjectileDataAsset())
				{
					TestEqual(TEXT("Projectile Launch Speed should be 3000.0"), ProjData->LaunchSpeed, 3000.0f);
					TestEqual(TEXT("Projectile Damage should be 1.0"), ProjData->Damage, 1.0f);
					TestTrue(TEXT("Projectile ElementTags should contain Fire"), ProjData->ProjectileElementTags.Contains(FCGameplayTags::Element_Fire.GetTag()));
					TestTrue(TEXT("Projectile Template Class should be valid"), ProjData->ProjectileClass != nullptr);
				}
			}
		}
	}

	UFCProjectileDataAsset* TestProjData = NewObject<UFCProjectileDataAsset>();
	TestProjData->Damage = 2.5f;
	TestProjData->LaunchSpeed = 3200.0f;
	TestProjData->MaxSpeed = 3200.0f;
	TestProjData->GravityScale = 0.0f;
	TestProjData->ProjectileElementTags = { FCGameplayTags::Element_Fire.GetTag() };
	TestProjData->SourceClassTag = FCGameplayTags::Class_Mage;
	TestProjData->SourceCardTypeTag = FCGameplayTags::Card_Type_Attack;
	TestProjData->LifeSpan = 4.0f;

	AFCProjectileBase* TestProj = NewObject<AFCProjectileBase>();
	TestNotNull(TEXT("AFCProjectileBase should instantiate"), TestProj);

	if (TestProj)
	{
		if (TestProj->GetCollisionComponent())
		{
			TestProj->GetCollisionComponent()->InitSphereRadius(14.0f);
		}

		TestProj->InitializeFromDataAsset(TestProjData);

		TestEqual(TEXT("Injected projectile damage must be 2.5"), TestProj->GetDamage(), 2.5f);
		TestTrue(TEXT("Injected projectile must replicate"), TestProj->GetIsReplicated());
		TestTrue(TEXT("Injected projectile source class must be Mage"), TestProj->GetSourceClassTag() == FCGameplayTags::Class_Mage);
		TestTrue(TEXT("Injected projectile card type must be Attack"), TestProj->GetSourceCardTypeTag() == FCGameplayTags::Card_Type_Attack);
		TestTrue(TEXT("Injected projectile element must be Fire"), TestProj->GetProjectileElementTags().Contains(FCGameplayTags::Element_Fire.GetTag()));

		if (TestProj->GetCollisionComponent())
		{
			TestEqual(TEXT("Native BP collision radius must be preserved"), TestProj->GetCollisionComponent()->GetUnscaledSphereRadius(), 14.0f);
		}

		if (TestProj->GetProjectileMovement())
		{
			TestEqual(TEXT("Injected projectile speed must be 3200.0"), TestProj->GetProjectileMovement()->InitialSpeed, 3200.0f);
		}
	}

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
