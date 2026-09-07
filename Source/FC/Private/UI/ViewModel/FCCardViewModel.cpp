#include "UI/ViewModel/FCCardViewModel.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardHandContainer.h"
#include "Data/Card/FCCardSubsystem.h"

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

	// Static Data Asset Resolution: If InDataAsset was not passed in, look it up via Subsystem
	const UFCCardDataAsset* StaticData = InDataAsset;
	if (!StaticData && !InItem.CardId.IsNone())
	{
		if (UFCCardSubsystem* Subsystem = UFCCardSubsystem::GetCardSubsystem(this))
		{
			StaticData = Subsystem->GetCardDataAsset(InItem.CardId);
		}
	}

	if (StaticData)
	{
		const FText RealCardName = !StaticData->DisplayData.CardName.IsEmpty()
			? StaticData->DisplayData.CardName
			: FText::FromName(!StaticData->GetCardId().IsNone() ? StaticData->GetCardId() : StaticData->GetFName());

		SetCardName(RealCardName);
		SetCardDescription(StaticData->DisplayData.CardDescription);
		SetCardIcon(StaticData->DisplayData.CardIcon);
		SetCardType(StaticData->GameplayData.CardType);
		SetRarity(StaticData->DisplayData.Rarity);
		SetManaCost(StaticData->GameplayData.BaseManaCost);
		SetRequiredClass(StaticData->GameplayData.RequiredClass);
		SetElements(StaticData->GameplayData.Elements);
		SetIsNeutral(StaticData->GameplayData.IsNeutral());
		SetFormattedClassText(FCClassTraitUtils::GetClassDisplayName(StaticData->GameplayData.RequiredClass));
		SetClassTraitTypeName(FCClassTraitUtils::GetTraitCategoryName(StaticData->GameplayData.RequiredClass));

		const FText FormattedTrait = StaticData->GameplayData.GetFormattedTraitText();
		SetClassTraitFormattedText(FormattedTrait);
		SetHasClassTrait(!FormattedTrait.IsEmpty());

		SetExhaustsOnPlay(StaticData->GameplayData.DoesExhaustOnPlay());
		const FText KeywordsText = StaticData->GameplayData.GetFormattedKeywordsText();
		SetFormattedKeywords(KeywordsText);
		SetHasKeywords(!KeywordsText.IsEmpty());
	}
	else
	{
		SetCardName(FText::FromName(InItem.CardId));
		SetCardDescription(FText::GetEmpty());
		SetRequiredClass(EFCCharacterClass::Neutral);
		SetElements({});
		SetIsNeutral(true);
		SetFormattedClassText(FCClassTraitUtils::GetClassDisplayName(EFCCharacterClass::Neutral));
		SetClassTraitTypeName(FCClassTraitUtils::GetTraitCategoryName(EFCCharacterClass::Neutral));
		SetClassTraitFormattedText(FText::GetEmpty());
		SetHasClassTrait(false);
		SetExhaustsOnPlay(false);
		SetFormattedKeywords(FText::GetEmpty());
		SetHasKeywords(false);
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
