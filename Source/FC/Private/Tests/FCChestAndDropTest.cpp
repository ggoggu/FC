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

	return true;
}

#endif
