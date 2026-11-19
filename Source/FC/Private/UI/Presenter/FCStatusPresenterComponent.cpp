#include "UI/Presenter/FCStatusPresenterComponent.h"
#include "UI/ViewModel/FCStatusToggleContainerViewModel.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/DataTable.h"

UFCStatusPresenterComponent::UFCStatusPresenterComponent()
	: CatalogDataTable(nullptr)
	, DefaultCatalogTablePath(TEXT("/Game/UI/Data/DT_StatusToggleCatalog.DT_StatusToggleCatalog"))
	, bAutoManageShieldToggle(true)
	, bAutoManageAttackBuffToggle(true)
	, TargetViewModel(nullptr)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFCStatusPresenterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromAbilitySystem();
	Super::EndPlay(EndPlayReason);
}

void UFCStatusPresenterComponent::EnsureCatalogLoaded()
{
	if (!CatalogDataTable && DefaultCatalogTablePath.IsValid())
	{
		CatalogDataTable = Cast<UDataTable>(DefaultCatalogTablePath.TryLoad());
	}
}

void UFCStatusPresenterComponent::BindToAbilitySystem(UAbilitySystemComponent* InASC, UFCStatusToggleContainerViewModel* InTargetViewModel)
{
	UnbindFromAbilitySystem();

	TargetViewModel = InTargetViewModel;
	BoundASC = InASC;

	if (!InASC || !InTargetViewModel)
	{
		return;
	}

	EnsureCatalogLoaded();

	// 1. Initial Attributes Synchronization
	const UFCAttributeSet* AttrSet = InASC->GetSet<UFCAttributeSet>();
	if (AttrSet)
	{
		if (bAutoManageShieldToggle)
		{
			HandleShieldChanged(AttrSet->GetShield(), 0.0f);
		}
		if (bAutoManageAttackBuffToggle)
		{
			HandleAttackPowerChanged(AttrSet->GetAttackPower(), 0.0f);
		}
	}

	// 2. Register Attribute Change Listeners
	auto RegisterAttrListener = [this, InASC](const FGameplayAttribute& Attribute, auto&& Callback)
	{
		FDelegateHandle Handle = InASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddWeakLambda(
			this,
			[Callback](const FOnAttributeChangeData& Data)
			{
				Callback(Data.NewValue, Data.OldValue);
			});
		BoundAttributeHandles.Add(MakeTuple(Attribute, Handle));
	};

	if (bAutoManageShieldToggle)
	{
		RegisterAttrListener(UFCAttributeSet::GetShieldAttribute(), [this](float NewVal, float OldVal)
		{
			HandleShieldChanged(NewVal, OldVal);
		});
	}

	if (bAutoManageAttackBuffToggle)
	{
		RegisterAttrListener(UFCAttributeSet::GetAttackPowerAttribute(), [this](float NewVal, float OldVal)
		{
			HandleAttackPowerChanged(NewVal, OldVal);
		});
	}
}

void UFCStatusPresenterComponent::UnbindFromAbilitySystem()
{
	if (UAbilitySystemComponent* CurrentASC = BoundASC.Get())
	{
		for (const auto& Entry : BoundAttributeHandles)
		{
			CurrentASC->GetGameplayAttributeValueChangeDelegate(Entry.Key).Remove(Entry.Value);
		}

		for (const auto& TagPair : BoundTagHandles)
		{
			CurrentASC->RegisterGameplayTagEvent(TagPair.Key, EGameplayTagEventType::AnyCountChange).Remove(TagPair.Value);
		}
	}

	BoundAttributeHandles.Empty();
	BoundTagHandles.Empty();
	BoundASC.Reset();
	TargetViewModel = nullptr;
}

bool UFCStatusPresenterComponent::FindDefinitionById(FName InStatusId, FFCStatusToggleDefinitionRow& OutRow) const
{
	const_cast<UFCStatusPresenterComponent*>(this)->EnsureCatalogLoaded();

	if (CatalogDataTable && InStatusId != NAME_None)
	{
		if (const FFCStatusToggleDefinitionRow* Row = CatalogDataTable->FindRow<FFCStatusToggleDefinitionRow>(InStatusId, TEXT("FindDefinitionById"), false))
		{
			OutRow = *Row;
			return true;
		}

		// Also check by StatusId property within rows
		if (CatalogDataTable->GetRowStruct() && CatalogDataTable->GetRowStruct()->IsChildOf(FFCStatusToggleDefinitionRow::StaticStruct()))
		{
			for (const auto& Pair : CatalogDataTable->GetRowMap())
			{
				const FFCStatusToggleDefinitionRow* Row = reinterpret_cast<const FFCStatusToggleDefinitionRow*>(Pair.Value);
				if (Row && Row->StatusId == InStatusId)
				{
					OutRow = *Row;
					return true;
				}
			}
		}
	}

	return false;
}

bool UFCStatusPresenterComponent::FindDefinitionByTag(const FGameplayTag& InTag, FFCStatusToggleDefinitionRow& OutRow) const
{
	const_cast<UFCStatusPresenterComponent*>(this)->EnsureCatalogLoaded();

	if (CatalogDataTable && InTag.IsValid() && CatalogDataTable->GetRowStruct() && CatalogDataTable->GetRowStruct()->IsChildOf(FFCStatusToggleDefinitionRow::StaticStruct()))
	{
		for (const auto& Pair : CatalogDataTable->GetRowMap())
		{
			const FFCStatusToggleDefinitionRow* Row = reinterpret_cast<const FFCStatusToggleDefinitionRow*>(Pair.Value);
			if (Row && Row->StatusTag.MatchesTagExact(InTag))
			{
				OutRow = *Row;
				return true;
			}
		}
	}

	return false;
}

bool UFCStatusPresenterComponent::AddStatusFromCatalog(FName InStatusId, int32 InStackCount, float InRemaining, float InTotal, bool bInActive)
{
	if (!TargetViewModel || InStatusId == NAME_None)
	{
		return false;
	}

	FFCStatusToggleDefinitionRow DefRow;
	if (FindDefinitionById(InStatusId, DefRow))
	{
		TargetViewModel->AddOrUpdateStatus(DefRow.ToStatusToggleData(InStackCount, InRemaining, InTotal, bInActive));
		return true;
	}

	// Safe fallback for standard Shield status if catalog row is not yet authored
	if (InStatusId == FName(TEXT("Status_MagicShield")))
	{
		FFCStatusToggleData FallbackData;
		FallbackData.StatusId = InStatusId;
		FallbackData.DisplayName = FText::FromString(TEXT("매직 실드"));
		FallbackData.BorderColor = FLinearColor(0.0f, 0.75f, 1.0f, 1.0f);
		FallbackData.bIsActive = bInActive;
		FallbackData.StackCount = InStackCount;
		FallbackData.Description = FText::FromString(TEXT("공격 1회 방어"));
		TargetViewModel->AddOrUpdateStatus(FallbackData);
		return true;
	}

	return false;
}

bool UFCStatusPresenterComponent::AddStatusFromCatalogByTag(const FGameplayTag& InTag, int32 InStackCount, float InRemaining, float InTotal, bool bInActive)
{
	if (!TargetViewModel || !InTag.IsValid())
	{
		return false;
	}

	FFCStatusToggleDefinitionRow DefRow;
	if (FindDefinitionByTag(InTag, DefRow))
	{
		TargetViewModel->AddOrUpdateStatus(DefRow.ToStatusToggleData(InStackCount, InRemaining, InTotal, bInActive));
		return true;
	}

	return false;
}

bool UFCStatusPresenterComponent::RemoveStatusById(FName InStatusId)
{
	if (TargetViewModel && InStatusId != NAME_None)
	{
		return TargetViewModel->RemoveStatusById(InStatusId);
	}
	return false;
}

bool UFCStatusPresenterComponent::RemoveStatusByTag(const FGameplayTag& InTag)
{
	if (TargetViewModel && InTag.IsValid())
	{
		return TargetViewModel->RemoveStatusByTag(InTag);
	}
	return false;
}

void UFCStatusPresenterComponent::PushStatusData(const FFCStatusToggleData& InData)
{
	if (TargetViewModel)
	{
		TargetViewModel->AddOrUpdateStatus(InData);
	}
}

void UFCStatusPresenterComponent::HandleShieldChanged(float NewShield, float OldShield)
{
	static const FName ShieldStatusId(TEXT("Status_MagicShield"));

	if (NewShield > 0.0f)
	{
		// Single shield rule: always 1 stack of shield guard indicator
		AddStatusFromCatalog(ShieldStatusId, 1, 0.0f, 0.0f, true);
	}
	else
	{
		RemoveStatusById(ShieldStatusId);
	}
}

void UFCStatusPresenterComponent::HandleAttackPowerChanged(float NewAttackPower, float OldAttackPower)
{
	static const FName AtkBuffId(TEXT("Status_AttackBuff"));
	static const FName AtkDebuffId(TEXT("Status_AttackReduction"));

	if (NewAttackPower > 0.0f)
	{
		AddStatusFromCatalog(AtkBuffId, FMath::RoundToInt(NewAttackPower), 0.0f, 0.0f, true);
		RemoveStatusById(AtkDebuffId);
	}
	else if (NewAttackPower < 0.0f)
	{
		AddStatusFromCatalog(AtkDebuffId, FMath::Abs(FMath::RoundToInt(NewAttackPower)), 0.0f, 0.0f, true);
		RemoveStatusById(AtkBuffId);
	}
	else
	{
		RemoveStatusById(AtkBuffId);
		RemoveStatusById(AtkDebuffId);
	}
}

void UFCStatusPresenterComponent::HandleGameplayTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		AddStatusFromCatalogByTag(Tag, NewCount, 0.0f, 0.0f, true);
	}
	else
	{
		RemoveStatusByTag(Tag);
	}
}
