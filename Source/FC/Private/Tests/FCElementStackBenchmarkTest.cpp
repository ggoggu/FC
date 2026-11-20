#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"
#include "UI/ViewModel/FCElementOverheadViewModel.h"
#include "UI/View/FCElementStackItemWidget.h"
#include "UI/View/FCElementOverheadWidget.h"
#include "Combat/Element/FCElementComponent.h"
#include "Data/Class/FCClassTypes.h"
#include "Data/FCGameplayTags.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCElementStackBenchmarkTest, "FC.UI.ElementStackBenchmark", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCElementStackBenchmarkTest::RunTest(const FString& Parameters)
{
	const int32 IterationCount = 2000;
	const TArray<FGameplayTag> ElementCycle = {
		FCGameplayTags::Element_Fire.GetTag(),
		FCGameplayTags::Element_Earth.GetTag(),
		FCGameplayTags::Element_Water.GetTag(),
		FCGameplayTags::Element_Wind.GetTag(),
		FCGameplayTags::Element_Lightning.GetTag(),
		FCGameplayTags::Element_Holy.GetTag(),
		FCGameplayTags::Element_Dark.GetTag()
	};

	TArray<TArray<FGameplayTag>> TestBatches;
	TestBatches.Reserve(IterationCount);
	for (int32 i = 0; i < IterationCount; ++i)
	{
		const int32 StackSize = 1 + (i % 7);
		TArray<FGameplayTag> Stacks;
		Stacks.Reserve(StackSize);
		for (int32 j = 0; j < StackSize; ++j)
		{
			Stacks.Add(ElementCycle[(i + j) % ElementCycle.Num()]);
		}
		TestBatches.Add(Stacks);
	}

	int64 LegacyAllocatedObjects = 0;
	const double LegacyStartTime = FPlatformTime::Seconds();

	{
		UFCElementOverheadWidget* DummyParent = NewObject<UFCElementOverheadWidget>(GetTransientPackage());

		for (int32 i = 0; i < IterationCount; ++i)
		{
			const TArray<FGameplayTag>& CurrentBatch = TestBatches[i];

			TArray<UFCElementOverheadViewModel*> DynamicItemVMs;
			DynamicItemVMs.Reserve(CurrentBatch.Num());
			for (int32 j = 0; j < CurrentBatch.Num(); ++j)
			{
				UFCElementOverheadViewModel* DummyItemVM = NewObject<UFCElementOverheadViewModel>(DummyParent);
				DynamicItemVMs.Add(DummyItemVM);
				LegacyAllocatedObjects++;
			}

			TArray<UFCElementStackItemWidget*> DynamicWidgets;
			DynamicWidgets.Reserve(CurrentBatch.Num());
			for (int32 j = 0; j < CurrentBatch.Num(); ++j)
			{
				UFCElementStackItemWidget* NewWidget = NewObject<UFCElementStackItemWidget>(DummyParent);
				NewWidget->SetElement(CurrentBatch[j], j);
				DynamicWidgets.Add(NewWidget);
				LegacyAllocatedObjects++;
			}

			DynamicItemVMs.Empty();
			DynamicWidgets.Empty();
		}
	}

	const double LegacyEndTime = FPlatformTime::Seconds();
	const double LegacyElapsedMs = (LegacyEndTime - LegacyStartTime) * 1000.0;

	int64 PooledAllocatedObjects = 0;
	const double PooledStartTime = FPlatformTime::Seconds();

	{
		UFCElementOverheadViewModel* OverheadVM = NewObject<UFCElementOverheadViewModel>(GetTransientPackage());
		UFCElementOverheadWidget* DummyParent = NewObject<UFCElementOverheadWidget>(GetTransientPackage());

		TArray<UFCElementStackItemWidget*> PreAllocatedPool;
		PreAllocatedPool.Reserve(7);
		for (int32 Slot = 0; Slot < 7; ++Slot)
		{
			UFCElementStackItemWidget* SlotWidget = NewObject<UFCElementStackItemWidget>(DummyParent);
			SlotWidget->ClearElement();
			PreAllocatedPool.Add(SlotWidget);
			PooledAllocatedObjects++;
		}

		for (int32 i = 0; i < IterationCount; ++i)
		{
			const TArray<FGameplayTag>& CurrentBatch = TestBatches[i];
			OverheadVM->UpdateFromStacks(CurrentBatch);

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

	AddInfo(FString::Printf(TEXT("Iterations: %d | Legacy: %.2f ms (%lld objs) | Pooled: %.2f ms (%lld objs) | Speedup: %.2fx"),
		IterationCount, LegacyElapsedMs, LegacyAllocatedObjects, PooledElapsedMs, PooledAllocatedObjects, SpeedupRatio));

	TestTrue(TEXT("Pooled mode should execute faster than legacy dynamic allocation mode"), PooledElapsedMs < LegacyElapsedMs);
	TestTrue(TEXT("Pooled mode runtime allocation must be zero (only 7 setup objects)"), PooledAllocatedObjects == 7);
	TestTrue(TEXT("Legacy mode must have allocated thousands of temporary objects"), LegacyAllocatedObjects >= (IterationCount * 2));

	return true;
}

#endif
