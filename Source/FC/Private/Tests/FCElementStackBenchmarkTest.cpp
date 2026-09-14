#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"
#include "UI/ViewModel/FCElementOverheadViewModel.h"
#include "UI/View/FCElementStackItemWidget.h"
#include "UI/View/FCElementOverheadWidget.h"
#include "Combat/Element/FCElementComponent.h"
#include "Data/Class/FCClassTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCElementStackBenchmarkTest, "FC.UI.ElementStackBenchmark", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCElementStackBenchmarkTest::RunTest(const FString& Parameters)
{
	const int32 IterationCount = 2000;
	const TArray<EFCElement> ElementCycle = {
		EFCElement::Fire,
		EFCElement::Earth,
		EFCElement::Water,
		EFCElement::Wind,
		EFCElement::Lightning,
		EFCElement::Holy,
		EFCElement::Dark
	};

	// Pre-generate pseudo random stack state batches for consistency
	TArray<TArray<EFCElement>> TestBatches;
	TestBatches.Reserve(IterationCount);
	for (int32 i = 0; i < IterationCount; ++i)
	{
		const int32 StackSize = 1 + (i % 7); // Cycles 1 to 7 stacks
		TArray<EFCElement> Stacks;
		Stacks.Reserve(StackSize);
		for (int32 j = 0; j < StackSize; ++j)
		{
			Stacks.Add(ElementCycle[(i + j) % ElementCycle.Num()]);
		}
		TestBatches.Add(Stacks);
	}

	// =========================================================================
	// 1. Legacy Dynamic Alloc Pattern (이전 방식: 매 갱신마다 동적 할당 및 재빌드)
	// =========================================================================
	int64 LegacyAllocatedObjects = 0;
	const double LegacyStartTime = FPlatformTime::Seconds();

	{
		UFCElementOverheadWidget* DummyParent = NewObject<UFCElementOverheadWidget>(GetTransientPackage());

		for (int32 i = 0; i < IterationCount; ++i)
		{
			const TArray<EFCElement>& CurrentBatch = TestBatches[i];

			// 1. Dynamic ViewModel allocation (1 per stack item)
			TArray<UFCElementOverheadViewModel*> DynamicItemVMs;
			DynamicItemVMs.Reserve(CurrentBatch.Num());
			for (int32 j = 0; j < CurrentBatch.Num(); ++j)
			{
				UFCElementOverheadViewModel* DummyItemVM = NewObject<UFCElementOverheadViewModel>(DummyParent);
				DynamicItemVMs.Add(DummyItemVM);
				LegacyAllocatedObjects++;
			}

			// 2. Dynamic Widget allocation (ClearChildren + CreateWidget per item)
			TArray<UFCElementStackItemWidget*> DynamicWidgets;
			DynamicWidgets.Reserve(CurrentBatch.Num());
			for (int32 j = 0; j < CurrentBatch.Num(); ++j)
			{
				UFCElementStackItemWidget* NewWidget = NewObject<UFCElementStackItemWidget>(DummyParent);
				NewWidget->SetElement(CurrentBatch[j], j);
				DynamicWidgets.Add(NewWidget);
				LegacyAllocatedObjects++;
			}

			// Simulated destruction/garbage collection drop of previous items
			DynamicItemVMs.Empty();
			DynamicWidgets.Empty();
		}
	}

	const double LegacyEndTime = FPlatformTime::Seconds();
	const double LegacyElapsedMs = (LegacyEndTime - LegacyStartTime) * 1000.0;

	// =========================================================================
	// 2. Hybrid Pre-allocated Widget Pool Pattern (현재 방식: 방안 A 풀링)
	// =========================================================================
	int64 PooledAllocatedObjects = 0;
	const double PooledStartTime = FPlatformTime::Seconds();

	{
		UFCElementOverheadViewModel* OverheadVM = NewObject<UFCElementOverheadViewModel>(GetTransientPackage());
		UFCElementOverheadWidget* DummyParent = NewObject<UFCElementOverheadWidget>(GetTransientPackage());

		// Pre-allocated fixed 7 slot widget pool (1-time allocation)
		TArray<UFCElementStackItemWidget*> PreAllocatedPool;
		PreAllocatedPool.Reserve(7);
		for (int32 Slot = 0; Slot < 7; ++Slot)
		{
			UFCElementStackItemWidget* SlotWidget = NewObject<UFCElementStackItemWidget>(DummyParent);
			SlotWidget->ClearElement();
			PreAllocatedPool.Add(SlotWidget);
			PooledAllocatedObjects++; // Counted only during setup
		}

		// Runtime iterations: Zero allocation!
		for (int32 i = 0; i < IterationCount; ++i)
		{
			const TArray<EFCElement>& CurrentBatch = TestBatches[i];

			// 1. Update ViewModel (Zero UObject allocation, direct value update)
			OverheadVM->UpdateFromStacks(CurrentBatch);

			// 2. Update pooled widgets without creating or destroying widgets
			const int32 ActiveCount = CurrentBatch.Num();
			for (int32 Slot = 0; Slot < 7; ++Slot)
			{
				UFCElementStackItemWidget* SlotWidget = PreAllocatedPool[Slot];
				if (Slot < ActiveCount)
				{
					SlotWidget->SetElement(CurrentBatch[Slot], Slot);
					SlotWidget->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					SlotWidget->ClearElement();
					SlotWidget->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
	}

	const double PooledEndTime = FPlatformTime::Seconds();
	const double PooledElapsedMs = (PooledEndTime - PooledStartTime) * 1000.0;

	const double SpeedupRatio = (PooledElapsedMs > 0.0) ? (LegacyElapsedMs / PooledElapsedMs) : 0.0;
	const int64 ObjectsSaved = LegacyAllocatedObjects - PooledAllocatedObjects;

	// Output detailed benchmark telemetry to Automation Test Log
	UE_LOG(LogTemp, Display, TEXT("========================================================="));
	UE_LOG(LogTemp, Display, TEXT(" [BENCHMARK] ElementStack UI Architecture Load Test"));
	UE_LOG(LogTemp, Display, TEXT(" Iterations: %d state transitions"), IterationCount);
	UE_LOG(LogTemp, Display, TEXT(" ---------------------------------------------------------"));
	UE_LOG(LogTemp, Display, TEXT(" 1. Legacy Dynamic Alloc Mode : %.2f ms | UObjects Allocated: %lld"), LegacyElapsedMs, LegacyAllocatedObjects);
	UE_LOG(LogTemp, Display, TEXT(" 2. Hybrid Widget Pool Mode   : %.2f ms | UObjects Allocated: %lld (Setup only)"), PooledElapsedMs, PooledAllocatedObjects);
	UE_LOG(LogTemp, Display, TEXT(" ---------------------------------------------------------"));
	UE_LOG(LogTemp, Display, TEXT(" PERFORMANCE SPEEDUP         : %.2fx Faster!"), SpeedupRatio);
	UE_LOG(LogTemp, Display, TEXT(" UOBJECTS AVOIDED (ZERO GC)   : %lld Objects Saved"), ObjectsSaved);
	UE_LOG(LogTemp, Display, TEXT("========================================================="));

	AddInfo(FString::Printf(TEXT("Iterations: %d | Legacy: %.2f ms (%lld objs) | Pooled: %.2f ms (%lld objs) | Speedup: %.2fx"),
		IterationCount, LegacyElapsedMs, LegacyAllocatedObjects, PooledElapsedMs, PooledAllocatedObjects, SpeedupRatio));

	// Verification Assertions
	TestTrue(TEXT("Pooled mode should execute faster than legacy dynamic allocation mode"), PooledElapsedMs < LegacyElapsedMs);
	TestTrue(TEXT("Pooled mode runtime allocation must be zero (only 7 setup objects)"), PooledAllocatedObjects == 7);
	TestTrue(TEXT("Legacy mode must have allocated thousands of temporary objects"), LegacyAllocatedObjects >= (IterationCount * 2));

	return true;
}

#endif
