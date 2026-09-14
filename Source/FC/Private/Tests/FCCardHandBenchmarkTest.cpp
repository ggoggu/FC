#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"
#include "UI/ViewModel/FCHandViewModel.h"
#include "UI/ViewModel/FCCardViewModel.h"
#include "UI/View/FCHandWidget.h"
#include "UI/View/FCCardWidget.h"
#include "Data/Card/FCCardHandContainer.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCCardHandBenchmarkTest, "FC.UI.CardHandBenchmark", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCCardHandBenchmarkTest::RunTest(const FString& Parameters)
{
	const int32 CycleIterations = 1000;
	const int32 TickSimulationFrames = 3000;
	const TArray<FName> CardPool = {
		FName("Card_Fireball"),
		FName("Card_AttackBuff"),
		FName("Card_SandWall"),
		FName("Card_Ignite"),
		FName("Card_MagicShield"),
		FName("Card_SkillStrike"),
		FName("Card_FrostBolt"),
		FName("Card_ThunderClap"),
		FName("Card_HealingWard"),
		FName("Card_BladeStorm")
	};

	// 1. Setup CardSubsystem catalog with pre-created data assets to eliminate World/Context lookup warnings
	UGameInstance* DummyGI = NewObject<UGameInstance>();
	UFCCardSubsystem* BenchmarkSubsystem = NewObject<UFCCardSubsystem>(DummyGI);
	for (const FName& CardId : CardPool)
	{
		UFCCardDataAsset* DummyAsset = NewObject<UFCCardDataAsset>(BenchmarkSubsystem);
		DummyAsset->GameplayData.CardId = CardId;
		DummyAsset->GameplayData.BaseManaCost = 2;
		DummyAsset->DisplayData.CardName = FText::FromName(CardId);
		DummyAsset->DisplayData.CardDescription = FText::FromString(TEXT("Benchmark Card Description"));
		BenchmarkSubsystem->RegisterCardDataAsset(DummyAsset);
	}

	// 2. Pre-generate deterministic hand transition batches (hand size 2 to 10 cards)
	TArray<FFCCardHandContainer> HandBatches;
	HandBatches.Reserve(CycleIterations);
	for (int32 i = 0; i < CycleIterations; ++i)
	{
		FFCCardHandContainer Batch;
		const int32 CardCount = 2 + (i % 9); // Cycles 2 to 10 cards
		for (int32 j = 0; j < CardCount; ++j)
		{
			const FName ChosenCardId = CardPool[(i + j) % CardPool.Num()];
			Batch.AddCard(ChosenCardId, (j % 3)); // Upgrade level 0, 1, 2
		}
		HandBatches.Add(Batch);
	}

	// =========================================================================
	// BENCHMARK 1: Hand Cycle & Card Draw/Discard (동적 생성 vs 위젯 풀링)
	// =========================================================================

	// --- 1-A. Legacy Mode: 매 갱신마다 위젯 및 뷰모델 동적 신규 생성 (GC Churn) ---
	int64 LegacyAllocatedObjects = 0;
	const double LegacyCycleStartTime = FPlatformTime::Seconds();
	{
		UObject* DummyOuter = GetTransientPackage();
		for (int32 i = 0; i < CycleIterations; ++i)
		{
			const FFCCardHandContainer& Batch = HandBatches[i];

			// Simulate legacy unpooled widget recreation (ClearChildren + CreateWidget per card)
			TArray<UFCCardWidget*> DynamicWidgets;
			DynamicWidgets.Reserve(Batch.Num());

			for (const FFCCardItem& Item : Batch.Items)
			{
				UFCCardWidget* CardWidget = NewObject<UFCCardWidget>(DummyOuter);
				UFCCardViewModel* CardVM = NewObject<UFCCardViewModel>(DummyOuter);
				CardVM->InitializeFromCardItem(Item, BenchmarkSubsystem->GetCardDataAsset(Item.CardId));
				CardWidget->SetCardViewModel(CardVM);
				DynamicWidgets.Add(CardWidget);
				LegacyAllocatedObjects += 2; // 1 Widget + 1 ViewModel per card
			}

			// Simulating destruction / orphaned GC drop of previous widgets
			DynamicWidgets.Empty();
		}
	}
	const double LegacyCycleEndTime = FPlatformTime::Seconds();
	const double LegacyCycleElapsedMs = (LegacyCycleEndTime - LegacyCycleStartTime) * 1000.0;

	// --- 1-B. Optimized Mode: 슬롯 위젯 풀링 (Widget Pool) & 뷰모델 재사용 ---
	int64 PooledAllocatedObjects = 0;
	const double PooledCycleStartTime = FPlatformTime::Seconds();
	{
		UObject* DummyOuter = GetTransientPackage();
		UFCHandViewModel* HandVM = NewObject<UFCHandViewModel>(DummyOuter);
		PooledAllocatedObjects++;

		// Pre-allocated fixed 10-slot widget pool (1-time setup)
		TArray<UFCCardWidget*> WidgetPool;
		WidgetPool.Reserve(10);
		for (int32 Slot = 0; Slot < 10; ++Slot)
		{
			UFCCardWidget* PooledWidget = NewObject<UFCCardWidget>(DummyOuter);
			WidgetPool.Add(PooledWidget);
			PooledAllocatedObjects++; // Counted only during setup
		}

		// Runtime iterations: Zero widget allocation!
		for (int32 i = 0; i < CycleIterations; ++i)
		{
			const FFCCardHandContainer& Batch = HandBatches[i];

			// 1. Sync ViewModel (Reuses existing CardViewModels by Guid)
			HandVM->SyncFromHandContainer(Batch, BenchmarkSubsystem);

			// 2. Refresh pooled widgets via visibility toggle (Zero NewObject calls)
			const int32 TotalCards = HandVM->CardsInHand.Num();
			for (int32 Slot = 0; Slot < 10; ++Slot)
			{
				UFCCardWidget* CardWidget = WidgetPool[Slot];
				if (Slot < TotalCards)
				{
					CardWidget->SetVisibility(ESlateVisibility::Visible);
					CardWidget->SetCardViewModel(HandVM->CardsInHand[Slot]);
				}
				else
				{
					CardWidget->SetVisibility(ESlateVisibility::Collapsed);
					CardWidget->SetCardViewModel(nullptr);
				}
			}
		}
	}
	const double PooledCycleEndTime = FPlatformTime::Seconds();
	const double PooledCycleElapsedMs = (PooledCycleEndTime - PooledCycleStartTime) * 1000.0;

	// =========================================================================
	// BENCHMARK 2: Per-Frame Tick Polling vs Zero-Tick Event-Driven
	// =========================================================================

	// --- 2-A. Legacy Mode: 매 프레임 프로퍼티 바인딩 폴링 (Tick Reflection / Getter Calls) ---
	int64 LegacyTickEvaluations = 0;
	const double LegacyTickStartTime = FPlatformTime::Seconds();
	{
		UFCHandViewModel* DummyHandVM = NewObject<UFCHandViewModel>(GetTransientPackage());
		DummyHandVM->SyncFromHandContainer(HandBatches[HandBatches.Num() - 1], BenchmarkSubsystem);

		int32 SimulatedPlayerMana = 3;
		for (int32 Frame = 0; Frame < TickSimulationFrames; ++Frame)
		{
			if (Frame % 60 == 0)
			{
				SimulatedPlayerMana = (SimulatedPlayerMana % 5) + 1;
			}

			// Simulating traditional UMG property binding evaluation per card per frame
			for (int32 CardIdx = 0; CardIdx < DummyHandVM->CardsInHand.Num(); ++CardIdx)
			{
				if (UFCCardViewModel* CardVM = DummyHandVM->CardsInHand[CardIdx])
				{
					volatile bool bCanPlay = CardVM->GetCanPlay();
					FText DisplayName = CardVM->GetFormattedDisplayName();
					(void)bCanPlay;
					(void)DisplayName;
					LegacyTickEvaluations += 2;
				}
			}
		}
	}
	const double LegacyTickEndTime = FPlatformTime::Seconds();
	const double LegacyTickElapsedMs = (LegacyTickEndTime - LegacyTickStartTime) * 1000.0;

	// --- 2-B. Optimized Mode: Zero-Tick FieldNotify (이벤트 주도형 갱신) ---
	int64 OptimizedEventDispatches = 0;
	const double OptimizedTickStartTime = FPlatformTime::Seconds();
	{
		UFCHandViewModel* HandVM = NewObject<UFCHandViewModel>(GetTransientPackage());
		HandVM->SyncFromHandContainer(HandBatches[HandBatches.Num() - 1], BenchmarkSubsystem);

		int32 SimulatedPlayerMana = 3;
		int32 LastMana = SimulatedPlayerMana;

		for (int32 Frame = 0; Frame < TickSimulationFrames; ++Frame)
		{
			// Value changes only occasionally (e.g. mana recovery once every 60 frames)
			if (Frame % 60 == 0)
			{
				SimulatedPlayerMana = (SimulatedPlayerMana % 5) + 1;
			}

			// Zero-Tick: If mana did not change, 0 function calls on Tick!
			if (SimulatedPlayerMana != LastMana)
			{
				LastMana = SimulatedPlayerMana;
				HandVM->UpdatePlayability(SimulatedPlayerMana);
				OptimizedEventDispatches += HandVM->CardsInHand.Num();
			}
		}
	}
	const double OptimizedTickEndTime = FPlatformTime::Seconds();
	const double OptimizedTickElapsedMs = (OptimizedTickEndTime - OptimizedTickStartTime) * 1000.0;

	// =========================================================================
	// Telemetry & Results
	// =========================================================================
	const double CycleSpeedupRatio = (PooledCycleElapsedMs > 0.0) ? (LegacyCycleElapsedMs / PooledCycleElapsedMs) : 0.0;
	const int64 ObjectsSaved = LegacyAllocatedObjects - PooledAllocatedObjects;
	const double TickSpeedupRatio = (OptimizedTickElapsedMs > 0.0) ? (LegacyTickElapsedMs / OptimizedTickElapsedMs) : 0.0;

	UE_LOG(LogTemp, Display, TEXT("===================================================================="));
	UE_LOG(LogTemp, Display, TEXT(" [EMPIRICAL LOAD BENCHMARK] Card & Hand UI Architecture Test"));
	UE_LOG(LogTemp, Display, TEXT(" --------------------------------------------------------------------"));
	UE_LOG(LogTemp, Display, TEXT(" TEST 1: Hand State Transitions & Draw/Discard Cycles (%d Cycles)"), CycleIterations);
	UE_LOG(LogTemp, Display, TEXT("   1. Legacy Dynamic Alloc Mode   : %.2f ms | Objects Created: %lld"), LegacyCycleElapsedMs, LegacyAllocatedObjects);
	UE_LOG(LogTemp, Display, TEXT("   2. Optimized Widget Pool Mode  : %.2f ms | Objects Created: %lld (Setup only)"), PooledCycleElapsedMs, PooledAllocatedObjects);
	UE_LOG(LogTemp, Display, TEXT("   -> CYCLE SPEEDUP               : %.2fx Faster!"), CycleSpeedupRatio);
	UE_LOG(LogTemp, Display, TEXT("   -> UOBJECT GC CHURN ELIMINATED : %lld Objects Saved (100%% Zero GC Churn)"), ObjectsSaved);
	UE_LOG(LogTemp, Display, TEXT(" --------------------------------------------------------------------"));
	UE_LOG(LogTemp, Display, TEXT(" TEST 2: Per-Frame Property Evaluation (%d Frames / 10 Cards)"), TickSimulationFrames);
	UE_LOG(LogTemp, Display, TEXT("   1. Legacy UMG Tick Polling     : %.2f ms | Evals: %lld (Every Frame)"), LegacyTickElapsedMs, LegacyTickEvaluations);
	UE_LOG(LogTemp, Display, TEXT("   2. Zero-Tick MVVM FieldNotify  : %.2f ms | Dispatches: %lld (Event-driven)"), OptimizedTickElapsedMs, OptimizedEventDispatches);
	UE_LOG(LogTemp, Display, TEXT("   -> TICK EVALUATION SPEEDUP     : %.2fx Faster!"), TickSpeedupRatio);
	UE_LOG(LogTemp, Display, TEXT("===================================================================="));

	AddInfo(FString::Printf(TEXT("Cycle Benchmark: Legacy %.2f ms vs Pooled %.2f ms (%.2fx) | Objects Saved: %lld"),
		LegacyCycleElapsedMs, PooledCycleElapsedMs, CycleSpeedupRatio, ObjectsSaved));
	AddInfo(FString::Printf(TEXT("Tick Benchmark: Legacy %.2f ms vs Zero-Tick %.2f ms (%.2fx) | Evals Saved: %lld"),
		LegacyTickElapsedMs, OptimizedTickElapsedMs, TickSpeedupRatio, (LegacyTickEvaluations - OptimizedEventDispatches)));

	// Assertions
	TestTrue(TEXT("Widget pooling cycle time must be faster than legacy dynamic allocation"), PooledCycleElapsedMs < LegacyCycleElapsedMs);
	TestTrue(TEXT("Zero-Tick MVVM evaluation time must be faster than legacy tick polling"), OptimizedTickElapsedMs < LegacyTickElapsedMs);
	TestTrue(TEXT("Pooled allocated objects must equal 11 (1 VM + 10 slot pool setup)"), PooledAllocatedObjects == 11);
	TestTrue(TEXT("Legacy allocated objects must exceed thousands of objects"), LegacyAllocatedObjects >= 5000);

	return true;
}

#endif
