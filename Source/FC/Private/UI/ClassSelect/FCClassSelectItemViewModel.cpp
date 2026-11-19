#include "UI/ClassSelect/FCClassSelectItemViewModel.h"

void UFCClassSelectItemViewModel::InitializeFromClassData(const FFCCharacterClassData& InData)
{
	SetClassType(InData.ClassType);
	SetClassDisplayName(InData.ClassDisplayName.IsEmpty() ? FCClassTraitUtils::GetClassDisplayName(InData.ClassType) : InData.ClassDisplayName);
	SetClassDescription(InData.ClassDescription);
	SetClassIcon(InData.ClassIcon);
	SetBaseMaxHealth(InData.BaseMaxHealth);
	SetBaseMaxMana(InData.BaseMaxMana);
	SetBaseAttackPower(InData.BaseAttackPower);

	// Format elements text
	if (InData.AffinityElements.Num() > 0)
	{
		SetAffinityElementsText(FCClassTraitUtils::FormatMageElementsText(InData.AffinityElements));
	}
	else
	{
		SetAffinityElementsText(NSLOCTEXT("FCClassSelect", "NoAffinity", "없음"));
	}

	// Requirement: Currently only Mage is selectable
	const bool bSelectable = (InData.ClassType == EFCCharacterClass::Mage);
	SetIsSelectable(bSelectable);
	SetIsSelected(false);

	if (!bSelectable)
	{
		SetLockReasonText(NSLOCTEXT("FCClassSelect", "ClassLocked", "준비 중 (현재 마법사만 선택 가능)"));
	}
	else
	{
		SetLockReasonText(FText::GetEmpty());
	}
}
