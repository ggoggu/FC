#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Card/FCCardDeckComponent.h"
#include "Card/Policies/FCCardTargetPolicies.h"
#include "Card/Conditions/FCCardConditions.h"
#include "Card/Effects/FCCardEffects.h"
#include "Card/Keywords/FCCardKeywordRules.h"
#include "Character/FCCharacterBase.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Combat/Element/FCElementComponent.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardTypes.h"
#include "Data/FCGameplayTags.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCCardPipelineTest, "FC.Card.InstancedStructPipeline", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCCardPipelineTest::RunTest(const FString& Parameters)
{
	UGameInstance* DummyGI = NewObject<UGameInstance>();
	TestNotNull(TEXT("DummyGI should be valid"), DummyGI);
	if (!DummyGI)
	{
		return false;
	}

	UFCCardSubsystem* CardSubsystem = NewObject<UFCCardSubsystem>(DummyGI);
	TestNotNull(TEXT("CardSubsystem should be valid"), CardSubsystem);
	if (!CardSubsystem)
	{
		return false;
	}

	// =========================================================================
	// Test 1: Split Multi-DataTable Merging + LegacyAliases O(1) Resolution
	// =========================================================================
	{
		UDataTable* TableMage = NewObject<UDataTable>(DummyGI);
		TableMage->RowStruct = FFCCardTableRow::StaticStruct();

		FFCCardTableRow DrawComboRow;
		DrawComboRow.GameplayData.CardId = FName("Card_ArcaneInsight");
		DrawComboRow.GameplayData.LegacyAliases = { FName("Card_비전통찰"), FName("ArcaneLegacy") };
		DrawComboRow.GameplayData.BaseManaCost = 0;
		DrawComboRow.GameplayData.TargetPolicy = TInstancedStruct<FFCCardTargetPolicyBase>::Make<FFCCardTargetPolicy_Self>();
		FFCCardEffect_DrawCards DrawEffect;
		DrawEffect.DrawCount = 2;
		DrawComboRow.GameplayData.PlayEffects.Add(TInstancedStruct<FFCCardEffectBase>::Make<FFCCardEffect_DrawCards>(DrawEffect));
		DrawComboRow.DisplayData.CardName = FText::FromString(TEXT("비전 통찰"));
		TableMage->AddRow(FName("Card_ArcaneInsight"), DrawComboRow);

		UDataTable* TableWarrior = NewObject<UDataTable>(DummyGI);
		TableWarrior->RowStruct = FFCCardTableRow::StaticStruct();

		FFCCardTableRow UnplayableCurseRow;
		UnplayableCurseRow.GameplayData.CardId = FName("Card_HeavyCurse");
		UnplayableCurseRow.GameplayData.BaseManaCost = 0;
		UnplayableCurseRow.GameplayData.CardTags.AddTag(FCGameplayTags::Card_Keyword_Unplayable);
		UnplayableCurseRow.DisplayData.CardName = FText::FromString(TEXT("무거운 저주"));
		TableWarrior->AddRow(FName("Card_HeavyCurse"), UnplayableCurseRow);

		FFCCardTableRow InnateOpenerRow;
		InnateOpenerRow.GameplayData.CardId = FName("Card_InnateOpener");
		InnateOpenerRow.GameplayData.BaseManaCost = 0;
		InnateOpenerRow.GameplayData.CardTags.AddTag(FCGameplayTags::Card_Keyword_Innate);
		InnateOpenerRow.DisplayData.CardName = FText::FromString(TEXT("선제 기동"));
		TableWarrior->AddRow(FName("Card_InnateOpener"), InnateOpenerRow);

		CardSubsystem->SetCardDataTable(TableMage);
		CardSubsystem->AddCardDataTable(TableWarrior);

		// Verify split table merge and zero-copy pointer lookup
		const FFCCardTableRow* FoundDirect = CardSubsystem->FindCardRowPtr(FName("Card_ArcaneInsight"));
		const FFCCardTableRow* FoundAlias = CardSubsystem->FindCardRowPtr(FName("Card_비전통찰"));
		const FFCCardTableRow* FoundLegacyAlias = CardSubsystem->FindCardRowPtr(FName("ArcaneLegacy"));
		const FFCCardTableRow* FoundSecondTable = CardSubsystem->FindCardRowPtr(FName("Card_HeavyCurse"));

		TestNotNull(TEXT("Primary table row must resolve via FindCardRowPtr"), FoundDirect);
		TestTrue(TEXT("Alias lookup must resolve to exact same cached row pointer"), FoundDirect == FoundAlias && FoundDirect == FoundLegacyAlias);
		TestNotNull(TEXT("Second split table row must resolve cleanly"), FoundSecondTable);
		TestEqual(TEXT("CardTables count should be 2"), CardSubsystem->GetCardDataTables().Num(), 2);
	}

	// =========================================================================
	// Test 2: Innate Opening Hand Prioritization + Unplayable Block + DrawCards Effect
	// =========================================================================
	{
		AFCCharacterBase* CasterChar = NewObject<AFCCharacterBase>();
		UFCCardDeckComponent* DeckComp = NewObject<UFCCardDeckComponent>(CasterChar);

		TArray<FName> DeckIds = {
			FName("Card_HeavyCurse"),
			FName("Card_HeavyCurse"),
			FName("Card_HeavyCurse"),
			FName("Card_ArcaneInsight"),
			FName("Card_InnateOpener")
		};
		DeckComp->InitializeDeck(DeckIds);
		DeckComp->DrawCards(1);

		TestEqual(TEXT("Innate card must be drawn first on opening draw"), DeckComp->GetHandContainer().Items[0].CardId, FName("Card_InnateOpener"));

		// Add Unplayable curse to hand and attempt to play -> must be blocked by KeywordRule_Unplayable
		DeckComp->AddCardToDeck(FName("Card_HeavyCurse"), EFCCardZone::Hand);
		FGuid CurseGuid;
		for (const FFCCardItem& Item : DeckComp->GetHandContainer().Items)
		{
			if (Item.CardId == FName("Card_HeavyCurse"))
			{
				CurseGuid = Item.CardGuid;
				break;
			}
		}
		TestTrue(TEXT("CurseGuid should be valid"), CurseGuid.IsValid());
		const int32 HandBeforeCurse = DeckComp->GetHandContainer().Num();
		DeckComp->Server_PlayCard(CurseGuid, FFCCardTargetInfo());
		TestEqual(TEXT("Unplayable card must stay in hand after Server_PlayCard"), DeckComp->GetHandContainer().Num(), HandBeforeCurse);

		// Add ArcaneInsight (Draw 2) and play -> removes itself (-1) and draws 2 (+2) => net +1
		DeckComp->AddCardToDeck(FName("Card_ArcaneInsight"), EFCCardZone::Hand);
		FGuid InsightGuid;
		for (const FFCCardItem& Item : DeckComp->GetHandContainer().Items)
		{
			if (Item.CardId == FName("Card_ArcaneInsight"))
			{
				InsightGuid = Item.CardGuid;
				break;
			}
		}
		const int32 HandBeforeInsight = DeckComp->GetHandContainer().Num();
		DeckComp->Server_PlayCard(InsightGuid, FFCCardTargetInfo());
		TestEqual(TEXT("Playing Draw-2 card must yield net +1 hand size"), DeckComp->GetHandContainer().Num(), HandBeforeInsight + 1);
		TestEqual(TEXT("Played card must move to DiscardPile"), DeckComp->GetDiscardPileCount(), 1);
	}

	// =========================================================================
	// Test 3: Conditional Element Consumption + ApplyElementTags Pipeline
	// =========================================================================
	{
		FFCCardTableRow ConditionalBurstRow;
		ConditionalBurstRow.GameplayData.CardId = FName("Card_SteamBurst");
		ConditionalBurstRow.GameplayData.BaseManaCost = 0;

		FFCCardTargetPolicy_SingleTarget SinglePolicy;
		SinglePolicy.bRequireLineOfSight = false;
		ConditionalBurstRow.GameplayData.TargetPolicy = TInstancedStruct<FFCCardTargetPolicyBase>::Make<FFCCardTargetPolicy_SingleTarget>(SinglePolicy);

		FFCCardCondition_ConsumeElementTags ConsumeFire;
		ConsumeFire.RequiredElementTags = { FCGameplayTags::Element_Fire };
		ConsumeFire.bBlockCardPlayIfUnmet = false;
		ConditionalBurstRow.GameplayData.Conditions.Add(TInstancedStruct<FFCCardConditionBase>::Make<FFCCardCondition_ConsumeElementTags>(ConsumeFire));

		FFCCardEffect_ApplyElementTags ApplyWater;
		ApplyWater.ElementTags = { FCGameplayTags::Element_Water };
		ConditionalBurstRow.GameplayData.PlayEffects.Add(TInstancedStruct<FFCCardEffectBase>::Make<FFCCardEffect_ApplyElementTags>(ApplyWater));

		CardSubsystem->RegisterCardRow(FName("Card_SteamBurst"), ConditionalBurstRow);

		AFCCharacterBase* CasterChar = NewObject<AFCCharacterBase>();
		UFCCardDeckComponent* DeckComp = NewObject<UFCCardDeckComponent>(CasterChar);
		AFCMobCharacter* TargetMob = NewObject<AFCMobCharacter>();

		// Case A: Target has NO Fire -> card plays & fizzles, no Water applied
		DeckComp->AddCardToDeck(FName("Card_SteamBurst"), EFCCardZone::Hand);
		FFCCardTargetInfo TargetInfo;
		TargetInfo.TargetActor = TargetMob;
		if (TestTrue(TEXT("Hand must contain Card_SteamBurst for Case A"), DeckComp->GetHandContainer().Items.Num() > 0))
		{
			DeckComp->Server_PlayCard(DeckComp->GetHandContainer().Items[0].CardGuid, TargetInfo);
		}
		TestEqual(TEXT("Fizzled conditional card must still move to discard"), DeckComp->GetDiscardPileCount(), 1);
		TestEqual(TEXT("Fizzled card must not apply Water stack"), TargetMob->GetElementComponent()->GetElementTagCount(FCGameplayTags::Element_Water), 0);

		// Case B: Target has 1 Fire -> consumes Fire and applies 1 Water
		TargetMob->GetElementComponent()->AddElementTagStack(FCGameplayTags::Element_Fire, 1);
		DeckComp->AddCardToDeck(FName("Card_SteamBurst"), EFCCardZone::Hand);
		if (TestTrue(TEXT("Hand must contain Card_SteamBurst for Case B"), DeckComp->GetHandContainer().Items.Num() > 0))
		{
			DeckComp->Server_PlayCard(DeckComp->GetHandContainer().Items[0].CardGuid, TargetInfo);
		}
		TestEqual(TEXT("Consumed Fire stack must drop to 0"), TargetMob->GetElementComponent()->GetElementTagCount(FCGameplayTags::Element_Fire), 0);
		TestEqual(TEXT("Succeeded conditional card must apply 1 Water stack"), TargetMob->GetElementComponent()->GetElementTagCount(FCGameplayTags::Element_Water), 1);

		CardSubsystem->Deinitialize();
	}

	return true;
}

#endif
