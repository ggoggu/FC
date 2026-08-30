#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Card/FCCardDeckComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Data/FCPlayerPersistenceSubsystem.h"
#include "Data/FCPlayerPersistenceTypes.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCPlayerPersistenceTest, "FC.Gameplay.PlayerPersistence", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCPlayerPersistenceTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: UFCCardDeckComponent Export & Restore Data Integrity
	// =========================================================================
	{
		AActor* SourceActor = NewObject<AActor>();
		TestNotNull(TEXT("SourceActor should be valid"), SourceActor);

		if (SourceActor)
		{
			UFCCardDeckComponent* SourceDeck = NewObject<UFCCardDeckComponent>(SourceActor);
			TestNotNull(TEXT("SourceDeck should be valid"), SourceDeck);

			if (SourceDeck)
			{
				const TArray<FName> StartingDeck = {
					FName("Card_Fireball"),
					FName("Card_Fireball"),
					FName("Card_Fireball"),
					FName("Card_Fireball"),
					FName("Card_Fireball")
				};
				SourceDeck->InitializeDeck(StartingDeck);
				SourceDeck->DrawCards(2);

				// Upgrade the first hand card
				const FFCCardHandContainer& Hand = SourceDeck->GetHandContainer();
				TestEqual(TEXT("Source hand should have 2 cards"), Hand.Num(), 2);

				if (Hand.Num() >= 2)
				{
					const FGuid FirstCardGuid = Hand.Items[0].CardGuid;
					SourceDeck->UpgradeCardInHand(FirstCardGuid, 2);
					SourceDeck->SetCardLockedInHand(FirstCardGuid, true);

					// Discard second card
					const FGuid SecondCardGuid = Hand.Items[1].CardGuid;
					SourceDeck->DiscardCard(SecondCardGuid);

					TestEqual(TEXT("Source hand count after discard should be 1"), SourceDeck->GetHandContainer().Num(), 1);
					TestEqual(TEXT("Source discard count should be 1"), SourceDeck->GetDiscardPileCount(), 1);

					// Export snapshot
					FFCCardDeckSaveData DeckSnapshot = SourceDeck->ExportDeckSaveData();
					TestEqual(TEXT("Snapshot hand count should be 1"), DeckSnapshot.HandCards.Num(), 1);
					TestEqual(TEXT("Snapshot draw count should be 3"), DeckSnapshot.DrawPile.Num(), 3);
					TestEqual(TEXT("Snapshot discard count should be 1"), DeckSnapshot.DiscardPile.Num(), 1);

					// Restore into a brand new DeckComponent (Simulating Level Transition)
					AActor* TargetActor = NewObject<AActor>();
					UFCCardDeckComponent* TargetDeck = NewObject<UFCCardDeckComponent>(TargetActor);
					TargetDeck->RestoreFromDeckSaveData(DeckSnapshot);

					TestEqual(TEXT("Restored hand count should be 1"), TargetDeck->GetHandContainer().Num(), 1);
					TestEqual(TEXT("Restored draw count should be 3"), TargetDeck->GetDrawPileCount(), 3);
					TestEqual(TEXT("Restored discard count should be 1"), TargetDeck->GetDiscardPileCount(), 1);
					TestEqual(TEXT("Restored exhaust count should be 0"), TargetDeck->GetExhaustPileCount(), 0);

					const FFCCardItem* RestoredCard = TargetDeck->GetHandContainer().FindCard(FirstCardGuid);
					TestNotNull(TEXT("Restored card with matching GUID should exist"), RestoredCard);
					if (RestoredCard)
					{
						TestEqual(TEXT("Restored card UpgradeLevel must be 2"), RestoredCard->UpgradeLevel, 2);
						TestTrue(TEXT("Restored card must remain locked"), RestoredCard->bIsLocked);
					}
				}
			}
		}
	}

	// =========================================================================
	// Test 2: UFCAttributeSet Export & Restore Data Integrity
	// =========================================================================
	{
		AActor* SourceActor = NewObject<AActor>();
		UFCAttributeSet* SourceAttrSet = NewObject<UFCAttributeSet>(SourceActor);
		TestNotNull(TEXT("SourceAttrSet should be valid"), SourceAttrSet);

		if (SourceAttrSet)
		{
			FFCPlayerStatSaveData InitialData;
			InitialData.MaxHealth = 150.0f;
			InitialData.Health = 85.0f;
			InitialData.MaxMana = 80.0f;
			InitialData.Mana = 35.0f;
			SourceAttrSet->RestoreFromStatSaveData(InitialData);

			FFCPlayerStatSaveData StatSnapshot = SourceAttrSet->ExportStatSaveData();
			TestEqual(TEXT("Snapshot MaxHealth should be 150"), StatSnapshot.MaxHealth, 150.0f);
			TestEqual(TEXT("Snapshot Health should be 85"), StatSnapshot.Health, 85.0f);
			TestEqual(TEXT("Snapshot MaxMana should be 80"), StatSnapshot.MaxMana, 80.0f);
			TestEqual(TEXT("Snapshot Mana should be 35"), StatSnapshot.Mana, 35.0f);

			// Restore into new AttributeSet
			AActor* TargetActor = NewObject<AActor>();
			UFCAttributeSet* TargetAttrSet = NewObject<UFCAttributeSet>(TargetActor);
			TargetAttrSet->RestoreFromStatSaveData(StatSnapshot);

			TestEqual(TEXT("Restored MaxHealth should match"), TargetAttrSet->GetMaxHealth(), 150.0f);
			TestEqual(TEXT("Restored Health should match"), TargetAttrSet->GetHealth(), 85.0f);
			TestEqual(TEXT("Restored MaxMana should match"), TargetAttrSet->GetMaxMana(), 80.0f);
			TestEqual(TEXT("Restored Mana should match"), TargetAttrSet->GetMana(), 35.0f);
		}
	}

	// =========================================================================
	// Test 3: UFCPlayerPersistenceSubsystem Storage & Lifecycle
	// =========================================================================
	{
		UGameInstance* DummyGI = NewObject<UGameInstance>();
		TestNotNull(TEXT("DummyGI should be valid"), DummyGI);

		if (DummyGI)
		{
			UFCPlayerPersistenceSubsystem* Subsystem = NewObject<UFCPlayerPersistenceSubsystem>(DummyGI);
			TestNotNull(TEXT("Persistence Subsystem should instantiate"), Subsystem);

			if (Subsystem)
			{
				const FString TestPlayerKey = TEXT("Player_Test_999");
				TestFalse(TEXT("Should not have data initially"), Subsystem->HasPlayerData(TestPlayerKey));

				FFCCardDeckSaveData DeckData;
				DeckData.DrawPile = { FName("Card_Fireball"), FName("Card_Fireball") };

				FFCPlayerStatSaveData StatData;
				StatData.Health = 60.0f;
				StatData.MaxHealth = 120.0f;
				StatData.Mana = 40.0f;
				StatData.MaxMana = 60.0f;

				Subsystem->SavePlayerData(TestPlayerKey, DeckData, StatData);
				TestTrue(TEXT("Should have data after save"), Subsystem->HasPlayerData(TestPlayerKey));

				FFCCardDeckSaveData LoadedDeck;
				FFCPlayerStatSaveData LoadedStats;
				bool bLoaded = Subsystem->LoadPlayerData(TestPlayerKey, LoadedDeck, LoadedStats);

				TestTrue(TEXT("LoadPlayerData should succeed"), bLoaded);
				TestEqual(TEXT("Loaded draw count should be 2"), LoadedDeck.DrawPile.Num(), 2);
				TestEqual(TEXT("Loaded Health should be 60"), LoadedStats.Health, 60.0f);
				TestEqual(TEXT("Loaded MaxHealth should be 120"), LoadedStats.MaxHealth, 120.0f);

				// Test Clear
				Subsystem->ClearPlayerData(TestPlayerKey);
				TestFalse(TEXT("Should not have data after clear"), Subsystem->HasPlayerData(TestPlayerKey));
			}
		}
	}

	return true;
}

#endif
