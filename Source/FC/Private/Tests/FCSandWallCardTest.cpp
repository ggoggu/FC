#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Combat/Wall/FCSandWall.h"
#include "Combat/Projectile/FCFireballProjectile.h"
#include "AbilitySystem/Abilities/FCGA_SandWall.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardTypes.h"
#include "Data/Class/FCClassTypes.h"
#include "Character/Player/FCPlayerCharacter.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCSandWallCardTest, "FC.Combat.SandWallCard", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCSandWallCardTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Verify Card Catalog Definition for Card_SandWall (센드 워)
	// =========================================================================
	UGameInstance* DummyGameInstance = NewObject<UGameInstance>();
	TestNotNull(TEXT("GameInstance should be instantiable"), DummyGameInstance);

	if (DummyGameInstance)
	{
		UFCCardSubsystem* Subsystem = NewObject<UFCCardSubsystem>(DummyGameInstance);
		TestNotNull(TEXT("Card Subsystem should be instantiable"), Subsystem);

		if (Subsystem)
		{
			UFCCardDataAsset* SandWallAsset = Subsystem->GetCardDataAsset(FName("Card_SandWall"));
			TestNotNull(TEXT("Card_SandWall should resolve from catalog"), SandWallAsset);

			if (SandWallAsset)
			{
				TestEqual(TEXT("SandWall Card Type must be Skill"), (uint8)SandWallAsset->GameplayData.CardType, (uint8)EFCCardType::Skill);
				TestEqual(TEXT("SandWall Target Type must be DirectionalAoE"), (uint8)SandWallAsset->GameplayData.TargetType, (uint8)EFCCardTargetType::DirectionalAoE);
				TestEqual(TEXT("SandWall Mana Cost should be 1"), SandWallAsset->GameplayData.BaseManaCost, 1);
				TestEqual(TEXT("SandWall Base Value should be 1.0 (1초 지속시간)"), SandWallAsset->GameplayData.BaseValue, 1.0f);
				TestEqual(TEXT("SandWall Ability should be UFCGA_SandWall"), SandWallAsset->GameplayData.GetCardAbilityClass(), TSubclassOf<UGameplayAbility>(UFCGA_SandWall::StaticClass()));
				TestEqual(TEXT("SandWall Card Name should match '센드 워'"), SandWallAsset->DisplayData.CardName.ToString(), FString(TEXT("센드 워")));

				// Class & Earth element affinity
				TestEqual(TEXT("SandWall RequiredClass must be Mage"), (uint8)SandWallAsset->GameplayData.RequiredClass, (uint8)EFCCharacterClass::Mage);
				TestTrue(TEXT("SandWall must have Earth element affinity"), SandWallAsset->GameplayData.HasElement(EFCElement::Earth));
				TestTrue(TEXT("SandWall must be usable by Mage"), FCClassTraitUtils::CanCardBeUsedByClass(SandWallAsset->GameplayData.RequiredClass, EFCCharacterClass::Mage));
			}

			// Verify Korean alias Card_센드워 also resolves properly
			UFCCardDataAsset* SandWallAlias = Subsystem->GetCardDataAsset(FName("Card_센드워"));
			TestNotNull(TEXT("Card_센드워 alias should resolve from catalog"), SandWallAlias);
		}
	}

	// =========================================================================
	// Test 2: Verify AFCSandWall Actor Defaults, Lifetime, and Collision
	// =========================================================================
	AFCSandWall* SandWallCDO = GetMutableDefault<AFCSandWall>();
	TestNotNull(TEXT("AFCSandWall CDO should exist"), SandWallCDO);

	if (SandWallCDO)
	{
		TestTrue(TEXT("SandWall actor must be replicated"), SandWallCDO->GetIsReplicated());
		TestEqual(TEXT("SandWall default lifetime must be 1.0s"), SandWallCDO->InitialLifeSpan, 1.0f);

		UBoxComponent* BoxComp = SandWallCDO->GetCollisionComponent();
		TestNotNull(TEXT("SandWall CollisionComponent must exist"), BoxComp);

		if (BoxComp)
		{
			TestEqual(TEXT("Collision object type must be WorldDynamic"), (uint8)BoxComp->GetCollisionObjectType(), (uint8)ECC_WorldDynamic);
			TestEqual(TEXT("Must block WorldDynamic (projectiles)"), (uint8)BoxComp->GetCollisionResponseToChannel(ECC_WorldDynamic), (uint8)ECR_Block);
			TestEqual(TEXT("Must ignore Pawn to avoid clipping caster"), (uint8)BoxComp->GetCollisionResponseToChannel(ECC_Pawn), (uint8)ECR_Ignore);
			TestNearlyEqual(TEXT("Box Extent X should be 15cm (30cm thick)"), (float)BoxComp->GetUnscaledBoxExtent().X, 15.0f, 0.1f);
			TestNearlyEqual(TEXT("Box Extent Y should be 120cm (240cm wide)"), (float)BoxComp->GetUnscaledBoxExtent().Y, 120.0f, 0.1f);
			TestNearlyEqual(TEXT("Box Extent Z should be 90cm (180cm high)"), (float)BoxComp->GetUnscaledBoxExtent().Z, 90.0f, 0.1f);
		}
	}

	// =========================================================================
	// Test 3: Verify Sand Wall Attack Absorption Logic
	// =========================================================================
	{
		AFCSandWall* TestWall = NewObject<AFCSandWall>();
		TestNotNull(TEXT("TestWall must be instantiable"), TestWall);

		AFCPlayerCharacter* CasterPlayer = NewObject<AFCPlayerCharacter>();
		TestNotNull(TEXT("CasterPlayer must be instantiable"), CasterPlayer);

		AFCMobCharacter* EnemyMob = NewObject<AFCMobCharacter>();
		TestNotNull(TEXT("EnemyMob must be instantiable"), EnemyMob);

		AFCFireballProjectile* HostileProjectile = NewObject<AFCFireballProjectile>();
		TestNotNull(TEXT("HostileProjectile must be instantiable"), HostileProjectile);

		AFCFireballProjectile* FriendlyProjectile = NewObject<AFCFireballProjectile>();
		TestNotNull(TEXT("FriendlyProjectile must be instantiable"), FriendlyProjectile);

		if (TestWall && CasterPlayer && EnemyMob && HostileProjectile && FriendlyProjectile)
		{
			TestWall->SetInstigator(CasterPlayer);
			HostileProjectile->SetInstigator(EnemyMob);
			FriendlyProjectile->SetInstigator(CasterPlayer);

			// 1. Friendly projectile from caster must NOT be absorbed (passes through)
			TestFalse(TEXT("Friendly projectile must not be absorbed"), TestWall->CanAbsorbAttackFrom(FriendlyProjectile));

			// 2. Hostile projectile from enemy must be absorbable
			TestTrue(TEXT("Hostile projectile must be absorbable"), TestWall->CanAbsorbAttackFrom(HostileProjectile));

			// 3. Wall cannot absorb from itself or caster pawn directly
			TestFalse(TEXT("Wall cannot absorb from itself"), TestWall->CanAbsorbAttackFrom(TestWall));
			TestFalse(TEXT("Wall cannot absorb from caster pawn"), TestWall->CanAbsorbAttackFrom(CasterPlayer));

			// 4. Absorb hostile projectile attack
			TestEqual(TEXT("Initial absorbed attack count must be 0"), TestWall->GetAbsorbedAttackCount(), 0);
			TestWall->AbsorbIncomingAttack(HostileProjectile, FVector(100.0f, 0.0f, 50.0f));
			TestEqual(TEXT("Absorbed count should be 1 after absorbing hostile projectile"), TestWall->GetAbsorbedAttackCount(), 1);

			// 5. Repeated absorption of same projectile should be ignored (idempotent)
			TestWall->AbsorbIncomingAttack(HostileProjectile, FVector(100.0f, 0.0f, 50.0f));
			TestEqual(TEXT("Absorbed count should remain 1 on duplicate impact"), TestWall->GetAbsorbedAttackCount(), 1);
		}
	}

	// =========================================================================
	// Test 4: Verify UFCGA_SandWall Configuration
	// =========================================================================
	UFCGA_SandWall* SandWallGA = NewObject<UFCGA_SandWall>();
	TestNotNull(TEXT("UFCGA_SandWall must be instantiable"), SandWallGA);

	if (SandWallGA)
	{
		TestEqual(TEXT("SpawnForwardDistance should default to 150.0cm"), SandWallGA->GetSpawnForwardDistance(), 150.0f);
		TestEqual(TEXT("WallDuration should default to 1.0s"), SandWallGA->GetWallDuration(), 1.0f);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
