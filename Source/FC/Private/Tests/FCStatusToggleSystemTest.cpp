#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "UI/ViewModel/FCStatusToggleItemViewModel.h"
#include "UI/ViewModel/FCStatusToggleContainerViewModel.h"
#include "UI/ViewModel/FCHUDViewModel.h"
#include "UI/Data/FCStatusToggleTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFCStatusToggleSystemTest, "FC.UI.StatusToggleSystem", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFCStatusToggleSystemTest::RunTest(const FString& Parameters)
{
	// =========================================================================
	// Test 1: Item ViewModel FieldNotify & Computed Value Mechanics
	// =========================================================================
	{
		UFCStatusToggleItemViewModel* ItemVM = NewObject<UFCStatusToggleItemViewModel>();
		TestNotNull(TEXT("ItemVM must be instantiable"), ItemVM);

		if (ItemVM)
		{
			// Default state checks
			TestFalse(TEXT("Initially should not have stack count"), ItemVM->HasStackCount());
			TestFalse(TEXT("Initially should not have duration"), ItemVM->HasDuration());
			TestEqual(TEXT("Initial DurationPercent should be 0.0"), ItemVM->GetDurationPercent(), 0.0f);

			// Setup stack count
			ItemVM->SetStackCount(3);
			TestTrue(TEXT("HasStackCount should be true when StackCount > 0"), ItemVM->HasStackCount());
			TestEqual(TEXT("GetStackDisplayText should return '3'"), ItemVM->GetStackDisplayText().ToString(), FString(TEXT("3")));

			// Duration progression
			ItemVM->SetDuration(7.0f, 10.0f);
			TestTrue(TEXT("HasDuration should be true"), ItemVM->HasDuration());
			TestNearlyEqual(TEXT("DurationPercent should be 0.7"), ItemVM->GetDurationPercent(), 0.7f, 0.001f);

			// Untimed / Permanent duration
			ItemVM->SetDuration(0.0f, 0.0f);
			TestFalse(TEXT("Duration <= 0 should clear HasDuration"), ItemVM->HasDuration());
			TestEqual(TEXT("DurationPercent should be 0.0 for permanent"), ItemVM->GetDurationPercent(), 0.0f);
		}
	}

	// =========================================================================
	// Test 2: Container ViewModel Dynamic Collection Operations
	// =========================================================================
	{
		UFCStatusToggleContainerViewModel* ContainerVM = NewObject<UFCStatusToggleContainerViewModel>();
		TestNotNull(TEXT("ContainerVM must be instantiable"), ContainerVM);

		if (ContainerVM)
		{
			TestEqual(TEXT("Initial status count must be 0"), ContainerVM->GetStatusCount(), 0);

			// Add Magic Shield toggle
			FFCStatusToggleData ShieldData;
			ShieldData.StatusId = FName(TEXT("Status_MagicShield"));
			ShieldData.DisplayName = FText::FromString(TEXT("매직 실드"));
			ShieldData.BorderColor = FLinearColor(0.0f, 0.8f, 1.0f, 1.0f);
			ShieldData.bIsActive = true;
			ShieldData.StackCount = 1;
			ShieldData.RemainingDuration = 0.0f;
			ShieldData.TotalDuration = 0.0f;
			ShieldData.Description = FText::FromString(TEXT("공격 1회 방어"));

			UFCStatusToggleItemViewModel* AddedShield = ContainerVM->AddOrUpdateStatus(ShieldData);
			TestNotNull(TEXT("AddOrUpdateStatus should return instantiated ItemVM"), AddedShield);
			TestEqual(TEXT("Status count should be 1"), ContainerVM->GetStatusCount(), 1);
			TestTrue(TEXT("Shield should be active"), AddedShield->bIsActive);
			TestEqual(TEXT("Shield stack should be 1"), AddedShield->StackCount, 1);
			TestFalse(TEXT("Shield has no duration limit"), AddedShield->HasDuration());

			// Add Attack Reduction Debuff
			FFCStatusToggleData AtkDebuffData;
			AtkDebuffData.StatusId = FName(TEXT("Status_AttackReduction"));
			AtkDebuffData.DisplayName = FText::FromString(TEXT("공격력 감소"));
			AtkDebuffData.BorderColor = FLinearColor(1.0f, 0.1f, 0.1f, 1.0f);
			AtkDebuffData.bIsActive = true;
			AtkDebuffData.StackCount = 2;
			AtkDebuffData.RemainingDuration = 8.0f;
			AtkDebuffData.TotalDuration = 10.0f;

			UFCStatusToggleItemViewModel* AddedDebuff = ContainerVM->AddOrUpdateStatus(AtkDebuffData);
			TestNotNull(TEXT("Debuff ItemVM should be valid"), AddedDebuff);
			TestEqual(TEXT("Status count should be 2"), ContainerVM->GetStatusCount(), 2);
			TestNearlyEqual(TEXT("Debuff ratio should be 0.8"), AddedDebuff->GetDurationPercent(), 0.8f, 0.001f);

			// In-place update existing status (e.g. refresh duration)
			AtkDebuffData.RemainingDuration = 5.0f;
			ContainerVM->AddOrUpdateStatus(AtkDebuffData);
			TestEqual(TEXT("Status count should still be 2 after update"), ContainerVM->GetStatusCount(), 2);
			TestNearlyEqual(TEXT("Debuff ratio should update to 0.5"), AddedDebuff->GetDurationPercent(), 0.5f, 0.001f);

			// Remove Magic Shield when consumed
			bool bRemoved = ContainerVM->RemoveStatusById(FName(TEXT("Status_MagicShield")));
			TestTrue(TEXT("Shield should be successfully removed"), bRemoved);
			TestEqual(TEXT("Status count should decrease to 1"), ContainerVM->GetStatusCount(), 1);
			TestNull(TEXT("Shield should no longer be found"), ContainerVM->FindStatusById(FName(TEXT("Status_MagicShield"))));

			// Clear all
			ContainerVM->ClearAllStatuses();
			TestEqual(TEXT("Status count should be 0 after ClearAll"), ContainerVM->GetStatusCount(), 0);
		}
	}

	// =========================================================================
	// Test 3: HUDViewModel Hierarchy Sub-ViewModel Linking
	// =========================================================================
	{
		UFCHUDViewModel* HUDVM = NewObject<UFCHUDViewModel>();
		TestNotNull(TEXT("HUDVM must be instantiable"), HUDVM);

		if (HUDVM)
		{
			UFCStatusToggleContainerViewModel* StatusVM = HUDVM->GetOrCreateStatusContainerViewModel();
			TestNotNull(TEXT("StatusVM should be lazily created"), StatusVM);
			TestTrue(TEXT("Subsequent calls should return same StatusVM instance"), StatusVM == HUDVM->GetOrCreateStatusContainerViewModel());
			TestTrue(TEXT("StatusVM Outer should be HUDVM"), StatusVM->GetOuter() == HUDVM);
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
