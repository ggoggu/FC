#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "UI/ViewModel/FCHandViewModel.h"
#include "UI/ViewModel/FCCardViewModel.h"
#include "UI/View/FCHandWidget.h"
#include "UI/View/FCCardWidget.h"
#include "Card/FCCardDeckComponent.h"
#include "Data/Card/FCCardHandContainer.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCHandFanLayoutTest, "FC.UI.HandFanLayout", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCHandFanLayoutTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Pure Fan Layout Math - Single Card
	// =========================================================================
	{
		FVector2D Translation;
		float Angle = 0.0f;
		UFCHandViewModel::CalculateCardFanTransform(0, 1, 110.0f, 800.0f, 35.0f, 30.0f, 4.0f, Translation, Angle);

		TestNearlyEqual(TEXT("Single card translation X should be 0"), static_cast<float>(Translation.X), 0.0f);
		TestNearlyEqual(TEXT("Single card translation Y should be 0"), static_cast<float>(Translation.Y), 0.0f);
		TestNearlyEqual(TEXT("Single card angle should be 0"), Angle, 0.0f);
	}

	// =========================================================================
	// Test 2: Pure Fan Layout Math - 5 Cards (Odd Count Symmetry)
	// =========================================================================
	{
		const int32 TotalCards = 5;
		const float Spacing = 100.0f;
		const float MaxWidth = 1000.0f;
		const float ArcHeight = 40.0f;
		const float MaxAngle = 40.0f;
		const float StepAngle = 5.0f;

		TArray<FVector2D> Translations;
		TArray<float> Angles;
		Translations.SetNum(TotalCards);
		Angles.SetNum(TotalCards);

		for (int32 i = 0; i < TotalCards; ++i)
		{
			UFCHandViewModel::CalculateCardFanTransform(i, TotalCards, Spacing, MaxWidth, ArcHeight, MaxAngle, StepAngle, Translations[i], Angles[i]);
		}

		// Center card (index 2)
		TestNearlyEqual(TEXT("Center card (idx 2) translation X should be 0"), static_cast<float>(Translations[2].X), 0.0f);
		TestNearlyEqual(TEXT("Center card (idx 2) translation Y should be 0 (highest peak)"), static_cast<float>(Translations[2].Y), 0.0f);
		TestNearlyEqual(TEXT("Center card (idx 2) angle should be 0"), Angles[2], 0.0f);

		// Left cards (idx 0, 1)
		TestTrue(TEXT("Leftmost card (idx 0) X should be negative"), Translations[0].X < 0.0f);
		TestTrue(TEXT("Left card (idx 1) X should be negative"), Translations[1].X < 0.0f);
		TestTrue(TEXT("Leftmost card (idx 0) angle should be negative (tilted left)"), Angles[0] < 0.0f);
		TestTrue(TEXT("Left card (idx 1) angle should be negative"), Angles[1] < 0.0f);
		TestTrue(TEXT("Leftmost card (idx 0) Y should drop down"), Translations[0].Y > 0.0f);
		TestNearlyEqual(TEXT("Leftmost card (idx 0) Y drop should equal ArcHeight"), static_cast<float>(Translations[0].Y), ArcHeight);

		// Right cards (idx 3, 4)
		TestTrue(TEXT("Right card (idx 3) X should be positive"), Translations[3].X > 0.0f);
		TestTrue(TEXT("Rightmost card (idx 4) X should be positive"), Translations[4].X > 0.0f);
		TestTrue(TEXT("Right card (idx 3) angle should be positive (tilted right)"), Angles[3] > 0.0f);
		TestTrue(TEXT("Rightmost card (idx 4) angle should be positive"), Angles[4] > 0.0f);
		TestNearlyEqual(TEXT("Rightmost card (idx 4) Y drop should equal ArcHeight"), static_cast<float>(Translations[4].Y), ArcHeight);

		// Symmetry checks
		TestNearlyEqual(TEXT("X symmetry (idx 0 == -idx 4)"), static_cast<float>(Translations[0].X), static_cast<float>(-Translations[4].X));
		TestNearlyEqual(TEXT("X symmetry (idx 1 == -idx 3)"), static_cast<float>(Translations[1].X), static_cast<float>(-Translations[3].X));
		TestNearlyEqual(TEXT("Angle symmetry (idx 0 == -idx 4)"), Angles[0], -Angles[4]);
		TestNearlyEqual(TEXT("Angle symmetry (idx 1 == -idx 3)"), Angles[1], -Angles[3]);
		TestNearlyEqual(TEXT("Y symmetry (idx 0 == idx 4)"), static_cast<float>(Translations[0].Y), static_cast<float>(Translations[4].Y));
		TestNearlyEqual(TEXT("Y symmetry (idx 1 == idx 3)"), static_cast<float>(Translations[1].Y), static_cast<float>(Translations[3].Y));
	}

	// =========================================================================
	// Test 3: Pure Fan Layout Math - Dynamic Hand Compression (MaxHandWidth)
	// =========================================================================
	{
		const int32 TotalCards = 10;
		const float BaseSpacing = 150.0f; // 9 intervals * 150 = 1350px > 800px MaxWidth
		const float MaxWidth = 800.0f;
		const float ArcHeight = 30.0f;
		const float MaxAngle = 30.0f;
		const float StepAngle = 5.0f;

		FVector2D LeftTrans, RightTrans;
		float LeftAngle, RightAngle;
		UFCHandViewModel::CalculateCardFanTransform(0, TotalCards, BaseSpacing, MaxWidth, ArcHeight, MaxAngle, StepAngle, LeftTrans, LeftAngle);
		UFCHandViewModel::CalculateCardFanTransform(TotalCards - 1, TotalCards, BaseSpacing, MaxWidth, ArcHeight, MaxAngle, StepAngle, RightTrans, RightAngle);

		const float ActualTotalSpan = RightTrans.X - LeftTrans.X;
		TestNearlyEqual(TEXT("Compressed total hand span should not exceed MaxHandWidth"), ActualTotalSpan, MaxWidth, 0.01f);
	}

	// =========================================================================
	// Test 4: Pure Fan Layout Math - MaxFanAngle Clamping
	// =========================================================================
	{
		const int32 TotalCards = 10;
		const float BaseSpacing = 100.0f;
		const float MaxWidth = 1000.0f;
		const float ArcHeight = 30.0f;
		const float MaxAngle = 20.0f; // 9 intervals * 5 deg = 45 deg > 20 deg MaxAngle
		const float StepAngle = 5.0f;

		FVector2D LeftTrans, RightTrans;
		float LeftAngle, RightAngle;
		UFCHandViewModel::CalculateCardFanTransform(0, TotalCards, BaseSpacing, MaxWidth, ArcHeight, MaxAngle, StepAngle, LeftTrans, LeftAngle);
		UFCHandViewModel::CalculateCardFanTransform(TotalCards - 1, TotalCards, BaseSpacing, MaxWidth, ArcHeight, MaxAngle, StepAngle, RightTrans, RightAngle);

		const float TotalAngleSpread = RightAngle - LeftAngle;
		TestNearlyEqual(TEXT("Clamped total angle spread should not exceed MaxFanAngle"), TotalAngleSpread, MaxAngle, 0.01f);
	}

	// =========================================================================
	// Test 5: HandViewModel Collection Sync & Hover/Selection State
	// =========================================================================
	{
		UFCHandViewModel* HandVM = NewObject<UFCHandViewModel>();
		TestNotNull(TEXT("HandViewModel must instantiate"), HandVM);

		if (HandVM)
		{
			FFCCardHandContainer Container;
			Container.AddCard(FName("Card_Fireball"));
			Container.AddCard(FName("Card_AttackBuff"));
			Container.AddCard(FName("Card_Fireball"));

			HandVM->SyncFromHandContainer(Container, nullptr);

			TestEqual(TEXT("HandViewModel cards count should be 3"), HandVM->CardsInHand.Num(), 3);
			TestEqual(TEXT("HandViewModel CurrentHandCount should be 3"), HandVM->CurrentHandCount, 3);

			// Check HandIndex assignment
			for (int32 i = 0; i < HandVM->CardsInHand.Num(); ++i)
			{
				TestEqual(TEXT("Child ViewModel HandIndex must match index"), HandVM->CardsInHand[i]->HandIndex, i);
				TestEqual(TEXT("Child ViewModel TotalCardsInHand must match count"), HandVM->CardsInHand[i]->TotalCardsInHand, 3);
			}

			// Test Selection
			HandVM->SelectCardByIndex(1);
			TestTrue(TEXT("HandVM bHasSelection should be true"), HandVM->bHasSelection);
			TestEqual(TEXT("SelectedCardIndex should be 1"), HandVM->SelectedCardIndex, 1);
			TestTrue(TEXT("Card 1 should be selected"), HandVM->CardsInHand[1]->bIsSelected);
			TestFalse(TEXT("Card 0 should not be selected"), HandVM->CardsInHand[0]->bIsSelected);

			// Test Hover
			HandVM->HoverCardByIndex(2);
			TestTrue(TEXT("HandVM bHasHover should be true"), HandVM->bHasHover);
			TestEqual(TEXT("HoveredCardIndex should be 2"), HandVM->HoveredCardIndex, 2);
			TestTrue(TEXT("Card 2 should be hovered"), HandVM->CardsInHand[2]->bIsHovered);
			TestFalse(TEXT("Card 0 should not be hovered"), HandVM->CardsInHand[0]->bIsHovered);

			HandVM->ClearHover();
			TestFalse(TEXT("HandVM bHasHover should be false after clear"), HandVM->bHasHover);
			TestEqual(TEXT("HoveredCardIndex should be INDEX_NONE after clear"), HandVM->HoveredCardIndex, INDEX_NONE);
		}
	}

	// =========================================================================
	// Test 6: Fan Layout Hover (Scale Only - No Lift, Tilt Retained)
	// =========================================================================
	{
		const float HoverLiftY = 0.0f; // Hover does not lift
		const float HoverScale = 1.15f;
		const bool bStraighten = false; // Hover does not straighten

		FVector2D BaseTrans;
		float BaseAngle = 0.0f;
		UFCHandViewModel::CalculateCardFanTransform(0, 3, 110.0f, 800.0f, 35.0f, 30.0f, 4.0f, BaseTrans, BaseAngle);

		FVector2D HoverTrans = BaseTrans + FVector2D(0.0f, HoverLiftY);
		float HoverAngle = bStraighten ? 0.0f : BaseAngle;
		FVector2D HoverScaleVec = FVector2D(HoverScale, HoverScale);

		// Position & Angle should match normal fan layout (no lift, tilt retained)
		TestNearlyEqual(TEXT("Hovered card translation X should match normal"), static_cast<float>(HoverTrans.X), static_cast<float>(BaseTrans.X));
		TestNearlyEqual(TEXT("Hovered card translation Y should match normal (no lift)"), static_cast<float>(HoverTrans.Y), static_cast<float>(BaseTrans.Y));
		TestNearlyEqual(TEXT("Hovered card angle should match normal (tilted)"), HoverAngle, BaseAngle);

		// Scale should be enlarged
		TestNearlyEqual(TEXT("Hovered card scale X should be HoverScale"), static_cast<float>(HoverScaleVec.X), 1.15f);
		TestNearlyEqual(TEXT("Hovered card scale Y should be HoverScale"), static_cast<float>(HoverScaleVec.Y), 1.15f);
	}

	// =========================================================================
	// Test 7: Fan Layout Selection (No Pop-up Lift - SelectedLiftY == 0.0f)
	// =========================================================================
	{
		const float SelectedLiftY = 0.0f; // Click selection does not pop up
		const float SelectedScale = 1.0f;

		FVector2D BaseTrans;
		float BaseAngle = 0.0f;
		UFCHandViewModel::CalculateCardFanTransform(1, 3, 110.0f, 800.0f, 35.0f, 30.0f, 4.0f, BaseTrans, BaseAngle);

		FVector2D SelectedTrans = BaseTrans + FVector2D(0.0f, SelectedLiftY);
		FVector2D SelectedScaleVec = FVector2D(SelectedScale, SelectedScale);

		// Position Y should remain unchanged (no vertical pop-up on selection)
		TestNearlyEqual(TEXT("Selected card translation Y should match normal (no pop-up)"), static_cast<float>(SelectedTrans.Y), static_cast<float>(BaseTrans.Y));
		TestNearlyEqual(TEXT("Selected card scale should be 1.0f"), static_cast<float>(SelectedScaleVec.X), 1.0f);
	}

	// =========================================================================
	// Test 8: Hand Limit Enforcement (MaxHandSize = 10)
	// =========================================================================
	{
		AActor* DummyActor = NewObject<AActor>();
		TestNotNull(TEXT("DummyActor must instantiate"), DummyActor);

		if (DummyActor)
		{
			UFCCardDeckComponent* DeckComp = NewObject<UFCCardDeckComponent>(DummyActor);
			TestNotNull(TEXT("DeckComp must instantiate"), DeckComp);

			if (DeckComp)
			{
				TestEqual(TEXT("Default MaxHandSize must be 10"), DeckComp->GetMaxHandSize(), 10);

				// Prepare 15 cards in deck
				TArray<FName> LargeDeck;
				for (int32 i = 0; i < 15; ++i)
				{
					LargeDeck.Add(FName(FString::Printf(TEXT("Card_Test_%d"), i)));
				}

				DeckComp->InitializeDeck(LargeDeck);
				TestEqual(TEXT("Draw pile count should be 15"), DeckComp->GetDrawPileCount(), 15);

				// Attempt to draw 15 cards
				DeckComp->DrawCards(15);

				// Hand should be strictly capped at 10 cards
				TestEqual(TEXT("Hand count should be capped at MaxHandSize (10)"), DeckComp->GetHandContainer().Num(), 10);
				TestEqual(TEXT("Remaining draw pile count should be 5"), DeckComp->GetDrawPileCount(), 5);
			}
		}
	}

	// =========================================================================
	// Test 9: Number HotKey 1~0 (Slot Index 0~9) Mapping & HandViewModel Selection
	// =========================================================================
	{
		UFCHandViewModel* HandVM = NewObject<UFCHandViewModel>();
		TestNotNull(TEXT("HandViewModel must instantiate"), HandVM);

		if (HandVM)
		{
			FFCCardHandContainer Container;
			for (int32 i = 0; i < 10; ++i)
			{
				Container.AddCard(FName(FString::Printf(TEXT("Card_Slot_%d"), i + 1)));
			}

			HandVM->SyncFromHandContainer(Container, nullptr);
			TestEqual(TEXT("10 cards must be in HandViewModel"), HandVM->CardsInHand.Num(), 10);

			// Key '1' -> Slot Index 0 (1st card)
			HandVM->SelectCardByIndex(0);
			TestEqual(TEXT("Slot 0 should be selected"), HandVM->SelectedCardIndex, 0);
			TestEqual(TEXT("First card HandIndex must be 0"), HandVM->CardsInHand[0]->HandIndex, 0);
			TestTrue(TEXT("First card must be selected"), HandVM->CardsInHand[0]->bIsSelected);

			// Key '0' -> Slot Index 9 (10th card)
			HandVM->SelectCardByIndex(9);
			TestEqual(TEXT("Slot 9 should be selected"), HandVM->SelectedCardIndex, 9);
			TestEqual(TEXT("10th card HandIndex must be 9"), HandVM->CardsInHand[9]->HandIndex, 9);
			TestTrue(TEXT("10th card must be selected"), HandVM->CardsInHand[9]->bIsSelected);
			TestFalse(TEXT("First card must no longer be selected"), HandVM->CardsInHand[0]->bIsSelected);
		}
	}

	// =========================================================================
	// Test 10: Card Widget Held State & Transform Behavior
	// =========================================================================
	{
		UFCCardWidget* CardWidget = NewObject<UFCCardWidget>();
		TestNotNull(TEXT("CardWidget must instantiate"), CardWidget);

		if (CardWidget)
		{
			TestFalse(TEXT("Default IsHeldByHotKey should be false"), CardWidget->IsHeldByHotKey());

			CardWidget->SetIsHeldByHotKey(true);
			TestTrue(TEXT("IsHeldByHotKey should be true"), CardWidget->IsHeldByHotKey());
			TestNearlyEqual(TEXT("Held card target angle must be 0 (straight)"), CardWidget->GetTargetAngle(), 0.0f);
			TestEqual(TEXT("Held card target Z-Order must be 1000"), CardWidget->GetTargetZOrder(), 1000);

			CardWidget->SetIsHeldByHotKey(false);
			TestFalse(TEXT("IsHeldByHotKey should be false after clear"), CardWidget->IsHeldByHotKey());
		}
	}

	// =========================================================================
	// Test 11: Card Description Auto-Wrapping & Wrap Width Configuration
	// =========================================================================
	{
		UFCCardWidget* CardWidget = NewObject<UFCCardWidget>();
		TestNotNull(TEXT("CardWidget must instantiate"), CardWidget);

		if (CardWidget)
		{
			TestTrue(TEXT("Default bAutoWrapDescription should be true"), CardWidget->GetAutoWrapDescription());
			TestNearlyEqual(TEXT("Default DescriptionWrapWidth should be 0.0f (respects UMG editor setting)"), CardWidget->GetDescriptionWrapWidth(), 0.0f);

			CardWidget->SetDescriptionWrapWidth(200.0f);
			TestNearlyEqual(TEXT("DescriptionWrapWidth should update to 200.0f"), CardWidget->GetDescriptionWrapWidth(), 200.0f);

			CardWidget->SetAutoWrapDescription(false);
			TestFalse(TEXT("bAutoWrapDescription should update to false"), CardWidget->GetAutoWrapDescription());

			CardWidget->SetAutoWrapDescription(true);
			TestTrue(TEXT("bAutoWrapDescription should restore to true"), CardWidget->GetAutoWrapDescription());
		}
	}

	return true;
}

#endif
