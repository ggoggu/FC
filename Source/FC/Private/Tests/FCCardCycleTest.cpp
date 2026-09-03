#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Card/FCCardDeckComponent.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardTypes.h"
#include "Character/FCCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCCardCycleTest, "FC.Card.CycleSystem", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCCardCycleTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Keyword Helpers (DoesRetain & IsEthereal)
	// =========================================================================
	{
		FFCCardGameplayData RetainCardData;
		RetainCardData.CardId = FName("Card_RetainTest");
		RetainCardData.Keywords = { EFCCardKeyword::Retain };

		TestTrue(TEXT("Retain card must return DoesRetain()=true"), RetainCardData.DoesRetain());
		TestFalse(TEXT("Retain card must return IsEthereal()=false"), RetainCardData.IsEthereal());

		FFCCardGameplayData EtherealCardData;
		EtherealCardData.CardId = FName("Card_EtherealTest");
		EtherealCardData.Keywords = { EFCCardKeyword::Ethereal };

		TestTrue(TEXT("Ethereal card must return IsEthereal()=true"), EtherealCardData.IsEthereal());
		TestFalse(TEXT("Ethereal card must return DoesRetain()=false"), EtherealCardData.DoesRetain());

		FFCCardGameplayData NormalCardData;
		NormalCardData.CardId = FName("Card_NormalTest");

		TestFalse(TEXT("Normal card must return DoesRetain()=false"), NormalCardData.DoesRetain());
		TestFalse(TEXT("Normal card must return IsEthereal()=false"), NormalCardData.IsEthereal());

		// Tag-based check
		FFCCardGameplayData TagRetainData;
		TagRetainData.CardTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Card.Keyword.Retain"), false));
		if (TagRetainData.CardTags.IsValid())
		{
			TestTrue(TEXT("Tag-based card must return DoesRetain()=true"), TagRetainData.DoesRetain());
		}

		FFCCardGameplayData TagEtherealData;
		TagEtherealData.CardTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Card.Keyword.Ethereal"), false));
		if (TagEtherealData.CardTags.IsValid())
		{
			TestTrue(TEXT("Tag-based card must return IsEthereal()=true"), TagEtherealData.IsEthereal());
		}
	}

	// =========================================================================
	// Test 2: Hand Card Routing, Mana Refresh & Draw on ExecuteHandCycle
	// =========================================================================
	{
		UGameInstance* DummyGI = NewObject<UGameInstance>();
		TestNotNull(TEXT("DummyGI should be created"), DummyGI);

		if (DummyGI)
		{
			UFCCardSubsystem* Subsystem = NewObject<UFCCardSubsystem>(DummyGI);
			TestNotNull(TEXT("Subsystem should be created"), Subsystem);

			// Register Normal Card
			UFCCardDataAsset* NormalAsset = NewObject<UFCCardDataAsset>(Subsystem);
			NormalAsset->GameplayData.CardId = FName("Card_NormalTest");
			NormalAsset->GameplayData.BaseManaCost = 1;
			NormalAsset->DisplayData.CardName = FText::FromString(TEXT("일반 카드"));
			Subsystem->RegisterCardDataAsset(NormalAsset);

			// Register Retain Card
			UFCCardDataAsset* RetainAsset = NewObject<UFCCardDataAsset>(Subsystem);
			RetainAsset->GameplayData.CardId = FName("Card_RetainTest");
			RetainAsset->GameplayData.BaseManaCost = 1;
			RetainAsset->GameplayData.Keywords = { EFCCardKeyword::Retain };
			RetainAsset->DisplayData.CardName = FText::FromString(TEXT("보존 카드"));
			Subsystem->RegisterCardDataAsset(RetainAsset);

			// Register Ethereal Card
			UFCCardDataAsset* EtherealAsset = NewObject<UFCCardDataAsset>(Subsystem);
			EtherealAsset->GameplayData.CardId = FName("Card_EtherealTest");
			EtherealAsset->GameplayData.BaseManaCost = 1;
			EtherealAsset->GameplayData.Keywords = { EFCCardKeyword::Ethereal };
			EtherealAsset->DisplayData.CardName = FText::FromString(TEXT("휘발성 카드"));
			Subsystem->RegisterCardDataAsset(EtherealAsset);

			// Register Draw Filler Card
			UFCCardDataAsset* FillerAsset = NewObject<UFCCardDataAsset>(Subsystem);
			FillerAsset->GameplayData.CardId = FName("Card_Filler");
			FillerAsset->GameplayData.BaseManaCost = 1;
			FillerAsset->DisplayData.CardName = FText::FromString(TEXT("드로우용 카드"));
			Subsystem->RegisterCardDataAsset(FillerAsset);

			AActor* OwnerActor = NewObject<AActor>();
			UFCCardDeckComponent* DeckComp = NewObject<UFCCardDeckComponent>(OwnerActor);
			TestNotNull(TEXT("DeckComp should be created"), DeckComp);

			if (DeckComp)
			{
				// Setup Draw pile with 10 filler cards
				TArray<FName> DrawDeck;
				for (int32 i = 0; i < 10; ++i)
				{
					DrawDeck.Add(FName("Card_Filler"));
				}
				DeckComp->InitializeDeck(DrawDeck);

				// Manually place Normal, Retain, and Ethereal cards into hand
				DeckComp->AddCardToDeck(FName("Card_NormalTest"), EFCCardAddDestination::Hand);
				DeckComp->AddCardToDeck(FName("Card_RetainTest"), EFCCardAddDestination::Hand);
				DeckComp->AddCardToDeck(FName("Card_EtherealTest"), EFCCardAddDestination::Hand);

				TestEqual(TEXT("Initial Hand size must be 3"), DeckComp->GetHandContainer().Num(), 3);
				TestEqual(TEXT("Initial Discard count must be 0"), DeckComp->GetDiscardPileCount(), 0);
				TestEqual(TEXT("Initial Exhaust count must be 0"), DeckComp->GetExhaustPileCount(), 0);

				// Set Cycle parameters: Draw 5 cards per cycle
				DeckComp->SetCycleDrawCount(5);
				TestEqual(TEXT("CycleDrawCount must be 5"), DeckComp->GetCycleDrawCount(), 5);

				// Execute Cycle!
				DeckComp->ExecuteHandCycle();

				// Check 1: Normal card should be discarded (DiscardPile = 1)
				TestEqual(TEXT("Discard pile must have 1 card (Normal)"), DeckComp->GetDiscardPileCount(), 1);
				TestTrue(TEXT("ServerDiscardPile should contain Card_NormalTest"), DeckComp->GetServerDiscardPile().Contains(FName("Card_NormalTest")));

				// Check 2: Ethereal card should be exhausted (ExhaustPile = 1)
				TestEqual(TEXT("Exhaust pile must have 1 card (Ethereal)"), DeckComp->GetExhaustPileCount(), 1);
				TestTrue(TEXT("ServerExhaustPile should contain Card_EtherealTest"), DeckComp->GetServerExhaustPile().Contains(FName("Card_EtherealTest")));

				// Check 3: Hand should contain Retain card + 5 drawn cards = 6 cards
				TestEqual(TEXT("Hand size must be 6 (1 Retain + 5 drawn)"), DeckComp->GetHandContainer().Num(), 6);

				bool bFoundRetained = false;
				int32 FillerCount = 0;
				for (const FFCCardItem& Item : DeckComp->GetHandContainer().Items)
				{
					if (Item.CardId == FName("Card_RetainTest"))
					{
						bFoundRetained = true;
					}
					else if (Item.CardId == FName("Card_Filler"))
					{
						FillerCount++;
					}
				}
				TestTrue(TEXT("Retain card must still be in hand"), bFoundRetained);
				TestEqual(TEXT("Hand must contain 5 drawn Filler cards"), FillerCount, 5);
			}
		}
	}

	// =========================================================================
	// Test 3: UFCAttributeSet Mana Refresh
	// =========================================================================
	{
		AActor* AttrActor = NewObject<AActor>();
		UFCAttributeSet* AttrSet = NewObject<UFCAttributeSet>(AttrActor);
		TestNotNull(TEXT("AttrSet should be created"), AttrSet);

		if (AttrSet)
		{
			AttrSet->InitMaxMana(50.0f);
			AttrSet->InitMana(0.0f);
			TestEqual(TEXT("Mana before refresh must be 0.0"), AttrSet->GetMana(), 0.0f);
			TestEqual(TEXT("MaxMana must be 50.0"), AttrSet->GetMaxMana(), 50.0f);

			AttrSet->RefreshMana();
			TestEqual(TEXT("Mana after refresh must be restored to MaxMana (50.0)"), AttrSet->GetMana(), 50.0f);
		}
	}

	// =========================================================================
	// Test 4: Configurable Parameters & Timer API
	// =========================================================================
	{
		AActor* OwnerActor = NewObject<AActor>();
		UFCCardDeckComponent* DeckComp = NewObject<UFCCardDeckComponent>(OwnerActor);
		TestNotNull(TEXT("DeckComp should be created"), DeckComp);

		if (DeckComp)
		{
			// Default check
			TestEqual(TEXT("Default CycleInterval should be 30.0f"), DeckComp->GetCycleInterval(), 30.0f);
			TestEqual(TEXT("Default CycleDrawCount should be 5"), DeckComp->GetCycleDrawCount(), 5);
			TestTrue(TEXT("Default bAutoCycleEnabled should be true"), DeckComp->IsAutoCycleEnabled());

			// Adjust interval
			DeckComp->SetCycleInterval(15.0f);
			TestEqual(TEXT("Modified CycleInterval should be 15.0f"), DeckComp->GetCycleInterval(), 15.0f);

			// Interval clamp min 1.0f
			DeckComp->SetCycleInterval(-5.0f);
			TestEqual(TEXT("Negative CycleInterval should clamp to 1.0f"), DeckComp->GetCycleInterval(), 1.0f);

			// Adjust draw count
			DeckComp->SetCycleDrawCount(3);
			TestEqual(TEXT("Modified CycleDrawCount should be 3"), DeckComp->GetCycleDrawCount(), 3);

			// Draw count clamp min 0
			DeckComp->SetCycleDrawCount(-2);
			TestEqual(TEXT("Negative CycleDrawCount should clamp to 0"), DeckComp->GetCycleDrawCount(), 0);

			// Toggle auto cycle
			DeckComp->SetAutoCycleEnabled(false);
			TestFalse(TEXT("AutoCycleEnabled should be false"), DeckComp->IsAutoCycleEnabled());
		}
	}

	return true;
}

#endif
