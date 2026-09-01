#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Data/Class/FCClassTypes.h"
#include "Data/Class/FCClassDataAsset.h"
#include "Data/Class/FCClassSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardTypes.h"
#include "Card/FCCardDeckComponent.h"
#include "Game/FCPlayerState.h"
#include "Data/FCPlayerPersistenceSubsystem.h"
#include "Data/FCPlayerPersistenceTypes.h"
#include "UI/ViewModel/FCCardViewModel.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCClassSystemTest, "FC.Gameplay.ClassSystem", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCClassSystemTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: UFCClassSubsystem Catalog Resolution & Class Data Defaults
	// =========================================================================
	{
		UGameInstance* DummyGI = NewObject<UGameInstance>();
		TestNotNull(TEXT("GameInstance should be valid"), DummyGI);

		if (DummyGI)
		{
			UFCClassSubsystem* ClassSubsystem = NewObject<UFCClassSubsystem>(DummyGI);
			TestNotNull(TEXT("ClassSubsystem should be instantiable"), ClassSubsystem);

			if (ClassSubsystem)
			{
				// 1.1 Mage Class Data
				UFCClassDataAsset* MageAsset = ClassSubsystem->GetClassDataAsset(EFCCharacterClass::Mage);
				TestNotNull(TEXT("Mage class asset must exist"), MageAsset);
				if (MageAsset)
				{
					TestEqual(TEXT("Mage class type must match"), (uint8)MageAsset->GetClassType(), (uint8)EFCCharacterClass::Mage);
					TestEqual(TEXT("Mage max health must be 80"), MageAsset->ClassData.BaseMaxHealth, 80.0f);
					TestEqual(TEXT("Mage max mana must be 100"), MageAsset->ClassData.BaseMaxMana, 100.0f);
					TestTrue(TEXT("Mage must have Fire affinity"), MageAsset->ClassData.AffinityElements.Contains(EFCElement::Fire));
					TestTrue(TEXT("Mage must have Earth affinity"), MageAsset->ClassData.AffinityElements.Contains(EFCElement::Earth));
					TestTrue(TEXT("Mage must have Water affinity"), MageAsset->ClassData.AffinityElements.Contains(EFCElement::Water));
					TestTrue(TEXT("Mage starter deck must contain Card_Fireball"), MageAsset->ClassData.StartingDeck.Contains(FName("Card_Fireball")));
					TestTrue(TEXT("Mage starter deck must contain Card_AttackBuff"), MageAsset->ClassData.StartingDeck.Contains(FName("Card_AttackBuff")));
				}

				// 1.2 Neutral Class Data
				UFCClassDataAsset* NeutralAsset = ClassSubsystem->GetClassDataAsset(EFCCharacterClass::Neutral);
				TestNotNull(TEXT("Neutral class asset must exist"), NeutralAsset);
				if (NeutralAsset)
				{
					TestEqual(TEXT("Neutral class type must match"), (uint8)NeutralAsset->GetClassType(), (uint8)EFCCharacterClass::Neutral);
					TestEqual(TEXT("Neutral max health must be 100"), NeutralAsset->ClassData.BaseMaxHealth, 100.0f);
					TestEqual(TEXT("Neutral max mana must be 50"), NeutralAsset->ClassData.BaseMaxMana, 50.0f);
					TestTrue(TEXT("Neutral starter deck must contain Card_AttackBuff"), NeutralAsset->ClassData.StartingDeck.Contains(FName("Card_AttackBuff")));
				}
			}
		}
	}

	// =========================================================================
	// Test 2: FCClassTraitUtils Static Adapter Tests
	// =========================================================================
	{
		// Display names
		TestEqual(TEXT("Mage display name"), FCClassTraitUtils::GetClassDisplayName(EFCCharacterClass::Mage).ToString(), FString(TEXT("마법사")));
		TestEqual(TEXT("Neutral display name"), FCClassTraitUtils::GetClassDisplayName(EFCCharacterClass::Neutral).ToString(), FString(TEXT("중립")));
		TestEqual(TEXT("Fire display name"), FCClassTraitUtils::GetElementDisplayName(EFCElement::Fire).ToString(), FString(TEXT("화염")));
		TestEqual(TEXT("Earth display name"), FCClassTraitUtils::GetElementDisplayName(EFCElement::Earth).ToString(), FString(TEXT("대지")));

		// Trait category names
		TestEqual(TEXT("Mage trait category"), FCClassTraitUtils::GetTraitCategoryName(EFCCharacterClass::Mage).ToString(), FString(TEXT("속성")));
		TestEqual(TEXT("Warrior trait category"), FCClassTraitUtils::GetTraitCategoryName(EFCCharacterClass::Warrior).ToString(), FString(TEXT("태세")));
		TestEqual(TEXT("Neutral trait category"), FCClassTraitUtils::GetTraitCategoryName(EFCCharacterClass::Neutral).ToString(), FString(TEXT("공용")));

		// Multi-element formatting
		const TArray<EFCElement> FireAndEarth = { EFCElement::Fire, EFCElement::Earth };
		TestEqual(TEXT("Fire and Earth formatted text"), FCClassTraitUtils::FormatMageElementsText(FireAndEarth).ToString(), FString(TEXT("화염 / 대지")));

		// Usability check (Neutral usable by all, Mage only by Mage)
		TestTrue(TEXT("Neutral card usable by Mage"), FCClassTraitUtils::CanCardBeUsedByClass(EFCCharacterClass::Neutral, EFCCharacterClass::Mage));
		TestTrue(TEXT("Neutral card usable by Warrior"), FCClassTraitUtils::CanCardBeUsedByClass(EFCCharacterClass::Neutral, EFCCharacterClass::Warrior));
		TestTrue(TEXT("Mage card usable by Mage"), FCClassTraitUtils::CanCardBeUsedByClass(EFCCharacterClass::Mage, EFCCharacterClass::Mage));
		TestFalse(TEXT("Mage card not usable by Warrior"), FCClassTraitUtils::CanCardBeUsedByClass(EFCCharacterClass::Mage, EFCCharacterClass::Warrior));
	}

	// =========================================================================
	// Test 3: UFCCardDeckComponent InitializeDeckForClass
	// =========================================================================
	{
		AActor* SourceActor = NewObject<AActor>();
		TestNotNull(TEXT("SourceActor should be valid"), SourceActor);

		if (SourceActor)
		{
			UFCCardDeckComponent* DeckComp = NewObject<UFCCardDeckComponent>(SourceActor);
			TestNotNull(TEXT("DeckComp should be valid"), DeckComp);

			if (DeckComp)
			{
				DeckComp->InitializeDeckForClass(EFCCharacterClass::Mage);
				TestTrue(TEXT("Mage starter deck should have cards"), DeckComp->GetDrawPileCount() > 0);
			}
		}
	}

	// =========================================================================
	// Test 4: UFCPlayerPersistenceSubsystem CharacterClass Roundtrip
	// =========================================================================
	{
		UGameInstance* DummyGI = NewObject<UGameInstance>();
		TestNotNull(TEXT("DummyGI for persistence should be valid"), DummyGI);

		if (DummyGI)
		{
			UFCPlayerPersistenceSubsystem* Subsystem = NewObject<UFCPlayerPersistenceSubsystem>(DummyGI);
			TestNotNull(TEXT("Persistence Subsystem should instantiate"), Subsystem);

			if (Subsystem)
			{
				const FString TestPlayerKey = TEXT("Player_Class_Test_001");

				FFCCardDeckSaveData DeckData;
				DeckData.DrawPile = { FName("Card_Fireball") };

				FFCPlayerStatSaveData StatData;
				StatData.CharacterClass = EFCCharacterClass::Mage;
				StatData.Health = 75.0f;
				StatData.MaxHealth = 80.0f;
				StatData.Mana = 90.0f;
				StatData.MaxMana = 100.0f;

				Subsystem->SavePlayerData(TestPlayerKey, DeckData, StatData);

				FFCCardDeckSaveData LoadedDeck;
				FFCPlayerStatSaveData LoadedStats;
				bool bLoaded = Subsystem->LoadPlayerData(TestPlayerKey, LoadedDeck, LoadedStats);

				TestTrue(TEXT("Persistence load should succeed"), bLoaded);
				TestEqual(TEXT("Persisted CharacterClass must be Mage"), (uint8)LoadedStats.CharacterClass, (uint8)EFCCharacterClass::Mage);
				TestEqual(TEXT("Persisted Health should match"), LoadedStats.Health, 75.0f);
				TestEqual(TEXT("Persisted MaxMana should match"), LoadedStats.MaxMana, 100.0f);
			}
		}
	}

	// =========================================================================
	// Test 5: UFCCardViewModel Presentation Binding for Class & Traits
	// =========================================================================
	{
		UGameInstance* DummyGI = NewObject<UGameInstance>();
		if (DummyGI)
		{
			UFCCardSubsystem* CardSubsystem = NewObject<UFCCardSubsystem>(DummyGI);
			if (CardSubsystem)
			{
				UFCCardDataAsset* NewFireball = NewObject<UFCCardDataAsset>(CardSubsystem);
				NewFireball->GameplayData.CardId = FName("Card_Fireball");
				NewFireball->GameplayData.RequiredClass = EFCCharacterClass::Mage;
				NewFireball->GameplayData.Elements = { EFCElement::Fire, EFCElement::Earth };
				CardSubsystem->RegisterCardDataAsset(NewFireball);

				UFCCardDataAsset* NewBuff = NewObject<UFCCardDataAsset>(CardSubsystem);
				NewBuff->GameplayData.CardId = FName("Card_AttackBuff");
				NewBuff->GameplayData.RequiredClass = EFCCharacterClass::Neutral;
				CardSubsystem->RegisterCardDataAsset(NewBuff);

				UFCCardDataAsset* FireballAsset = CardSubsystem->GetCardDataAsset(FName("Card_Fireball"));
				UFCCardDataAsset* AttackBuffAsset = CardSubsystem->GetCardDataAsset(FName("Card_AttackBuff"));

				UFCCardViewModel* CardVM = NewObject<UFCCardViewModel>();
				TestNotNull(TEXT("CardVM should instantiate"), CardVM);

				if (CardVM && FireballAsset && AttackBuffAsset)
				{
					// Fireball (Mage, Fire+Earth)
					FFCCardItem FireballItem;
					FireballItem.CardId = FName("Card_Fireball");
					FireballItem.CardGuid = FGuid::NewGuid();
					CardVM->InitializeFromCardItem(FireballItem, FireballAsset);

					TestEqual(TEXT("VM RequiredClass must be Mage"), (uint8)CardVM->RequiredClass, (uint8)EFCCharacterClass::Mage);
					TestFalse(TEXT("Fireball VM is not neutral"), CardVM->bIsNeutral);
					TestTrue(TEXT("Fireball VM has class trait"), CardVM->bHasClassTrait);
					TestEqual(TEXT("Fireball VM trait type"), CardVM->ClassTraitTypeName.ToString(), FString(TEXT("속성")));
					TestEqual(TEXT("Fireball VM trait text"), CardVM->ClassTraitFormattedText.ToString(), FString(TEXT("화염 / 대지")));

					// AttackBuff (Neutral)
					FFCCardItem BuffItem;
					BuffItem.CardId = FName("Card_AttackBuff");
					BuffItem.CardGuid = FGuid::NewGuid();
					CardVM->InitializeFromCardItem(BuffItem, AttackBuffAsset);

					TestEqual(TEXT("VM RequiredClass must be Neutral"), (uint8)CardVM->RequiredClass, (uint8)EFCCharacterClass::Neutral);
					TestTrue(TEXT("Buff VM is neutral"), CardVM->bIsNeutral);
					TestFalse(TEXT("Buff VM has no class trait"), CardVM->bHasClassTrait);
				}
			}
		}
	}

	return true;
}

#endif
