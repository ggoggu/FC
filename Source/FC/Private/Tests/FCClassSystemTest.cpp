#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Data/Class/FCClassTypes.h"
#include "Data/Class/FCClassDataAsset.h"
#include "Data/Class/FCClassSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardTypes.h"
#include "Data/FCGameplayTags.h"
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
				UFCClassDataAsset* MageAsset = ClassSubsystem->GetClassDataAsset(FCGameplayTags::Class_Mage);
				TestNotNull(TEXT("Mage class asset must exist"), MageAsset);
				if (MageAsset)
				{
					TestTrue(TEXT("Mage class tag must match"), MageAsset->GetClassTag() == FCGameplayTags::Class_Mage);
					TestEqual(TEXT("Mage max health must be 80"), MageAsset->ClassData.BaseMaxHealth, 80.0f);
					TestEqual(TEXT("Mage max mana must be 100"), MageAsset->ClassData.BaseMaxMana, 100.0f);
					TestTrue(TEXT("Mage must have Fire affinity"), MageAsset->ClassData.AffinityElementTags.Contains(FCGameplayTags::Element_Fire.GetTag()));
					TestTrue(TEXT("Mage must have Earth affinity"), MageAsset->ClassData.AffinityElementTags.Contains(FCGameplayTags::Element_Earth.GetTag()));
					TestTrue(TEXT("Mage must have Water affinity"), MageAsset->ClassData.AffinityElementTags.Contains(FCGameplayTags::Element_Water.GetTag()));
					TestTrue(TEXT("Mage starter deck must contain Card_Fireball"), MageAsset->ClassData.StartingDeck.Contains(FName("Card_Fireball")));
					TestTrue(TEXT("Mage starter deck must contain Card_AttackBuff"), MageAsset->ClassData.StartingDeck.Contains(FName("Card_AttackBuff")));
				}

				UFCClassDataAsset* NeutralAsset = ClassSubsystem->GetClassDataAsset(FCGameplayTags::Class_Neutral);
				TestNotNull(TEXT("Neutral class asset must exist"), NeutralAsset);
				if (NeutralAsset)
				{
					TestTrue(TEXT("Neutral class tag must match"), NeutralAsset->GetClassTag() == FCGameplayTags::Class_Neutral);
					TestEqual(TEXT("Neutral max health must be 100"), NeutralAsset->ClassData.BaseMaxHealth, 100.0f);
					TestEqual(TEXT("Neutral max mana must be 50"), NeutralAsset->ClassData.BaseMaxMana, 50.0f);
					TestTrue(TEXT("Neutral starter deck must contain Card_AttackBuff"), NeutralAsset->ClassData.StartingDeck.Contains(FName("Card_AttackBuff")));
				}
			}
		}
	}

	// =========================================================================
	// Test 2: FCClassTraitUtils Static Data-Driven Adapter Tests
	// =========================================================================
	{
		TestEqual(TEXT("Mage display name"), FCClassTraitUtils::GetClassDisplayName(FCGameplayTags::Class_Mage).ToString(), FString(TEXT("마법사")));
		TestEqual(TEXT("Neutral display name"), FCClassTraitUtils::GetClassDisplayName(FCGameplayTags::Class_Neutral).ToString(), FString(TEXT("중립")));
		TestEqual(TEXT("Fire display name"), FCClassTraitUtils::GetElementDisplayName(FCGameplayTags::Element_Fire).ToString(), FString(TEXT("화염")));
		TestEqual(TEXT("Earth display name"), FCClassTraitUtils::GetElementDisplayName(FCGameplayTags::Element_Earth).ToString(), FString(TEXT("대지")));

		TestEqual(TEXT("Mage trait category"), FCClassTraitUtils::GetTraitCategoryName(FCGameplayTags::Class_Mage).ToString(), FString(TEXT("속성")));
		TestEqual(TEXT("Warrior trait category"), FCClassTraitUtils::GetTraitCategoryName(FCGameplayTags::Class_Warrior).ToString(), FString(TEXT("태세")));
		TestEqual(TEXT("Neutral trait category"), FCClassTraitUtils::GetTraitCategoryName(FCGameplayTags::Class_Neutral).ToString(), FString(TEXT("공용")));

		const TArray<FGameplayTag> FireAndEarth = { FCGameplayTags::Element_Fire.GetTag(), FCGameplayTags::Element_Earth.GetTag() };
		TestEqual(TEXT("Fire and Earth formatted text"), FCClassTraitUtils::FormatElementTagsText(FireAndEarth).ToString(), FString(TEXT("화염 / 대지")));

		TestTrue(TEXT("Neutral card usable by Mage"), FCClassTraitUtils::CanCardBeUsedByClass(FCGameplayTags::Class_Neutral, FCGameplayTags::Class_Mage));
		TestTrue(TEXT("Neutral card usable by Warrior"), FCClassTraitUtils::CanCardBeUsedByClass(FCGameplayTags::Class_Neutral, FCGameplayTags::Class_Warrior));
		TestTrue(TEXT("Mage card usable by Mage"), FCClassTraitUtils::CanCardBeUsedByClass(FCGameplayTags::Class_Mage, FCGameplayTags::Class_Mage));
		TestFalse(TEXT("Mage card not usable by Warrior"), FCClassTraitUtils::CanCardBeUsedByClass(FCGameplayTags::Class_Mage, FCGameplayTags::Class_Warrior));
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
				DeckComp->InitializeDeckForClass(FCGameplayTags::Class_Mage);
				TestTrue(TEXT("Mage starter deck should have cards"), DeckComp->GetDrawPileCount() > 0);
			}
		}
	}

	// =========================================================================
	// Test 4: UFCPlayerPersistenceSubsystem CharacterClass Tag Roundtrip
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
				StatData.CharacterClass = FCGameplayTags::Class_Mage;
				StatData.Health = 75.0f;
				StatData.MaxHealth = 80.0f;
				StatData.Mana = 90.0f;
				StatData.MaxMana = 100.0f;

				Subsystem->SavePlayerData(TestPlayerKey, DeckData, StatData);

				FFCCardDeckSaveData LoadedDeck;
				FFCPlayerStatSaveData LoadedStats;
				bool bLoaded = Subsystem->LoadPlayerData(TestPlayerKey, LoadedDeck, LoadedStats);

				TestTrue(TEXT("Persistence load should succeed"), bLoaded);
				TestTrue(TEXT("Persisted CharacterClass must be Mage"), LoadedStats.CharacterClass == FCGameplayTags::Class_Mage);
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
				NewFireball->GameplayData.RequiredClassTag = FCGameplayTags::Class_Mage;
				NewFireball->GameplayData.ElementTags = { FCGameplayTags::Element_Fire.GetTag(), FCGameplayTags::Element_Earth.GetTag() };
				CardSubsystem->RegisterCardDataAsset(NewFireball);

				UFCCardDataAsset* NewBuff = NewObject<UFCCardDataAsset>(CardSubsystem);
				NewBuff->GameplayData.CardId = FName("Card_AttackBuff");
				NewBuff->GameplayData.RequiredClassTag = FCGameplayTags::Class_Neutral;
				CardSubsystem->RegisterCardDataAsset(NewBuff);

				UFCCardDataAsset* FireballAsset = CardSubsystem->GetCardDataAsset(FName("Card_Fireball"));
				UFCCardDataAsset* AttackBuffAsset = CardSubsystem->GetCardDataAsset(FName("Card_AttackBuff"));

				UFCCardViewModel* CardVM = NewObject<UFCCardViewModel>();
				TestNotNull(TEXT("CardVM should instantiate"), CardVM);

				if (CardVM && FireballAsset && AttackBuffAsset)
				{
					FFCCardItem FireballItem;
					FireballItem.CardId = FName("Card_Fireball");
					FireballItem.CardGuid = FGuid::NewGuid();
					CardVM->InitializeFromCardItem(FireballItem, FireballAsset);

					TestTrue(TEXT("VM RequiredClassTag must be Mage"), CardVM->RequiredClassTag == FCGameplayTags::Class_Mage);
					TestFalse(TEXT("Fireball VM is not neutral"), CardVM->bIsNeutral);
					TestTrue(TEXT("Fireball VM has class trait"), CardVM->bHasClassTrait);
					TestEqual(TEXT("Fireball VM trait type"), CardVM->ClassTraitTypeName.ToString(), FString(TEXT("속성")));
					TestEqual(TEXT("Fireball VM trait text"), CardVM->ClassTraitFormattedText.ToString(), FString(TEXT("화염 / 대지")));

					FFCCardItem BuffItem;
					BuffItem.CardId = FName("Card_AttackBuff");
					BuffItem.CardGuid = FGuid::NewGuid();
					CardVM->InitializeFromCardItem(BuffItem, AttackBuffAsset);

					TestTrue(TEXT("VM RequiredClassTag must be Neutral"), CardVM->RequiredClassTag == FCGameplayTags::Class_Neutral);
					TestTrue(TEXT("Buff VM is neutral"), CardVM->bIsNeutral);
					TestFalse(TEXT("Buff VM has no class trait"), CardVM->bHasClassTrait);
				}
			}
		}
	}

	return true;
}

#endif
