#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "UI/ViewModel/FCStatusToggleItemViewModel.h"
#include "UI/ViewModel/FCStatusToggleContainerViewModel.h"
#include "UI/ViewModel/FCHUDViewModel.h"
#include "UI/Presenter/FCStatusPresenterComponent.h"
#include "UI/Data/FCStatusToggleTypes.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/Player/FCPlayerCharacter.h"
#include "Data/FCGameplayTags.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

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

			// Pure ViewModel checks
			HUDVM->SetCurrentShield(20);
			TestEqual(TEXT("Shield value should match"), HUDVM->CurrentShield, 20);
			TestTrue(TEXT("HasShield should be true"), HUDVM->HasShield());

			HUDVM->SetCurrentShield(0);
			TestEqual(TEXT("Shield value should be 0"), HUDVM->CurrentShield, 0);
			TestFalse(TEXT("HasShield should be false"), HUDVM->HasShield());
		}
	}

	// =========================================================================
	// Test 4: UFCStatusPresenterComponent Integration, Single Shield & Tag Auto-Subscription
	// =========================================================================
	if (GEngine)
	{
		UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
		TestNotNull(TEXT("TestWorld should be created"), TestWorld);

		if (TestWorld)
		{
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(TestWorld);
			TestWorld->InitializeActorsForPlay(FURL());
			TestWorld->BeginPlay();

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AFCPlayerCharacter* TestChar = TestWorld->SpawnActor<AFCPlayerCharacter>(AFCPlayerCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			TestNotNull(TEXT("TestChar should spawn in TestWorld"), TestChar);

			if (TestChar)
			{
				UAbilitySystemComponent* ASC = TestChar->GetAbilitySystemComponent();
				UFCAttributeSet* AttrSet = TestChar->GetAttributeSet();
				UFCStatusPresenterComponent* Presenter = NewObject<UFCStatusPresenterComponent>(TestChar);
				UFCStatusToggleContainerViewModel* StatusVM = NewObject<UFCStatusToggleContainerViewModel>(TestChar);

				TestNotNull(TEXT("ASC should exist"), ASC);
				TestNotNull(TEXT("AttrSet should exist"), AttrSet);
				TestNotNull(TEXT("Presenter should exist"), Presenter);
				TestNotNull(TEXT("StatusVM should exist"), StatusVM);

				if (ASC && AttrSet && Presenter && StatusVM)
				{
					TestChar->AddInstanceComponent(Presenter);
					Presenter->RegisterComponent();

					// Author a transient DT_StatusToggleCatalog with both attribute-based and GameplayTag-driven statuses
					UDataTable* TransientCatalog = NewObject<UDataTable>(TestChar);
					TransientCatalog->RowStruct = FFCStatusToggleDefinitionRow::StaticStruct();

					FFCStatusToggleDefinitionRow ShieldRow;
					ShieldRow.StatusId = FName(TEXT("Status_MagicShield"));
					ShieldRow.DisplayName = FText::FromString(TEXT("매직 실드"));
					TransientCatalog->AddRow(ShieldRow.StatusId, ShieldRow);

					FFCStatusToggleDefinitionRow AtkBuffRow;
					AtkBuffRow.StatusId = FName(TEXT("Status_AttackBuff"));
					AtkBuffRow.DisplayName = FText::FromString(TEXT("공격력 강화"));
					TransientCatalog->AddRow(AtkBuffRow.StatusId, AtkBuffRow);

					FFCStatusToggleDefinitionRow TagDrivenRow;
					TagDrivenRow.StatusId = FName(TEXT("Status_TagDrivenBurn"));
					TagDrivenRow.StatusTag = FCGameplayTags::Status_Debuff_AttackReduction;
					TagDrivenRow.DisplayName = FText::FromString(TEXT("약화"));
					TransientCatalog->AddRow(TagDrivenRow.StatusId, TagDrivenRow);

					Presenter->SetCatalogDataTable(TransientCatalog);

					AttrSet->InitShield(0.0f);
					AttrSet->InitMaxShield(100.0f);
					AttrSet->InitAttackPower(0.0f);

					Presenter->BindToAbilitySystem(ASC, StatusVM);
					TestEqual(TEXT("Initial status count should be 0"), StatusVM->GetStatusCount(), 0);

					// 1. Shield applied (> 0) -> Presenter automatically creates single Status_MagicShield toggle
					AttrSet->SetShield(20.0f);
					TestEqual(TEXT("Shield > 0 must add exactly 1 shield toggle"), StatusVM->GetStatusCount(), 1);
					UFCStatusToggleItemViewModel* ShieldItem = StatusVM->FindStatusById(FName(TEXT("Status_MagicShield")));
					TestNotNull(TEXT("Status_MagicShield item must exist"), ShieldItem);
					if (ShieldItem)
					{
						TestEqual(TEXT("Shield stack must be 1"), ShieldItem->StackCount, 1);
						TestTrue(TEXT("Shield must be active"), ShieldItem->bIsActive);
					}

					// 2. Shield reinforced -> Still maintains single toggle
					AttrSet->SetShield(40.0f);
					TestEqual(TEXT("Shield reinforced must still have exactly 1 toggle"), StatusVM->GetStatusCount(), 1);

					// 3. Shield depleted (<= 0) -> Presenter automatically removes shield toggle
					AttrSet->SetShield(0.0f);
					TestEqual(TEXT("Shield depleted must remove shield toggle"), StatusVM->GetStatusCount(), 0);
					TestNull(TEXT("Status_MagicShield should be null after removal"), StatusVM->FindStatusById(FName(TEXT("Status_MagicShield"))));

					// 4. AttackPower buff (> 0) -> Presenter creates AttackBuff toggle
					AttrSet->SetAttackPower(2.0f);
					TestEqual(TEXT("AttackPower > 0 adds buff toggle"), StatusVM->GetStatusCount(), 1);
					TestNotNull(TEXT("Status_AttackBuff should exist"), StatusVM->FindStatusById(FName(TEXT("Status_AttackBuff"))));
					AttrSet->SetAttackPower(0.0f);
					TestEqual(TEXT("AttackPower reset removes buff toggle"), StatusVM->GetStatusCount(), 0);

					// 5. DataTable StatusTag auto-subscription (OCP: zero C++ edits for new tag statuses)
					ASC->AddLooseGameplayTag(FCGameplayTags::Status_Debuff_AttackReduction);
					TestEqual(TEXT("Adding cataloged StatusTag must auto-add toggle"), StatusVM->GetStatusCount(), 1);
					TestNotNull(TEXT("Status_TagDrivenBurn must be found by StatusId"), StatusVM->FindStatusById(FName(TEXT("Status_TagDrivenBurn"))));

					ASC->RemoveLooseGameplayTag(FCGameplayTags::Status_Debuff_AttackReduction);
					TestEqual(TEXT("Removing cataloged StatusTag must auto-remove toggle"), StatusVM->GetStatusCount(), 0);

					Presenter->UnbindFromAbilitySystem();
				}
			}

			TestWorld->DestroyWorld(false);
			GEngine->DestroyWorldContext(TestWorld);
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
