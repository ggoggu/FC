#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Data/Class/FCClassSubsystem.h"
#include "Data/Class/FCClassTypes.h"
#include "Data/Class/Traits/FCClassTraitBehaviors.h"
#include "Data/FCGameplayTags.h"
#include "Combat/Element/FCElementComponent.h"
#include "Combat/FCCombatUtils.h"
#include "Card/Effects/FCCardEffects.h"
#include "Character/Mob/FCMobCharacter.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "UI/ViewModel/FCElementOverheadViewModel.h"
#include "UI/View/FCElementStackItemWidget.h"
#include "UI/ClassSelect/FCClassSelectViewModel.h"
#include "UI/ClassSelect/FCClassSelectItemViewModel.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "GameplayTagsManager.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCTagCatalogAndReactionTest, "FC.Combat.TagCatalogAndReactions", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCTagCatalogAndReactionTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Open-Ended Custom Element DataTable Row Flows End-to-End Without C++ Changes
	// =========================================================================
	{
		UGameInstance* GI = NewObject<UGameInstance>();
		UFCClassSubsystem* ClassSub = NewObject<UFCClassSubsystem>(GI);
		TestNotNull(TEXT("ClassSubsystem must instantiate"), ClassSub);

		if (ClassSub)
		{
			// Register a custom element definition in an injected DataTable (overrides or extends catalog)
			UDataTable* CustomElementTable = NewObject<UDataTable>(GI);
			CustomElementTable->RowStruct = FFCElementDefinitionRow::StaticStruct();

			FFCElementDefinitionRow CustomElemRow;
			CustomElemRow.ElementTag = FCGameplayTags::Element_Lightning;
			CustomElemRow.DisplayName = FText::FromString(TEXT("초전도 번개"));
			CustomElemRow.ShortCode = TEXT("⚡");
			CustomElemRow.Color = FLinearColor(0.2f, 0.9f, 1.0f, 1.0f);
			CustomElementTable->AddRow(FName("Element_SuperLightning"), CustomElemRow);

			ClassSub->AddElementDataTable(CustomElementTable);

			const FFCElementDefinitionRow* FoundElem = ClassSub->FindElementRowPtr(FCGameplayTags::Element_Lightning);
			TestNotNull(TEXT("Custom element DataTable row must be resolvable"), FoundElem);
			if (FoundElem)
			{
				TestEqual(TEXT("Custom DisplayName must override builtin"), FoundElem->DisplayName.ToString(), FString(TEXT("초전도 번개")));
				TestEqual(TEXT("Custom ShortCode must override builtin"), FoundElem->ShortCode, FString(TEXT("⚡")));
			}

			// Verify UI helpers pick up DataTable values dynamically
			TestEqual(TEXT("UI ShortName must read from DataTable"),
				UFCElementStackItemWidget::GetShortNameForElement(FCGameplayTags::Element_Lightning, ClassSub),
				FString(TEXT("⚡")));
			TestEqual(TEXT("UI DisplayName must read from DataTable"),
				UFCElementStackItemWidget::GetDisplayNameForElement(FCGameplayTags::Element_Lightning, ClassSub).ToString(),
				FString(TEXT("초전도 번개")));

			// Verify ElementComponent -> OverheadViewModel flows dynamic counts via TMap
			AFCMobCharacter* Mob = NewObject<AFCMobCharacter>();
			UFCElementOverheadViewModel* OverheadVM = NewObject<UFCElementOverheadViewModel>();
			OverheadVM->BindToElementComponent(Mob->GetElementComponent());

			Mob->GetElementComponent()->AddElementStack(FCGameplayTags::Element_Lightning, 2);
			TestEqual(TEXT("OverheadVM dynamic map count for Lightning must be 2"),
				OverheadVM->GetElementCount(FCGameplayTags::Element_Lightning), 2);
			TestEqual(TEXT("OverheadVM TotalStacks must be 2"), OverheadVM->TotalStacks, 2);

			OverheadVM->UnbindFromElementComponent();
			ClassSub->Deinitialize();
		}
	}

	// =========================================================================
	// Test 2: DataTable-Driven Elemental Reactions (Fire + Water -> Vaporize Burst)
	// =========================================================================
	{
		UGameInstance* GI = NewObject<UGameInstance>();
		UFCClassSubsystem* ClassSub = NewObject<UFCClassSubsystem>(GI);
		if (ClassSub)
		{
			UDataTable* ReactionTable = NewObject<UDataTable>(GI);
			ReactionTable->RowStruct = FFCElementReactionRow::StaticStruct();

			FFCElementReactionRow VaporizeRow;
			VaporizeRow.ReactionId = FName("Reaction_Vaporize");
			VaporizeRow.DisplayName = FText::FromString(TEXT("증발"));
			VaporizeRow.RequiredElementTags = { FCGameplayTags::Element_Fire.GetTag(), FCGameplayTags::Element_Water.GetTag() };
			VaporizeRow.bConsumeTriggerElements = true;
			VaporizeRow.Priority = 10;

			// Attach a composable TInstancedStruct card effect to run on reaction trigger:
			// Apply +1 Wind stack (e.g. Steam cloud) as a verifiable pure-data side effect
			FFCCardEffect_ApplyElementTags SteamEffect;
			SteamEffect.ElementTags = { FCGameplayTags::Element_Wind.GetTag() };
			VaporizeRow.ReactionEffects.Add(TInstancedStruct<FFCCardEffectBase>::Make<FFCCardEffect_ApplyElementTags>(SteamEffect));

			ReactionTable->AddRow(FName("Reaction_Vaporize"), VaporizeRow);
			ClassSub->AddElementReactionDataTable(ReactionTable);

			AFCMobCharacter* TargetMob = NewObject<AFCMobCharacter>();
			UFCElementComponent* ElemComp = TargetMob->GetElementComponent();
			TestNotNull(TEXT("Mob ElementComponent must exist"), ElemComp);

			if (ElemComp)
			{
				// Add Fire alone -> no reaction yet
				ElemComp->AddElementStack(FCGameplayTags::Element_Fire, 1);
				TestEqual(TEXT("Fire stack must be 1 before Water arrives"), ElemComp->GetElementCount(FCGameplayTags::Element_Fire), 1);
				TestEqual(TEXT("Wind (Steam) must be 0 before reaction"), ElemComp->GetElementCount(FCGameplayTags::Element_Wind), 0);

				// Add Water -> Triggers Reaction_Vaporize automatically!
				// Consumes Fire + Water and spawns Wind (Steam) via TInstancedStruct ReactionEffects!
				ElemComp->AddElementStack(FCGameplayTags::Element_Water, 1);
				TestEqual(TEXT("Fire stack must be consumed by Vaporize"), ElemComp->GetElementCount(FCGameplayTags::Element_Fire), 0);
				TestEqual(TEXT("Water stack must be consumed by Vaporize"), ElemComp->GetElementCount(FCGameplayTags::Element_Water), 0);
				TestEqual(TEXT("Wind (Steam) stack must be applied by ReactionEffects"), ElemComp->GetElementCount(FCGameplayTags::Element_Wind), 1);
			}

			ClassSub->Deinitialize();
		}
	}

	// =========================================================================
	// Test 3: ClassSelectViewModel Driven Purely by DT_ClassCatalog (Zero Hardcoded Switches)
	// =========================================================================
	{
		UGameInstance* GI = NewObject<UGameInstance>();
		UFCClassSubsystem* ClassSub = NewObject<UFCClassSubsystem>(GI);
		if (ClassSub)
		{
			// Unlock Warrior via DataTable row without editing a single line of C++ UI code
			FFCCharacterClassData UnlockedWarrior = *UFCClassSubsystem::FindBuiltinClassDefault(FCGameplayTags::Class_Warrior);
			UnlockedWarrior.bIsSelectable = true;
			UnlockedWarrior.LockReasonText = FText::GetEmpty();
			ClassSub->RegisterClassRow(UnlockedWarrior);

			UFCClassSelectViewModel* SelectVM = NewObject<UFCClassSelectViewModel>();
			SelectVM->InitializeClasses(ClassSub);

			TestTrue(TEXT("ClassItemList must populate playable classes from subsystem"), SelectVM->ClassItemList.Num() >= 4);
			TestTrue(TEXT("Warrior should now be selectable via DataTable flag alone"), SelectVM->SelectClass(FCGameplayTags::Class_Warrior));
			TestTrue(TEXT("SelectedClassTag must match Warrior"), SelectVM->SelectedClassTag == FCGameplayTags::Class_Warrior);
			TestFalse(TEXT("Rogue remains locked via DataTable flag"), SelectVM->SelectClass(FCGameplayTags::Class_Rogue));
			ClassSub->Deinitialize();
		}
	}

	return true;
}

#endif
