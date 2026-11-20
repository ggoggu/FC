#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "AbilitySystem/Effects/FCGE_Damage.h"
#include "AbilitySystem/ExecCalc/FCExecCalc_Damage.h"
#include "AbilitySystem/Abilities/FCGA_Ignite.h"
#include "Character/Player/FCPlayerCharacter.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Gameplay/FCChestActor.h"
#include "Map/FCDestructibleWall.h"
#include "Combat/FCCombatUtils.h"
#include "Combat/Element/FCElementComponent.h"
#include "Combat/Projectile/FCProjectileBase.h"
#include "Combat/Projectile/Behaviors/FCProjectileBehaviors.h"
#include "Combat/Wall/FCSandWall.h"
#include "Data/FCGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCCombatPipelineTest, "FC.Combat.UnifiedPipeline", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCCombatPipelineTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: UFCGE_Damage Uses UFCExecCalc_Damage + IncomingDamage Meta-Attribute
	// =========================================================================
	{
		UFCGE_Damage* DamageCDO = GetMutableDefault<UFCGE_Damage>();
		TestNotNull(TEXT("UFCGE_Damage CDO must exist"), DamageCDO);
		if (DamageCDO)
		{
			TestEqual(TEXT("UFCGE_Damage must have 1 ExecutionCalculation"), DamageCDO->Executions.Num(), 1);
			TestEqual(TEXT("UFCGE_Damage must have 0 direct Health modifiers"), DamageCDO->Modifiers.Num(), 0);
			if (DamageCDO->Executions.Num() > 0)
			{
				TestTrue(TEXT("Execution class must be UFCExecCalc_Damage"),
					DamageCDO->Executions[0].CalculationClass == UFCExecCalc_Damage::StaticClass());
			}
		}

		AFCMobCharacter* TargetMob = NewObject<AFCMobCharacter>();
		UFCAttributeSet* AttrSet = TargetMob->GetAttributeSet();
		AttrSet->InitMaxHealth(100.0f);
		AttrSet->InitHealth(100.0f);
		AttrSet->InitMaxShield(50.0f);
		AttrSet->InitShield(25.0f);

		// Unified ApplyIncomingDamageValue absorbs 25 shield and deals 15 to health -> 85 HP
		AttrSet->ApplyIncomingDamageValue(40.0f, nullptr, nullptr);
		TestEqual(TEXT("Shield must deplete to 0 after 40 damage vs 25 shield"), AttrSet->GetShield(), 0.0f);
		TestEqual(TEXT("Health must reduce to 85 after 15 residual damage"), AttrSet->GetHealth(), 85.0f);

		// Fatal damage triggers Die() through unified pipeline
		AttrSet->ApplyIncomingDamageValue(90.0f, nullptr, nullptr);
		TestEqual(TEXT("Health must clamp to 0 on fatal damage"), AttrSet->GetHealth(), 0.0f);
		TestTrue(TEXT("TargetMob must be dead after fatal damage"), TargetMob->IsDead());
	}

	// =========================================================================
	// Test 2: Polymorphic IFCCombatTargetInterface (Zero Cast/IsA Chains)
	// =========================================================================
	{
		AFCPlayerCharacter* PlayerA = NewObject<AFCPlayerCharacter>();
		AFCPlayerCharacter* PlayerB = NewObject<AFCPlayerCharacter>();
		AFCMobCharacter* Mob = NewObject<AFCMobCharacter>();
		AFCChestActor* Chest = NewObject<AFCChestActor>();
		AFCDestructibleWall* Wall = NewObject<AFCDestructibleWall>();

		TestTrue(TEXT("Player team tag must be Combat.Team.Player"), PlayerA->GetCombatTeamTag() == FCGameplayTags::Combat_Team_Player);
		TestTrue(TEXT("Mob team tag must be Combat.Team.Enemy"), Mob->GetCombatTeamTag() == FCGameplayTags::Combat_Team_Enemy);
		TestTrue(TEXT("Chest team tag must be Combat.Target.Destructible"), Chest->GetCombatTeamTag() == FCGameplayTags::Combat_Target_Destructible);
		TestTrue(TEXT("Wall team tag must be Combat.Target.Destructible"), Wall->GetCombatTeamTag() == FCGameplayTags::Combat_Target_Destructible);

		TestFalse(TEXT("Player must not attack friendly player"), UFCCombatUtils::IsAttackableTarget(PlayerA, PlayerB));
		TestTrue(TEXT("Player must be able to attack alive Mob"), UFCCombatUtils::IsAttackableTarget(PlayerA, Mob));
		TestTrue(TEXT("Player must be able to attack unopened Chest"), UFCCombatUtils::IsAttackableTarget(PlayerA, Chest));
		TestTrue(TEXT("Player must be able to attack intact DestructibleWall"), UFCCombatUtils::IsAttackableTarget(PlayerA, Wall));

		Mob->Die();
		TestFalse(TEXT("Dead Mob must not be attackable"), UFCCombatUtils::IsAttackableTarget(PlayerA, Mob));
	}

	// =========================================================================
	// Test 3: IFCInterceptableAttack & Unblockable Tag Interaction on AFCSandWall
	// =========================================================================
	{
		AFCSandWall* SandWall = NewObject<AFCSandWall>();
		AFCPlayerCharacter* Player = NewObject<AFCPlayerCharacter>();
		AFCMobCharacter* Enemy = NewObject<AFCMobCharacter>();
		AFCProjectileBase* NormalEnemyProj = NewObject<AFCProjectileBase>();
		AFCProjectileBase* UnblockableEnemyProj = NewObject<AFCProjectileBase>();

		SandWall->SetInstigator(Player);
		NormalEnemyProj->SetInstigator(Enemy);
		UnblockableEnemyProj->SetInstigator(Enemy);

		FGameplayTagContainer UnblockableTags;
		UnblockableTags.AddTag(FCGameplayTags::Combat_Attack_Projectile);
		UnblockableTags.AddTag(FCGameplayTags::Combat_Attack_Unblockable);
		UnblockableEnemyProj->SetAttackTags(UnblockableTags);

		TestTrue(TEXT("Standard enemy projectile must be interceptable by SandWall"), SandWall->CanAbsorbAttackFrom(NormalEnemyProj));
		TestFalse(TEXT("Unblockable enemy projectile must bypass SandWall via IFCInterceptableAttack"), SandWall->CanAbsorbAttackFrom(UnblockableEnemyProj));
	}

	// =========================================================================
	// Test 4: Element-Agnostic Detonation (UFCGA_Ignite with Water/Lightning Tags)
	// =========================================================================
	{
		AFCMobCharacter* TargetMob = NewObject<AFCMobCharacter>();
		TargetMob->GetAttributeSet()->InitMaxHealth(100.0f);
		TargetMob->GetAttributeSet()->InitHealth(100.0f);
		TargetMob->GetElementComponent()->AddElementTagStack(FCGameplayTags::Element_Water, 2);

		const float WaterDetonateDmg = UFCGA_Ignite::ApplyIgniteToTarget(
			nullptr,
			TargetMob,
			15.0f,
			true,
			nullptr,
			FCGameplayTags::Element_Water);

		TestEqual(TEXT("Detonating 2 Water stacks at 15/stack must deal 30 damage"), WaterDetonateDmg, 30.0f);
		TestEqual(TEXT("TargetMob health must drop to 70"), TargetMob->GetAttributeSet()->GetHealth(), 70.0f);
		TestEqual(TEXT("Water stacks must be consumed to 0"), TargetMob->GetElementComponent()->GetElementTagCount(FCGameplayTags::Element_Water), 0);
	}

	// =========================================================================
	// Test 5: Composable TInstancedStruct Projectile Behaviors (Pierce & ElementTags)
	// =========================================================================
	{
		AFCProjectileBase* Proj = NewObject<AFCProjectileBase>();
		AFCMobCharacter* Mob = NewObject<AFCMobCharacter>();

		FFCProjectileBehavior_Pierce Pierce;
		Pierce.MaxPierceCount = 2;
		Proj->AddProjectileBehavior(TInstancedStruct<FFCProjectileBehaviorBase>::Make<FFCProjectileBehavior_Pierce>(Pierce));

		FFCProjectileBehavior_ApplyElementTags ElemBehavior;
		ElemBehavior.ElementTags = { FCGameplayTags::Element_Lightning };
		Proj->AddProjectileBehavior(TInstancedStruct<FFCProjectileBehaviorBase>::Make<FFCProjectileBehavior_ApplyElementTags>(ElemBehavior));

		// Invoke hit actor behavior directly
		ElemBehavior.OnProjectileHitActor(Proj, Mob, FHitResult());
		TestEqual(TEXT("ApplyElementTags projectile behavior must apply 1 Lightning stack"),
			Mob->GetElementComponent()->GetElementTagCount(FCGameplayTags::Element_Lightning), 1);

		// Verify pierce counter allows 2 continues then stops
		bool bContinue1 = false;
		Pierce.OnProjectileImpact(Proj, Mob, FHitResult(), bContinue1);
		TestTrue(TEXT("1st pierce impact must continue flight"), bContinue1);

		bool bContinue2 = false;
		Pierce.OnProjectileImpact(Proj, Mob, FHitResult(), bContinue2);
		TestTrue(TEXT("2nd pierce impact must continue flight"), bContinue2);

		bool bContinue3 = false;
		Pierce.OnProjectileImpact(Proj, Mob, FHitResult(), bContinue3);
		TestFalse(TEXT("3rd impact beyond MaxPierceCount=2 must terminate flight"), bContinue3);
	}

	return true;
}

#endif
