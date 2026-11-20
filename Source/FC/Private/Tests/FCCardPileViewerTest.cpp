#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Card/FCCardDeckComponent.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardPileContainer.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardTypes.h"
#include "Data/FCPlayerPersistenceTypes.h"
#include "UI/ViewModel/FCHUDViewModel.h"
#include "UI/ViewModel/FCCardPileViewModel.h"
#include "UI/ViewModel/FCCardViewModel.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCCardPileViewerTest, "FC.Card.PileViewer", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCCardPileViewerTest::RunTest(const FString& Parameters)
{
	UGameInstance* DummyGI = NewObject<UGameInstance>();
	TestNotNull(TEXT("DummyGI should be created"), DummyGI);
	if (!DummyGI)
	{
		return false;
	}

	UFCCardSubsystem* Subsystem = NewObject<UFCCardSubsystem>(DummyGI);
	TestNotNull(TEXT("CardSubsystem should be created"), Subsystem);
	if (!Subsystem)
	{
		return false;
	}

	auto RegisterCard = [Subsystem](FName CardId, EFCCardType CardType, int32 ManaCost, const TCHAR* DisplayName)
	{
		UFCCardDataAsset* Asset = NewObject<UFCCardDataAsset>(Subsystem);
		Asset->GameplayData.CardId = CardId;
		Asset->GameplayData.CardType = CardType;
		Asset->GameplayData.BaseManaCost = ManaCost;
		Asset->DisplayData.CardName = FText::FromString(DisplayName);
		Subsystem->RegisterCardDataAsset(Asset);
	};

	RegisterCard(FName("Card_TestFireball"), EFCCardType::Attack, 2, TEXT("파이어 볼"));
	RegisterCard(FName("Card_TestFireArrow"), EFCCardType::Attack, 1, TEXT("파이어 애로우"));
	RegisterCard(FName("Card_TestAttackBuff"), EFCCardType::Skill, 1, TEXT("공격력 강화"));
	RegisterCard(FName("Card_TestPower"), EFCCardType::Power, 0, TEXT("마력 각성"));

	AActor* OwnerActor = NewObject<AActor>();
	UFCCardDeckComponent* DeckComp = NewObject<UFCCardDeckComponent>(OwnerActor);
	TestNotNull(TEXT("DeckComp should be created"), DeckComp);
	if (!DeckComp)
	{
		return false;
	}

	// =========================================================================
	// Test 1: Canonical Summary Aggregation & Fog-of-War Order Hiding
	// =========================================================================
	{
		const TArray<FName> InitialDeck = {
			FName("Card_TestFireball"),
			FName("Card_TestPower"),
			FName("Card_TestFireArrow"),
			FName("Card_TestFireball"),
			FName("Card_TestAttackBuff")
		};
		DeckComp->InitializeDeck(InitialDeck);

		const FFCCardPileContainer& DrawSummary = DeckComp->GetDrawPileSummary();
		TestEqual(TEXT("DrawPileSummary total count must match DrawPileCount"), DrawSummary.GetTotalCardCount(), DeckComp->GetDrawPileCount());
		TestEqual(TEXT("DrawPileSummary should contain 4 distinct CardId entries"), DrawSummary.Items.Num(), 4);

		// Verify entries are sorted canonically by CardId (LexicalLess), NOT draw order
		for (int32 i = 1; i < DrawSummary.Items.Num(); ++i)
		{
			TestTrue(
				TEXT("DrawPileSummary entries must be canonically sorted by CardId"),
				DrawSummary.Items[i - 1].CardId.LexicalLess(DrawSummary.Items[i].CardId));
		}
	}

	// =========================================================================
	// Test 2: Pile Summaries Sync on Draw, Discard, Reshuffle & Restore
	// =========================================================================
	{
		DeckComp->DrawCards(3);
		TestEqual(TEXT("DrawPileSummary total must drop to 2 after drawing 3"), DeckComp->GetDrawPileSummary().GetTotalCardCount(), 2);

		// Discard all cards from hand
		DeckComp->DiscardEntireHand();
		TestEqual(TEXT("DiscardPileSummary total must be 3 after DiscardEntireHand"), DeckComp->GetDiscardPileSummary().GetTotalCardCount(), 3);

		// Restore snapshot with specific discard & draw counts
		FFCCardDeckSaveData Snapshot = DeckComp->ExportDeckSaveData();
		Snapshot.DrawPile = { FName("Card_TestFireball"), FName("Card_TestAttackBuff") };
		Snapshot.DiscardPile = { FName("Card_TestPower"), FName("Card_TestFireArrow"), FName("Card_TestFireball") };
		DeckComp->RestoreFromDeckSaveData(Snapshot);

		TestEqual(TEXT("Restored DrawPileSummary total must be 2"), DeckComp->GetDrawPileSummary().GetTotalCardCount(), 2);
		TestEqual(TEXT("Restored DiscardPileSummary total must be 3"), DeckComp->GetDiscardPileSummary().GetTotalCardCount(), 3);
	}

	// =========================================================================
	// Test 3: ViewModel Canonical Display Sorting, Toggle, Switch & Live Sync
	// =========================================================================
	{
		UFCHUDViewModel* HUDVM = NewObject<UFCHUDViewModel>();
		UFCCardPileViewModel* PileVM = HUDVM->GetOrCreatePileViewerViewModel();
		TestNotNull(TEXT("PileViewerViewModel should be created"), PileVM);

		if (PileVM)
		{
			FFCCardPileContainer DrawSummary;
			DrawSummary.SyncFromCardIds({
				FName("Card_TestPower"),
				FName("Card_TestFireball"),
				FName("Card_TestFireball"),
				FName("Card_TestAttackBuff"),
				FName("Card_TestFireArrow")
			});

			FFCCardPileContainer DiscardSummary;
			DiscardSummary.SyncFromCardIds({
				FName("Card_TestAttackBuff"),
				FName("Card_TestFireArrow")
			});

			PileVM->UpdatePileSummaries(DrawSummary, DiscardSummary, Subsystem);
			TestFalse(TEXT("Pile viewer must initially be closed"), HUDVM->IsPileViewerOpen());

			// Open Draw Pile -> Attack (1-cost FireArrow, 2-cost Fireball x2) -> Skill (AttackBuff) -> Power (TestPower)
			HUDVM->TogglePileViewer(EFCCardZone::DrawPile);
			TestTrue(TEXT("Pile viewer should be open after clicking DrawPile"), HUDVM->IsPileViewerOpen());
			TestEqual(TEXT("ActiveZone should be DrawPile"), PileVM->ActiveZone, EFCCardZone::DrawPile);
			TestEqual(TEXT("DrawPile CardCount should be 5"), PileVM->CardCount, 5);
			TestEqual(TEXT("Cards array must contain 5 expanded cards"), PileVM->Cards.Num(), 5);

			if (PileVM->Cards.Num() == 5)
			{
				TestEqual(TEXT("Sorted[0] should be 1-cost Attack (Card_TestFireArrow)"), PileVM->Cards[0]->CardId, FName("Card_TestFireArrow"));
				TestEqual(TEXT("Sorted[1] should be 2-cost Attack (Card_TestFireball)"), PileVM->Cards[1]->CardId, FName("Card_TestFireball"));
				TestEqual(TEXT("Sorted[2] should be 2-cost Attack (Card_TestFireball)"), PileVM->Cards[2]->CardId, FName("Card_TestFireball"));
				TestEqual(TEXT("Sorted[3] should be Skill (Card_TestAttackBuff)"), PileVM->Cards[3]->CardId, FName("Card_TestAttackBuff"));
				TestEqual(TEXT("Sorted[4] should be Power (Card_TestPower)"), PileVM->Cards[4]->CardId, FName("Card_TestPower"));
			}

			// Switch directly to Discard Pile
			HUDVM->TogglePileViewer(EFCCardZone::DiscardPile);
			TestTrue(TEXT("Pile viewer remains open when switching to DiscardPile"), HUDVM->IsPileViewerOpen());
			TestEqual(TEXT("ActiveZone should switch to DiscardPile"), PileVM->ActiveZone, EFCCardZone::DiscardPile);
			TestEqual(TEXT("DiscardPile CardCount should be 2"), PileVM->CardCount, 2);
			if (PileVM->Cards.Num() == 2)
			{
				TestEqual(TEXT("Discard Sorted[0] should be Attack (Card_TestFireArrow)"), PileVM->Cards[0]->CardId, FName("Card_TestFireArrow"));
				TestEqual(TEXT("Discard Sorted[1] should be Skill (Card_TestAttackBuff)"), PileVM->Cards[1]->CardId, FName("Card_TestAttackBuff"));
			}

			// Live update while Discard Pile is open
			DiscardSummary.SyncFromCardIds({
				FName("Card_TestPower"),
				FName("Card_TestAttackBuff"),
				FName("Card_TestFireArrow")
			});
			PileVM->UpdatePileSummaries(DrawSummary, DiscardSummary, Subsystem);
			TestEqual(TEXT("Open DiscardPile must update live to 3 cards"), PileVM->Cards.Num(), 3);
			if (PileVM->Cards.Num() == 3)
			{
				TestEqual(TEXT("Updated Discard Sorted[2] should be Power (Card_TestPower)"), PileVM->Cards[2]->CardId, FName("Card_TestPower"));
			}

			// Toggle same zone closes modal
			HUDVM->TogglePileViewer(EFCCardZone::DiscardPile);
			TestFalse(TEXT("Clicking active zone again must close the pile viewer"), HUDVM->IsPileViewerOpen());
		}
	}

	return true;
}

#endif
