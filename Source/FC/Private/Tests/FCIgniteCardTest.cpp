#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AbilitySystem/Abilities/FCGA_Ignite.h"
#include "Combat/Element/FCElementComponent.h"
#include "Combat/FCCombatUtils.h"
#include "Character/Mob/FCMobCharacter.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardTypes.h"
#include "Data/Class/FCClassTypes.h"
#include "Data/FCGameplayTags.h"
#include "Combat/Projectile/FCProjectileDataAsset.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCIgniteCardTest, "FC.Combat.IgniteCard", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCIgniteCardTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Verify Ignite Card Data Asset & Catalog Resolution
	// =========================================================================
	UGameInstance* DummyGI = NewObject<UGameInstance>();
	TestNotNull(TEXT("DummyGI should be instantiable"), DummyGI);

	if (DummyGI)
	{
		UFCCardSubsystem* Subsystem = NewObject<UFCCardSubsystem>(DummyGI);
		TestNotNull(TEXT("Card Subsystem should be instantiable"), Subsystem);

		if (Subsystem)
		{
			UFCCardDataAsset* IgniteAsset = Subsystem->GetCardDataAsset(FName("Card_Ignite"));
			TestNotNull(TEXT("Card_Ignite should resolve from catalog or uasset"), IgniteAsset);

			if (IgniteAsset)
			{
				TestEqual(TEXT("Ignite Card Name must be '점화'"), IgniteAsset->DisplayData.CardName.ToString(), FString(TEXT("점화")));
				TestEqual(TEXT("Ignite Base Mana Cost must be 1"), IgniteAsset->GameplayData.BaseManaCost, 1);
				TestTrue(TEXT("Ignite RequiredClassTag must be Mage"), IgniteAsset->GameplayData.RequiredClassTag == FCGameplayTags::Class_Mage);
				TestEqual(TEXT("Ignite TargetType must be AllEnemies"), (uint8)IgniteAsset->GameplayData.TargetType, (uint8)EFCCardTargetType::AllEnemies);
				TestEqual(TEXT("Ignite CardType must be Attack"), (uint8)IgniteAsset->GameplayData.CardType, (uint8)EFCCardType::Attack);
				TestEqual(TEXT("Ignite Base Value (Damage per stack) must be 10.0"), IgniteAsset->GameplayData.BaseValue, 10.0f);
				TestTrue(TEXT("Ignite CardAbilityClass must be UFCGA_Ignite"), IgniteAsset->GameplayData.GetCardAbilityClass() == UFCGA_Ignite::StaticClass());

				TestTrue(TEXT("Ignite should contain Element.None tag"), IgniteAsset->GameplayData.ElementTags.Contains(FCGameplayTags::Element_None.GetTag()));
				TestEqual(TEXT("Ignite trait text should format as '무속성'"), IgniteAsset->GameplayData.GetFormattedTraitText().ToString(), FString(TEXT("무속성")));

				TestFalse(TEXT("Ignite must not spawn projectile"), IgniteAsset->GameplayData.bSpawnsProjectile);
				TestNull(TEXT("Ignite ProjectileDataAsset must be null"), IgniteAsset->GameplayData.GetProjectileDataAsset());
				TestFalse(TEXT("Ignite SpawnsProjectile() must return false"), IgniteAsset->GameplayData.SpawnsProjectile());

				TestTrue(TEXT("Ignite must be usable by Mage"), FCClassTraitUtils::CanCardBeUsedByClass(IgniteAsset->GameplayData.RequiredClassTag, FCGameplayTags::Class_Mage));
				TestFalse(TEXT("Ignite must not be usable by Warrior"), FCClassTraitUtils::CanCardBeUsedByClass(IgniteAsset->GameplayData.RequiredClassTag, FCGameplayTags::Class_Warrior));
				TestFalse(TEXT("Ignite must not be usable by Rogue"), FCClassTraitUtils::CanCardBeUsedByClass(IgniteAsset->GameplayData.RequiredClassTag, FCGameplayTags::Class_Rogue));
			}

			// Verify Korean alias "Card_점화" also resolves via DataTable LegacyAliases
			UFCCardDataAsset* KoreanAliasAsset = Subsystem->GetCardDataAsset(FName(TEXT("Card_점화")));
			TestNotNull(TEXT("Card_점화 alias should also resolve"), KoreanAliasAsset);
		}
	}

	// =========================================================================
	// Test 2: Verify Damage Scaling per Fire Stack (10 per stack)
	// =========================================================================
	AFCMobCharacter* Target = NewObject<AFCMobCharacter>();
	TestNotNull(TEXT("Target mob character should be instantiable"), Target);

	if (Target)
	{
		UFCElementComponent* ElementComp = Target->GetElementComponent();
		TestNotNull(TEXT("Target must have UFCElementComponent"), ElementComp);

		UFCAttributeSet* AttrSet = Target->GetAttributeSet();
		TestNotNull(TEXT("Target must have UFCAttributeSet"), AttrSet);

		if (ElementComp && AttrSet)
		{
			AttrSet->InitMaxHealth(100.0f);
			AttrSet->InitHealth(100.0f);
			AttrSet->InitMaxShield(100.0f);
			AttrSet->InitShield(0.0f);
			ElementComp->ClearAllElementStacks();

			float DamageA = UFCGA_Ignite::ApplyIgniteToTarget(nullptr, Target, 10.0f, false);
			TestEqual(TEXT("Ignite on 0 Fire stacks should deal 0 damage"), DamageA, 0.0f);
			TestEqual(TEXT("Target health should remain 100.0"), AttrSet->GetHealth(), 100.0f);

			ElementComp->AddElementStack(FCGameplayTags::Element_Fire, 1);
			TestEqual(TEXT("Target should have 1 Fire stack"), ElementComp->GetElementCount(FCGameplayTags::Element_Fire), 1);

			float DamageB = UFCGA_Ignite::ApplyIgniteToTarget(nullptr, Target, 10.0f, false);
			TestEqual(TEXT("Ignite on 1 Fire stack should deal 10 damage"), DamageB, 10.0f);
			TestEqual(TEXT("Target health should reduce to 90.0"), AttrSet->GetHealth(), 90.0f);
			TestEqual(TEXT("bConsumeFireStacks=false should preserve Fire stack"), ElementComp->GetElementCount(FCGameplayTags::Element_Fire), 1);

			ElementComp->ClearAllElementStacks();
			ElementComp->AddElementStack(FCGameplayTags::Element_Fire, 3);
			AttrSet->InitHealth(100.0f);

			float DamageC = UFCGA_Ignite::ApplyIgniteToTarget(nullptr, Target, 10.0f, false);
			TestEqual(TEXT("Ignite on 3 Fire stacks should deal 30 damage"), DamageC, 30.0f);
			TestEqual(TEXT("Target health should reduce to 70.0"), AttrSet->GetHealth(), 70.0f);

			AttrSet->InitHealth(100.0f);
			AttrSet->InitShield(20.0f);

			float DamageD = UFCGA_Ignite::ApplyIgniteToTarget(nullptr, Target, 10.0f, false);
			TestEqual(TEXT("Ignite damage calculation with shield remains 30.0"), DamageD, 30.0f);
			TestEqual(TEXT("Shield should absorb 20 and deplete to 0.0"), AttrSet->GetShield(), 0.0f);
			TestEqual(TEXT("Health should absorb remaining 10 and become 90.0"), AttrSet->GetHealth(), 90.0f);

			ElementComp->ClearAllElementStacks();
			ElementComp->AddElementStack(FCGameplayTags::Element_Fire, 2);
			TestEqual(TEXT("Pre-consume Fire stacks should be 2"), ElementComp->GetElementCount(FCGameplayTags::Element_Fire), 2);

			float DamageE = UFCGA_Ignite::ApplyIgniteToTarget(nullptr, Target, 10.0f, true);
			TestEqual(TEXT("Ignite damage for 2 stacks should be 20.0"), DamageE, 20.0f);
			TestEqual(TEXT("After bConsumeFireStacks=true, Fire stacks should be 0"), ElementComp->GetElementCount(FCGameplayTags::Element_Fire), 0);
		}
	}

	// =========================================================================
	// Test 3: Multi-Target Filtering (Enemy vs Player)
	// =========================================================================
	AActor* PlayerActor = NewObject<AActor>();
	PlayerActor->Tags.Add(FName("Player"));

	AFCMobCharacter* MobActor = NewObject<AFCMobCharacter>();

	TestFalse(TEXT("Actor with Player tag must NOT be considered an enemy"), UFCGA_Ignite::IsEnemyActor(nullptr, PlayerActor));
	TestTrue(TEXT("AFCMobCharacter must be considered an enemy"), UFCGA_Ignite::IsEnemyActor(nullptr, MobActor));
	TestFalse(TEXT("Self/Source must NOT be considered an enemy"), UFCGA_Ignite::IsEnemyActor(MobActor, MobActor));

	return true;
}

#endif
