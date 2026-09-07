#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Gameplay/FCChestActor.h"
#include "Gameplay/FCCardPickupActor.h"
#include "Card/FCCardDeckComponent.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardTypes.h"
#include "UI/ViewModel/FCCardRewardViewModel.h"
#include "UI/ViewModel/FCCardViewModel.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCChestAndDropTest, "FC.Gameplay.ChestAndDrop", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCChestAndDropTest::RunTest(const FString& Parameters)
{
	// Test 1: AFCChestActor Defaults Verification
	AFCChestActor* ChestCDO = GetMutableDefault<AFCChestActor>();
	TestNotNull(TEXT("AFCChestActor CDO should exist"), ChestCDO);
	if (ChestCDO)
	{
		TestEqual(TEXT("Chest damage threshold should be 1.0"), ChestCDO->GetDamageThreshold(), 1.0f);
		TestEqual(TEXT("Chest default destroy delay should be 0.1"), ChestCDO->GetDestroyDelay(), 0.1f);
		TestFalse(TEXT("Chest should not be opened by default"), ChestCDO->IsOpened());
	}

	// Test 2: AddCardToDeck Dynamic Addition into DeckComponent
	UGameInstance* DummyGameInstance = NewObject<UGameInstance>();
	TestNotNull(TEXT("GameInstance should be valid"), DummyGameInstance);

	if (DummyGameInstance)
	{
		UFCCardSubsystem* Subsystem = NewObject<UFCCardSubsystem>(DummyGameInstance);
		TestNotNull(TEXT("Card Subsystem should be instantiable"), Subsystem);

		if (Subsystem)
		{
			UFCCardDataAsset* NewFireball = NewObject<UFCCardDataAsset>(Subsystem);
			NewFireball->GameplayData.CardId = FName("Card_Fireball");
			NewFireball->DisplayData.CardName = FText::FromString(TEXT("파이어 볼"));
			Subsystem->RegisterCardDataAsset(NewFireball);

			// Ensure Card_Fireball is resolvable
			UFCCardDataAsset* FireballAsset = Subsystem->GetCardDataAsset(FName("Card_Fireball"));
			TestNotNull(TEXT("Card_Fireball should resolve"), FireballAsset);

			// Test UFCCardRewardViewModel
			UFCCardRewardViewModel* RewardVM = NewObject<UFCCardRewardViewModel>();
			TestNotNull(TEXT("Reward ViewModel should be instantiable"), RewardVM);

			if (RewardVM)
			{
				RewardVM->SetupReward(FName("Card_Fireball"), Subsystem, EKeys::SpaceBar, EKeys::Escape);
				TestTrue(TEXT("Reward VM should be visible after setup"), RewardVM->bIsVisible);
				TestNotNull(TEXT("Reward VM should have child CardViewModel"), RewardVM->CardViewModel.Get());
				if (RewardVM->CardViewModel)
				{
					TestEqual(TEXT("Reward Card ID should be Card_Fireball"), RewardVM->CardViewModel->CardId, FName("Card_Fireball"));
				}
				TestTrue(TEXT("AcquireActionText should contain Space"), RewardVM->AcquireActionText.ToString().Contains(TEXT("Space")));
				TestTrue(TEXT("DiscardActionText should contain Escape"), RewardVM->DiscardActionText.ToString().Contains(TEXT("Escape")));
			}
		}
	}

	// Test 3: AFCCardPickupActor CDO Defaults
	AFCCardPickupActor* PickupCDO = GetMutableDefault<AFCCardPickupActor>();
	TestNotNull(TEXT("AFCCardPickupActor CDO should exist"), PickupCDO);
	if (PickupCDO)
	{
		TestTrue(TEXT("Card pickup must replicate"), PickupCDO->GetIsReplicated());
	}

	// =========================================================================
	// Test 4: AFCChestActor Drop Filtering by Opener Class & Settings
	// =========================================================================
	{
		UGameInstance* GI = NewObject<UGameInstance>();
		if (GI)
		{
			UFCCardSubsystem* Subsystem = NewObject<UFCCardSubsystem>(GI);
			if (Subsystem)
			{
				// Ensure catalog is populated
				UFCCardDataAsset* FireballCard = NewObject<UFCCardDataAsset>(Subsystem);
				FireballCard->GameplayData.CardId = FName("Card_Fireball");
				FireballCard->GameplayData.RequiredClass = EFCCharacterClass::Mage;
				Subsystem->RegisterCardDataAsset(FireballCard);

				UFCCardDataAsset* BuffCard = NewObject<UFCCardDataAsset>(Subsystem);
				BuffCard->GameplayData.CardId = FName("Card_AttackBuff");
				BuffCard->GameplayData.RequiredClass = EFCCharacterClass::Neutral;
				Subsystem->RegisterCardDataAsset(BuffCard);

				// Register dummy Warrior and Rogue cards for per-class testing
				UFCCardDataAsset* WarriorCard = NewObject<UFCCardDataAsset>(Subsystem);
				WarriorCard->GameplayData.CardId = FName("Card_WarriorSlash");
				WarriorCard->GameplayData.RequiredClass = EFCCharacterClass::Warrior;
				Subsystem->RegisterCardDataAsset(WarriorCard);

				UFCCardDataAsset* RogueCard = NewObject<UFCCardDataAsset>(Subsystem);
				RogueCard->GameplayData.CardId = FName("Card_RogueStab");
				RogueCard->GameplayData.RequiredClass = EFCCharacterClass::Rogue;
				Subsystem->RegisterCardDataAsset(RogueCard);

				AFCChestActor* TestChest = NewObject<AFCChestActor>(GI);
				TestNotNull(TEXT("TestChest should instantiate"), TestChest);

				if (TestChest)
				{
					TestChest->RewardCardIds = {
						FName("Card_Fireball"),     // Mage
						FName("Card_AttackBuff"),   // Neutral
						FName("Card_WarriorSlash"), // Warrior
						FName("Card_RogueStab")     // Rogue
					};

					// Case 4.1: Default Settings (Mage Opener -> Only Mage cards, Neutral & Others excluded)
					TestChest->bAllowOpenerClass = true;
					TestChest->bAllowNeutralCards = false;
					TestChest->AllowedOtherClasses.Empty();
					TestChest->bAllowAllOtherClasses = false;

					TArray<FName> MageOnlyDrops = TestChest->GetFilteredRewardCardIds(EFCCharacterClass::Mage, Subsystem);
					TestEqual(TEXT("Mage default drop count should be 1"), MageOnlyDrops.Num(), 1);
					TestTrue(TEXT("Mage default drop contains Fireball"), MageOnlyDrops.Contains(FName("Card_Fireball")));
					TestFalse(TEXT("Mage default drop does not contain AttackBuff"), MageOnlyDrops.Contains(FName("Card_AttackBuff")));
					TestFalse(TEXT("Mage default drop does not contain WarriorSlash"), MageOnlyDrops.Contains(FName("Card_WarriorSlash")));

					// Case 4.2: Allow Neutral Cards
					TestChest->bAllowNeutralCards = true;
					TArray<FName> MageAndNeutralDrops = TestChest->GetFilteredRewardCardIds(EFCCharacterClass::Mage, Subsystem);
					TestEqual(TEXT("Mage with neutral drop count should be 2"), MageAndNeutralDrops.Num(), 2);
					TestTrue(TEXT("Contains Fireball"), MageAndNeutralDrops.Contains(FName("Card_Fireball")));
					TestTrue(TEXT("Contains AttackBuff"), MageAndNeutralDrops.Contains(FName("Card_AttackBuff")));

					// Case 4.3: Per-Class Granular Control (Allow Warrior only)
					TestChest->bAllowNeutralCards = false;
					TestChest->AllowedOtherClasses.Add(EFCCharacterClass::Warrior);

					TArray<FName> MageAndWarriorDrops = TestChest->GetFilteredRewardCardIds(EFCCharacterClass::Mage, Subsystem);
					TestEqual(TEXT("Mage + Warrior drop count should be 2"), MageAndWarriorDrops.Num(), 2);
					TestTrue(TEXT("Contains Fireball"), MageAndWarriorDrops.Contains(FName("Card_Fireball")));
					TestTrue(TEXT("Contains WarriorSlash"), MageAndWarriorDrops.Contains(FName("Card_WarriorSlash")));
					TestFalse(TEXT("Does not contain RogueStab"), MageAndWarriorDrops.Contains(FName("Card_RogueStab")));

					// Case 4.4: Allow All Other Classes
					TestChest->bAllowAllOtherClasses = true;
					TArray<FName> AllClassDrops = TestChest->GetFilteredRewardCardIds(EFCCharacterClass::Mage, Subsystem);
					TestEqual(TEXT("All classes drop count (excluding neutral) should be 3"), AllClassDrops.Num(), 3);
					TestTrue(TEXT("Contains RogueStab"), AllClassDrops.Contains(FName("Card_RogueStab")));
				}
			}
		}
	}

	return true;
}

#endif
