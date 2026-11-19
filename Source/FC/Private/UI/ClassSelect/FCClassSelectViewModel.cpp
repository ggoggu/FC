#include "UI/ClassSelect/FCClassSelectViewModel.h"
#include "UI/ClassSelect/FCClassSelectItemViewModel.h"
#include "Data/Class/FCClassSubsystem.h"
#include "Data/Class/FCClassDataAsset.h"
#include "Data/FCPlayerPersistenceSubsystem.h"
#include "Game/FCPlayerState.h"

void UFCClassSelectViewModel::InitializeClasses(UFCClassSubsystem* Subsystem)
{
	ClassItemList.Empty();

	const TArray<EFCCharacterClass> DisplayClasses = {
		EFCCharacterClass::Mage,
		EFCCharacterClass::Warrior,
		EFCCharacterClass::Rogue,
		EFCCharacterClass::Priest
	};

	for (EFCCharacterClass ClassType : DisplayClasses)
	{
		UFCClassSelectItemViewModel* ItemVM = NewObject<UFCClassSelectItemViewModel>(this);
		if (!ItemVM)
		{
			continue;
		}

		FFCCharacterClassData ClassData;
		if (Subsystem)
		{
			if (const UFCClassDataAsset* DataAsset = Subsystem->GetClassDataAsset(ClassType))
			{
				ClassData = DataAsset->ClassData;
			}
		}

		// Fallback defaults if DataAsset not yet configured on disk
		if (ClassData.ClassDisplayName.IsEmpty())
		{
			ClassData.ClassType = ClassType;
			ClassData.ClassDisplayName = FCClassTraitUtils::GetClassDisplayName(ClassType);

			switch (ClassType)
			{
			case EFCCharacterClass::Mage:
				ClassData.ClassDescription = NSLOCTEXT("FCClass", "MageDefaultDesc", "다양한 원소 마법을 시전하여 원거리에서 폭발적인 화력을 투사합니다.");
				ClassData.AffinityElements = { EFCElement::Fire, EFCElement::Earth, EFCElement::Water, EFCElement::Wind, EFCElement::Lightning };
				ClassData.BaseMaxHealth = 80.0f;
				ClassData.BaseMaxMana = 100.0f;
				ClassData.BaseAttackPower = 0.0f;
				break;
			case EFCCharacterClass::Warrior:
				ClassData.ClassDescription = NSLOCTEXT("FCClass", "WarriorDefaultDesc", "단단한 방어구와 근접 무기로 전열을 지키는 굳건한 투사입니다.");
				ClassData.BaseMaxHealth = 150.0f;
				ClassData.BaseMaxMana = 30.0f;
				ClassData.BaseAttackPower = 15.0f;
				break;
			case EFCCharacterClass::Rogue:
				ClassData.ClassDescription = NSLOCTEXT("FCClass", "RogueDefaultDesc", "민첩한 몸놀림과 급소 공격으로 적을 기습하는 암살자입니다.");
				ClassData.BaseMaxHealth = 90.0f;
				ClassData.BaseMaxMana = 40.0f;
				ClassData.BaseAttackPower = 20.0f;
				break;
			case EFCCharacterClass::Priest:
				ClassData.ClassDescription = NSLOCTEXT("FCClass", "PriestDefaultDesc", "신성한 권능으로 아군을 보호하고 치유하는 성직자입니다.");
				ClassData.BaseMaxHealth = 100.0f;
				ClassData.BaseMaxMana = 80.0f;
				ClassData.BaseAttackPower = 5.0f;
				break;
			default:
				break;
			}
		}

		ItemVM->InitializeFromClassData(ClassData);
		ClassItemList.Add(ItemVM);
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ClassItemList);

	// Automatically pre-select Mage as default (since only Mage is selectable at this stage)
	SelectClass(EFCCharacterClass::Mage);
}

bool UFCClassSelectViewModel::SelectClass(EFCCharacterClass InClass)
{
	UFCClassSelectItemViewModel* TargetItem = nullptr;
	for (UFCClassSelectItemViewModel* Item : ClassItemList)
	{
		if (Item && Item->GetClassType() == InClass)
		{
			TargetItem = Item;
			break;
		}
	}

	if (!TargetItem)
	{
		return false;
	}

	// Enforce requirement: only selectable classes (Mage) can be selected
	if (!TargetItem->GetIsSelectable())
	{
		return false;
	}

	// Update selected state across all items
	for (UFCClassSelectItemViewModel* Item : ClassItemList)
	{
		if (Item)
		{
			Item->SetIsSelected(Item == TargetItem);
		}
	}

	SetSelectedItem(TargetItem);
	SetSelectedClassType(InClass);
	SetHasSelectedClass(true);

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCanConfirmSelection);
	OnClassSelected.Broadcast(InClass);
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

	// If no override provided, fallback to standard Mage deck
	if (FinalDeck.Num() == 0)
	{
		FinalDeck = {
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_AttackBuff"),
			FName("Card_AttackBuff"),
			FName("Card_AttackBuff"),
			FName("Card_MagicShield"),
			FName("Card_MagicShield")
		};
	}

	// Authoritatively update PlayerState if valid
	if (PC)
	{
		if (AFCPlayerState* PS = PC->GetPlayerState<AFCPlayerState>())
		{
			if (PC->HasAuthority())
			{
				PS->SetCharacterClass(SelectedClassType);
			}
		}

		// Store persistence snapshot
		if (UFCPlayerPersistenceSubsystem* Persistence = UFCPlayerPersistenceSubsystem::Get(PC))
		{
			const FString PlayerKey = Persistence->GetPlayerKey(PC);
			FFCCardDeckSaveData DeckSaveData;
			DeckSaveData.DrawPile = FinalDeck;

			FFCPlayerStatSaveData StatSaveData;
			StatSaveData.CharacterClass = SelectedClassType;
			StatSaveData.Health = SelectedItem->BaseMaxHealth;
			StatSaveData.MaxHealth = SelectedItem->BaseMaxHealth;
			StatSaveData.Mana = SelectedItem->BaseMaxMana;
			StatSaveData.MaxMana = SelectedItem->BaseMaxMana;
			StatSaveData.AttackPower = SelectedItem->BaseAttackPower;

			Persistence->SavePlayerData(PlayerKey, DeckSaveData, StatSaveData);
		}
	}

	OnClassConfirmed.Broadcast(SelectedClassType, FinalDeck);
	return true;
}

void UFCClassSelectViewModel::SetSelectedItem(UFCClassSelectItemViewModel* InItem)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(SelectedItem, InItem))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCanConfirmSelection);
	}
}

void UFCClassSelectViewModel::SetSelectedClassType(EFCCharacterClass InType)
{
	UE_MVVM_SET_PROPERTY_VALUE(SelectedClassType, InType);
}

void UFCClassSelectViewModel::SetHasSelectedClass(bool bInHas)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(bHasSelectedClass, bInHas))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCanConfirmSelection);
	}
}
