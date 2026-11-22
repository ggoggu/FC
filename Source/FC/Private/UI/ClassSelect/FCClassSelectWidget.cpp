#include "UI/ClassSelect/FCClassSelectWidget.h"
#include "UI/ClassSelect/FCClassSelectViewModel.h"
#include "UI/ClassSelect/FCClassSelectItemViewModel.h"
#include "UI/ClassSelect/FCClassSelectItemWidget.h"
#include "Data/Class/FCClassSubsystem.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UFCClassSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Confirm)
	{
		Btn_Confirm->OnClicked.RemoveDynamic(this, &UFCClassSelectWidget::HandleConfirmButtonClicked);
		Btn_Confirm->OnClicked.AddDynamic(this, &UFCClassSelectWidget::HandleConfirmButtonClicked);
	}
}

void UFCClassSelectWidget::NativeDestruct()
{
	if (ViewModel)
	{
		ViewModel->OnClassSelected.RemoveAll(this);
		ViewModel->OnClassConfirmed.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UFCClassSelectWidget::InitializeWidget(UFCClassSelectViewModel* InViewModel)
{
	if (ViewModel)
	{
		ViewModel->OnClassSelected.RemoveAll(this);
		ViewModel->OnClassConfirmed.RemoveAll(this);
	}

	ViewModel = InViewModel;

	if (ViewModel)
	{
		ViewModel->OnClassSelected.AddDynamic(this, &UFCClassSelectWidget::HandleViewModelClassSelected);
		ViewModel->OnClassConfirmed.AddDynamic(this, &UFCClassSelectWidget::HandleViewModelClassConfirmed);

		BuildClassItems();
		RefreshDetailPanel();
	}
}

TArray<FName> UFCClassSelectWidget::GetStartingDeckForClass(FGameplayTag InClassTag) const
{
	if (const FFCClassStartingDeckConfig* FoundConfig = StartingDeckOverrides.Find(InClassTag))
	{
		if (FoundConfig->CardIds.Num() > 0)
		{
			return FoundConfig->CardIds;
		}
	}

	if (const UFCClassSubsystem* ClassSub = UFCClassSubsystem::GetClassSubsystem(this))
	{
		return ClassSub->GetStartingDeckForClass(InClassTag);
	}

	if (const FFCCharacterClassData* BuiltinRow = UFCClassSubsystem::FindBuiltinClassDefault(InClassTag))
	{
		return BuiltinRow->StartingDeck;
	}

	return {};
}

void UFCClassSelectWidget::BuildClassItems()
{
	if (!ClassListContainer || !ViewModel)
	{
		return;
	}

	ClassListContainer->ClearChildren();
	SpawnedItemWidgets.Empty();

	if (!ItemWidgetClass)
	{
		return;
	}

	for (UFCClassSelectItemViewModel* ItemVM : ViewModel->ClassItemList)
	{
		if (!ItemVM)
		{
			continue;
		}

		UFCClassSelectItemWidget* ItemWidget = CreateWidget<UFCClassSelectItemWidget>(this, ItemWidgetClass);
		if (ItemWidget)
		{
			ItemWidget->InitializeWidget(ItemVM);
			ItemWidget->OnItemClicked.AddDynamic(this, &UFCClassSelectWidget::HandleClassItemClicked);
			ClassListContainer->AddChild(ItemWidget);
			SpawnedItemWidgets.Add(ItemWidget);
		}
	}
}

void UFCClassSelectWidget::HandleClassItemClicked(FGameplayTag ClickedClassTag)
{
	if (ViewModel)
	{
		ViewModel->SelectClass(ClickedClassTag);
	}
}

void UFCClassSelectWidget::HandleConfirmButtonClicked()
{
	if (!ViewModel)
	{
		return;
	}

	const FGameplayTag SelectedClassTag = ViewModel->SelectedClassTag;
	const TArray<FName> StartingDeck = GetStartingDeckForClass(SelectedClassTag);

	ViewModel->ConfirmSelection(GetOwningPlayer(), StartingDeck);
}

void UFCClassSelectWidget::HandleViewModelClassSelected(FGameplayTag SelectedClassTag)
{
	for (UFCClassSelectItemWidget* ItemWidget : SpawnedItemWidgets)
	{
		if (ItemWidget && ItemWidget->GetViewModel())
		{
			ItemWidget->InitializeWidget(ItemWidget->GetViewModel());
		}
	}

	RefreshDetailPanel();
	BP_OnClassSelected(SelectedClassTag);
}

void UFCClassSelectWidget::HandleViewModelClassConfirmed(FGameplayTag ConfirmedClassTag, const TArray<FName>& StartingDeck)
{
	BP_OnClassConfirmed(ConfirmedClassTag);
}

void UFCClassSelectWidget::RefreshDetailPanel()
{
	if (!ViewModel || !ViewModel->SelectedItem)
	{
		if (Btn_Confirm)
		{
			Btn_Confirm->SetIsEnabled(false);
		}
		return;
	}

	UFCClassSelectItemViewModel* SelectedItem = ViewModel->SelectedItem;

	if (Text_DetailName)
	{
		Text_DetailName->SetText(SelectedItem->ClassDisplayName);
	}

	if (Text_DetailDesc)
	{
		Text_DetailDesc->SetText(SelectedItem->ClassDescription);
	}

	if (Text_DetailStats)
	{
		const FString StatsString = FString::Printf(TEXT("HP: %.0f  |  MP: %.0f  |  ATK: %.0f"),
			SelectedItem->BaseMaxHealth,
			SelectedItem->BaseMaxMana,
			SelectedItem->BaseAttackPower);
		Text_DetailStats->SetText(FText::FromString(StatsString));
	}

	if (Text_DetailElements)
	{
		const FString ElemString = FString::Printf(TEXT("속성: %s"), *SelectedItem->AffinityElementsText.ToString());
		Text_DetailElements->SetText(FText::FromString(ElemString));
	}

	if (Btn_Confirm)
	{
		Btn_Confirm->SetIsEnabled(ViewModel->GetCanConfirmSelection());
	}
}
