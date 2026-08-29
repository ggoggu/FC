#include "UI/ViewModel/FCCardViewModel.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardHandContainer.h"

void UFCCardViewModel::SetUpgradeLevel(int32 InLevel)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(UpgradeLevel, InLevel))
	{
		UE_MVVM_SET_PROPERTY_VALUE(bIsUpgraded, UpgradeLevel > 0);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetFormattedDisplayName);
	}
}

void UFCCardViewModel::SetIsLocked(bool bInLocked)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(bIsLocked, bInLocked))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCanPlay);
	}
}

void UFCCardViewModel::SetManaCost(int32 InCost)
{
	UE_MVVM_SET_PROPERTY_VALUE(ManaCost, InCost);
}

void UFCCardViewModel::SetCardName(const FText& InName)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(CardName, InName))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetFormattedDisplayName);
	}
}

void UFCCardViewModel::SetIsPlayable(bool bInPlayable)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(bIsPlayable, bInPlayable))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCanPlay);
	}
}

void UFCCardViewModel::InitializeFromCardItem(const FFCCardItem& InItem, const UFCCardDataAsset* InDataAsset)
{
	SetCardGuid(InItem.CardGuid);
	SetCardId(InItem.CardId);
	SetUpgradeLevel(InItem.UpgradeLevel);
	SetIsLocked(InItem.bIsLocked);

	if (InDataAsset)
	{
		SetCardName(InDataAsset->DisplayData.CardName.IsEmpty() ? FText::FromName(InDataAsset->GetCardId()) : InDataAsset->DisplayData.CardName);
		SetCardDescription(InDataAsset->DisplayData.CardDescription);
		SetCardIcon(InDataAsset->DisplayData.CardIcon);
		SetCardType(InDataAsset->GameplayData.CardType);
		SetRarity(InDataAsset->DisplayData.Rarity);
		SetManaCost(InDataAsset->GameplayData.BaseManaCost);
	}
	else
	{
		SetCardName(FText::FromName(InItem.CardId));
	}
}

FText UFCCardViewModel::GetFormattedDisplayName() const
{
	if (UpgradeLevel > 0)
	{
		return FText::Format(NSLOCTEXT("FC_Card", "UpgradedName", "{0} +{1}"), CardName, FText::AsNumber(UpgradeLevel));
	}
	return CardName;
}

bool UFCCardViewModel::GetCanPlay() const
{
	return bIsPlayable && !bIsLocked;
}
