#include "UI/ClassSelect/FCClassSelectViewModel.h"
#include "UI/ClassSelect/FCClassSelectItemViewModel.h"
#include "Data/Class/FCClassSubsystem.h"
#include "Data/FCGameplayTags.h"
#include "Data/FCPlayerPersistenceSubsystem.h"
#include "Game/FCPlayerState.h"

void UFCClassSelectViewModel::InitializeClasses(UFCClassSubsystem* Subsystem)
{
	ClassItemList.Empty();

	TArray<FGameplayTag> DisplayClassTags;
	if (Subsystem)
	{
		DisplayClassTags = Subsystem->GetPlayableClassTags();
	}
	else
	{
		DisplayClassTags = {
			FCGameplayTags::Class_Mage.GetTag(),
			FCGameplayTags::Class_Warrior.GetTag(),
			FCGameplayTags::Class_Rogue.GetTag(),
			FCGameplayTags::Class_Priest.GetTag()
		};
	}

	FGameplayTag FirstSelectableTag;
	for (const FGameplayTag& ClassTag : DisplayClassTags)
	{
		const FFCCharacterClassData* ClassRow = Subsystem
			? Subsystem->FindClassRowPtr(ClassTag)
			: UFCClassSubsystem::FindBuiltinClassDefault(ClassTag);

		if (!ClassRow)
		{
			continue;
		}

		UFCClassSelectItemViewModel* ItemVM = NewObject<UFCClassSelectItemViewModel>(this);
		if (!ItemVM)
		{
			continue;
		}

		ItemVM->InitializeFromClassData(*ClassRow, Subsystem ? static_cast<const UObject*>(Subsystem) : static_cast<const UObject*>(this));
		ClassItemList.Add(ItemVM);

		if (!FirstSelectableTag.IsValid() && ItemVM->GetIsSelectable())
		{
			FirstSelectableTag = ClassTag;
		}
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ClassItemList);

	if (FirstSelectableTag.IsValid())
	{
		SelectClass(FirstSelectableTag);
	}
}

bool UFCClassSelectViewModel::SelectClass(FGameplayTag InClassTag)
{
	UFCClassSelectItemViewModel* TargetItem = nullptr;
	for (UFCClassSelectItemViewModel* Item : ClassItemList)
	{
		if (Item && Item->GetClassTag() == InClassTag)
		{
			TargetItem = Item;
			break;
		}
	}

	if (!TargetItem || !TargetItem->GetIsSelectable())
	{
		return false;
	}

	for (UFCClassSelectItemViewModel* Item : ClassItemList)
	{
		if (Item)
		{
			Item->SetIsSelected(Item == TargetItem);
		}
	}

	SetSelectedItem(TargetItem);
	SetSelectedClassTag(InClassTag);
	SetHasSelectedClass(true);

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCanConfirmSelection);
	OnClassSelected.Broadcast(InClassTag);
	return true;
}

bool UFCClassSelectViewModel::GetCanConfirmSelection() const
{
	return bHasSelectedClass && (SelectedItem != nullptr) && SelectedItem->GetIsSelectable();
}

bool UFCClassSelectViewModel::ConfirmSelection(APlayerController* PC, const TArray<FName>& StartingDeckOverride)
{
	if (!GetCanConfirmSelection() || !SelectedItem)
	{
		return false;
	}

	TArray<FName> FinalDeck = StartingDeckOverride;

	if (FinalDeck.Num() == 0)
	{
		const UObject* WorldCtx = PC ? static_cast<const UObject*>(PC) : static_cast<const UObject*>(this);
		if (const UFCClassSubsystem* ClassSub = UFCClassSubsystem::GetClassSubsystem(WorldCtx))
		{
			FinalDeck = ClassSub->GetStartingDeckForClass(SelectedClassTag);
		}
		else if (const FFCCharacterClassData* BuiltinRow = UFCClassSubsystem::FindBuiltinClassDefault(SelectedClassTag))
		{
			FinalDeck = BuiltinRow->StartingDeck;
		}
	}

	if (PC)
	{
		if (AFCPlayerState* PS = PC->GetPlayerState<AFCPlayerState>())
		{
			if (PC->HasAuthority())
			{
				PS->SetCharacterClass(SelectedClassTag);
			}
		}

		if (UFCPlayerPersistenceSubsystem* Persistence = UFCPlayerPersistenceSubsystem::Get(PC))
		{
			const FString PlayerKey = Persistence->GetPlayerKey(PC);
			FFCCardDeckSaveData DeckSaveData;
			DeckSaveData.DrawPile = FinalDeck;

			FFCPlayerStatSaveData StatSaveData;
			StatSaveData.CharacterClass = SelectedClassTag;
			StatSaveData.Health = SelectedItem->BaseMaxHealth;
			StatSaveData.MaxHealth = SelectedItem->BaseMaxHealth;
			StatSaveData.Mana = SelectedItem->BaseMaxMana;
			StatSaveData.MaxMana = SelectedItem->BaseMaxMana;
			StatSaveData.AttackPower = SelectedItem->BaseAttackPower;

			Persistence->SavePlayerData(PlayerKey, DeckSaveData, StatSaveData);
		}
	}

	OnClassConfirmed.Broadcast(SelectedClassTag, FinalDeck);
	return true;
}

void UFCClassSelectViewModel::SetSelectedItem(UFCClassSelectItemViewModel* InItem)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(SelectedItem, InItem))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCanConfirmSelection);
	}
}

void UFCClassSelectViewModel::SetSelectedClassTag(FGameplayTag InTag)
{
	UE_MVVM_SET_PROPERTY_VALUE(SelectedClassTag, InTag);
}

void UFCClassSelectViewModel::SetHasSelectedClass(bool bInHas)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(bHasSelectedClass, bInHas))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCanConfirmSelection);
	}
}
