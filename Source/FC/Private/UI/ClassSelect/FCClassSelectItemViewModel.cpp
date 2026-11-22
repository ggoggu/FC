#include "UI/ClassSelect/FCClassSelectItemViewModel.h"

void UFCClassSelectItemViewModel::InitializeFromClassData(const FFCCharacterClassData& InData, const UObject* WorldContextObject)
{
	SetClassTag(InData.ClassTag);
	SetClassDisplayName(InData.ClassDisplayName.IsEmpty()
		? FCClassTraitUtils::GetClassDisplayName(InData.ClassTag, WorldContextObject)
		: InData.ClassDisplayName);
	SetClassDescription(InData.ClassDescription);
	SetClassIcon(InData.ClassIcon);
	SetBaseMaxHealth(InData.BaseMaxHealth);
	SetBaseMaxMana(InData.BaseMaxMana);
	SetBaseAttackPower(InData.BaseAttackPower);

	if (InData.AffinityElementTags.Num() > 0)
	{
		SetAffinityElementsText(FCClassTraitUtils::FormatElementTagsText(InData.AffinityElementTags, WorldContextObject));
	}
	else
	{
		SetAffinityElementsText(NSLOCTEXT("FCClassSelect", "NoAffinity", "없음"));
	}

	SetIsSelectable(InData.bIsSelectable);
	SetIsSelected(false);
	SetLockReasonText(InData.bIsSelectable ? FText::GetEmpty() : InData.LockReasonText);
}
