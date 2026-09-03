#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Card/FCCardDeckComponent.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardTypes.h"
#include "UI/ViewModel/FCCardViewModel.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCExhaustCardTest, "FC.Card.ExhaustSystem", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCExhaustCardTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Card Definition & Exhaust Keyword Helpers
	// =========================================================================
	{
		FFCCardGameplayData ExhaustCardData;
		ExhaustCardData.CardId = FName("Card_ExhaustTest");
		ExhaustCardData.bExhaustsOnPlay = true;
		ExhaustCardData.Keywords = { EFCCardKeyword::Exhaust };

		TestTrue(TEXT("bExhaustsOnPlay=true must return DoesExhaustOnPlay()=true"), ExhaustCardData.DoesExhaustOnPlay());
		TestEqual(TEXT("Exhaust card formatted keyword must contain '소멸'"), ExhaustCardData.GetFormattedKeywordsText().ToString(), FString(TEXT("소멸")));

		FFCCardGameplayData NormalCardData;
		NormalCardData.CardId = FName("Card_NormalTest");
		NormalCardData.bExhaustsOnPlay = false;

		TestFalse(TEXT("Normal card must return DoesExhaustOnPlay()=false"), NormalCardData.DoesExhaustOnPlay());
		TestTrue(TEXT("Normal card formatted keywords must be empty"), NormalCardData.GetFormattedKeywordsText().IsEmpty());

		// Test tag-based exhaust detection
		FFCCardGameplayData TaggedCardData;
		TaggedCardData.CardId = FName("Card_TagExhaustTest");
		TaggedCardData.CardTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Card.Keyword.Exhaust"), false));
		if (TaggedCardData.CardTags.IsValid())
		{
			TestTrue(TEXT("Tag-based card must return DoesExhaustOnPlay()=true"), TaggedCardData.DoesExhaustOnPlay());
		}
	}

	// =========================================================================
	// Test 2: Server PlayCard Execution & Zone Routing (Discard vs Exhaust)
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
			NormalAsset->GameplayData.BaseManaCost = 0;
			NormalAsset->GameplayData.CardType = EFCCardType::Attack;
			NormalAsset->GameplayData.bExhaustsOnPlay = false;
			NormalAsset->DisplayData.CardName = FText::FromString(TEXT("일반 공격"));
			Subsystem->RegisterCardDataAsset(NormalAsset);

			// Register Exhaust Card
			UFCCardDataAsset* ExhaustAsset = NewObject<UFCCardDataAsset>(Subsystem);
			ExhaustAsset->GameplayData.CardId = FName("Card_ExhaustTest");
			ExhaustAsset->GameplayData.BaseManaCost = 0;
			ExhaustAsset->GameplayData.CardType = EFCCardType::Skill;
			ExhaustAsset->GameplayData.bExhaustsOnPlay = true;
			ExhaustAsset->GameplayData.Keywords = { EFCCardKeyword::Exhaust };
			ExhaustAsset->DisplayData.CardName = FText::FromString(TEXT("소멸 스킬"));
			Subsystem->RegisterCardDataAsset(ExhaustAsset);

			// Setup Actor & DeckComponent
			AActor* OwnerActor = NewObject<AActor>();
			UFCCardDeckComponent* DeckComp = NewObject<UFCCardDeckComponent>(OwnerActor);
			TestNotNull(TEXT("DeckComp should be created"), DeckComp);

			if (DeckComp)
			{
				DeckComp->InitializeDeck({ FName("Card_NormalTest"), FName("Card_ExhaustTest") });
				DeckComp->DrawCards(2);

				TestEqual(TEXT("Hand should have 2 cards"), DeckComp->GetHandContainer().Num(), 2);
				TestEqual(TEXT("Initial Discard count must be 0"), DeckComp->GetDiscardPileCount(), 0);
				TestEqual(TEXT("Initial Exhaust count must be 0"), DeckComp->GetExhaustPileCount(), 0);

				// Find and Play Normal Card
				FGuid NormalGuid;
				FGuid ExhaustGuid;
				for (const FFCCardItem& Item : DeckComp->GetHandContainer().Items)
				{
					if (Item.CardId == FName("Card_NormalTest"))
					{
						NormalGuid = Item.CardGuid;
					}
					else if (Item.CardId == FName("Card_ExhaustTest"))
					{
						ExhaustGuid = Item.CardGuid;
					}
				}

				TestTrue(TEXT("NormalGuid must be valid"), NormalGuid.IsValid());
				TestTrue(TEXT("ExhaustGuid must be valid"), ExhaustGuid.IsValid());

				// Play Normal Card -> Should route to Discard Pile
				FFCCardTargetInfo TargetInfo;
				DeckComp->Server_PlayCard(NormalGuid, TargetInfo);

				TestEqual(TEXT("Discard count after normal card play must be 1"), DeckComp->GetDiscardPileCount(), 1);
				TestEqual(TEXT("Exhaust count after normal card play must be 0"), DeckComp->GetExhaustPileCount(), 0);
				TestEqual(TEXT("Hand size after 1 play must be 1"), DeckComp->GetHandContainer().Num(), 1);
				TestTrue(TEXT("ServerDiscardPile should contain Normal Card"), DeckComp->GetServerDiscardPile().Contains(FName("Card_NormalTest")));

				// Play Exhaust Card -> Should route to Exhaust Pile
				DeckComp->Server_PlayCard(ExhaustGuid, TargetInfo);

				TestEqual(TEXT("Discard count after exhaust card play must remain 1"), DeckComp->GetDiscardPileCount(), 1);
				TestEqual(TEXT("Exhaust count after exhaust card play must be 1"), DeckComp->GetExhaustPileCount(), 1);
				TestEqual(TEXT("Hand size after 2 plays must be 0"), DeckComp->GetHandContainer().Num(), 0);
				TestTrue(TEXT("ServerExhaustPile should contain Exhaust Card"), DeckComp->GetServerExhaustPile().Contains(FName("Card_ExhaustTest")));

				// =========================================================================
				// Test 3: Direct Hand Exhaust via ExhaustCard API
				// =========================================================================
				DeckComp->AddCardToDeck(FName("Card_NormalTest"), EFCCardAddDestination::Hand);
				TestEqual(TEXT("Hand size after AddCardToDeck must be 1"), DeckComp->GetHandContainer().Num(), 1);

				const FGuid DirectExhaustGuid = DeckComp->GetHandContainer().Items[0].CardGuid;
				bool bExhausted = DeckComp->ExhaustCard(DirectExhaustGuid);
				TestTrue(TEXT("ExhaustCard must return true"), bExhausted);
				TestEqual(TEXT("Hand size after direct exhaust must be 0"), DeckComp->GetHandContainer().Num(), 0);
				TestEqual(TEXT("Exhaust count after direct exhaust must be 2"), DeckComp->GetExhaustPileCount(), 2);

				// =========================================================================
				// Test 4: Dynamic Add to Exhaust Pile & Retrieval
				// =========================================================================
				DeckComp->AddCardToDeck(FName("Card_ExhaustTest"), EFCCardAddDestination::ExhaustPile);
				TestEqual(TEXT("Exhaust count after AddCardToDeck(ExhaustPile) must be 3"), DeckComp->GetExhaustPileCount(), 3);

				// Retrieve card from exhaust pile into hand
				bool bRetrieved = DeckComp->RetrieveCardFromExhaust(FName("Card_ExhaustTest"), EFCCardAddDestination::Hand);
				TestTrue(TEXT("RetrieveCardFromExhaust must return true"), bRetrieved);
				TestEqual(TEXT("Exhaust count after retrieve must be 2"), DeckComp->GetExhaustPileCount(), 2);
				TestEqual(TEXT("Hand size after retrieve must be 1"), DeckComp->GetHandContainer().Num(), 1);

				// Retrieve non-existent card from exhaust pile
				bool bFailRetrieve = DeckComp->RetrieveCardFromExhaust(FName("NonExistentCard"), EFCCardAddDestination::Hand);
				TestFalse(TEXT("RetrieveCardFromExhaust for missing card must return false"), bFailRetrieve);

				// =========================================================================
				// Test 5: Deck Persistence with Exhaust Pile
				// =========================================================================
				FFCCardDeckSaveData Snapshot = DeckComp->ExportDeckSaveData();
				TestEqual(TEXT("Snapshot exhaust count should be 2"), Snapshot.ExhaustPile.Num(), 2);

				AActor* RestoredActor = NewObject<AActor>();
				UFCCardDeckComponent* RestoredDeck = NewObject<UFCCardDeckComponent>(RestoredActor);
				RestoredDeck->RestoreFromDeckSaveData(Snapshot);

				TestEqual(TEXT("Restored deck exhaust count must be 2"), RestoredDeck->GetExhaustPileCount(), 2);
				TestEqual(TEXT("Restored deck discard count must be 1"), RestoredDeck->GetDiscardPileCount(), 1);
				TestEqual(TEXT("Restored deck hand count must be 1"), RestoredDeck->GetHandContainer().Num(), 1);
			}
		}
	}

	// =========================================================================
	// Test 6: UFCCardViewModel Presentation & Zero-Tick FieldNotify Binding
	// =========================================================================
	{
		UFCCardViewModel* CardVM = NewObject<UFCCardViewModel>();
		TestNotNull(TEXT("CardVM should be created"), CardVM);

		if (CardVM)
		{
			UFCCardDataAsset* ExhaustAsset = NewObject<UFCCardDataAsset>();
			ExhaustAsset->GameplayData.CardId = FName("Card_ExhaustTest");
			ExhaustAsset->GameplayData.bExhaustsOnPlay = true;
			ExhaustAsset->GameplayData.Keywords = { EFCCardKeyword::Exhaust };
			ExhaustAsset->DisplayData.CardName = FText::FromString(TEXT("소멸의 일격"));

			FFCCardItem Item(FGuid::NewGuid(), FName("Card_ExhaustTest"), 0, false);
			CardVM->InitializeFromCardItem(Item, ExhaustAsset);

			TestTrue(TEXT("ViewModel bExhaustsOnPlay must be true"), CardVM->bExhaustsOnPlay);
			TestTrue(TEXT("ViewModel bHasKeywords must be true"), CardVM->bHasKeywords);
			TestEqual(TEXT("ViewModel FormattedKeywords must be '소멸'"), CardVM->FormattedKeywords.ToString(), FString(TEXT("소멸")));

			// Test non-exhaust card
			UFCCardDataAsset* NormalAsset = NewObject<UFCCardDataAsset>();
			NormalAsset->GameplayData.CardId = FName("Card_NormalTest");
			NormalAsset->GameplayData.bExhaustsOnPlay = false;

			FFCCardItem NormalItem(FGuid::NewGuid(), FName("Card_NormalTest"), 0, false);
			CardVM->InitializeFromCardItem(NormalItem, NormalAsset);

			TestFalse(TEXT("Normal ViewModel bExhaustsOnPlay must be false"), CardVM->bExhaustsOnPlay);
			TestFalse(TEXT("Normal ViewModel bHasKeywords must be false"), CardVM->bHasKeywords);
			TestTrue(TEXT("Normal ViewModel FormattedKeywords must be empty"), CardVM->FormattedKeywords.IsEmpty());
		}
	}

	return true;
}

#endif
