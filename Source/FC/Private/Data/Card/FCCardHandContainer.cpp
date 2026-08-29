#include "Data/Card/FCCardHandContainer.h"
#include "Components/ActorComponent.h"

void FFCCardItem::PostReplicatedAdd(const FFCCardHandContainer& InArraySerializer)
{
	// Client-side notification hook if needed
}

void FFCCardItem::PostReplicatedChange(const FFCCardHandContainer& InArraySerializer)
{
	// Client-side notification hook if needed
}

void FFCCardItem::PreReplicatedRemove(const FFCCardHandContainer& InArraySerializer)
{
	// Client-side notification hook if needed
}

FFCCardItem* FFCCardHandContainer::AddCard(FName InCardId, int32 InUpgradeLevel, bool bInLocked)
{
	if (InCardId.IsNone())
	{
		return nullptr;
	}

	FFCCardItem& NewItem = Items.Emplace_GetRef(FGuid::NewGuid(), InCardId, InUpgradeLevel, bInLocked);
	MarkItemDirty(NewItem);
	return &NewItem;
}

bool FFCCardHandContainer::RemoveCard(const FGuid& InCardGuid)
{
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		if (Items[Index].CardGuid == InCardGuid)
		{
			Items.RemoveAt(Index);
			MarkArrayDirty();
			return true;
		}
	}
	return false;
}

FFCCardItem* FFCCardHandContainer::FindCard(const FGuid& InCardGuid)
{
	for (FFCCardItem& Item : Items)
	{
		if (Item.CardGuid == InCardGuid)
		{
			return &Item;
		}
	}
	return nullptr;
}

const FFCCardItem* FFCCardHandContainer::FindCard(const FGuid& InCardGuid) const
{
	for (const FFCCardItem& Item : Items)
	{
		if (Item.CardGuid == InCardGuid)
		{
			return &Item;
		}
	}
	return nullptr;
}

bool FFCCardHandContainer::UpgradeCard(const FGuid& InCardGuid, int32 NewUpgradeLevel)
{
	FFCCardItem* Found = FindCard(InCardGuid);
	if (Found && Found->UpgradeLevel != NewUpgradeLevel)
	{
		Found->UpgradeLevel = NewUpgradeLevel;
		MarkItemDirty(*Found);
		return true;
	}
	return false;
}

bool FFCCardHandContainer::SetCardLocked(const FGuid& InCardGuid, bool bLocked)
{
	FFCCardItem* Found = FindCard(InCardGuid);
	if (Found && Found->bIsLocked != bLocked)
	{
		Found->bIsLocked = bLocked;
		MarkItemDirty(*Found);
		return true;
	}
	return false;
}

void FFCCardHandContainer::ClearCards()
{
	if (Items.Num() > 0)
	{
		Items.Empty();
		MarkArrayDirty();
	}
}
