#include "UI/ViewModel/FCStatusToggleContainerViewModel.h"
#include "UI/ViewModel/FCStatusToggleItemViewModel.h"

UFCStatusToggleItemViewModel* UFCStatusToggleContainerViewModel::AddOrUpdateStatus(const FFCStatusToggleData& InData)
{
	// 1. Check if an item already exists by Tag or Id
	UFCStatusToggleItemViewModel* ExistingItem = nullptr;
	if (InData.StatusTag.IsValid())
	{
		ExistingItem = FindStatusByTag(InData.StatusTag);
	}
	if (!ExistingItem && InData.StatusId != NAME_None)
	{
		ExistingItem = FindStatusById(InData.StatusId);
	}

	if (ExistingItem)
	{
		// Update existing item in-place without triggering full list rebuild
		ExistingItem->InitializeFromData(InData);
		return ExistingItem;
	}

	// 2. Instantiate new item ViewModel with this container as Outer
	UFCStatusToggleItemViewModel* NewItem = NewObject<UFCStatusToggleItemViewModel>(this);
	if (NewItem)
	{
		NewItem->InitializeFromData(InData);
		StatusItems.Add(NewItem);

		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StatusItems);
		OnStatusListUpdated.Broadcast();
	}

	return NewItem;
}

bool UFCStatusToggleContainerViewModel::RemoveStatusById(FName InStatusId)
{
	if (InStatusId == NAME_None)
	{
		return false;
	}

	const int32 RemovedCount = StatusItems.RemoveAll([InStatusId](const TObjectPtr<UFCStatusToggleItemViewModel>& Item)
	{
		return Item && Item->StatusId == InStatusId;
	});

	if (RemovedCount > 0)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StatusItems);
		OnStatusListUpdated.Broadcast();
		return true;
	}

	return false;
}

bool UFCStatusToggleContainerViewModel::RemoveStatusByTag(const FGameplayTag& InTag)
{
	if (!InTag.IsValid())
	{
		return false;
	}

	const int32 RemovedCount = StatusItems.RemoveAll([InTag](const TObjectPtr<UFCStatusToggleItemViewModel>& Item)
	{
		return Item && Item->StatusTag.MatchesTagExact(InTag);
	});

	if (RemovedCount > 0)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StatusItems);
		OnStatusListUpdated.Broadcast();
		return true;
	}

	return false;
}

void UFCStatusToggleContainerViewModel::ClearAllStatuses()
{
	if (StatusItems.Num() > 0)
	{
		StatusItems.Empty();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StatusItems);
		OnStatusListUpdated.Broadcast();
	}
}

UFCStatusToggleItemViewModel* UFCStatusToggleContainerViewModel::FindStatusById(FName InStatusId) const
{
	if (InStatusId == NAME_None)
	{
		return nullptr;
	}

	for (const auto& Item : StatusItems)
	{
		if (Item && Item->StatusId == InStatusId)
		{
			return Item;
		}
	}
	return nullptr;
}

UFCStatusToggleItemViewModel* UFCStatusToggleContainerViewModel::FindStatusByTag(const FGameplayTag& InTag) const
{
	if (!InTag.IsValid())
	{
		return nullptr;
	}

	for (const auto& Item : StatusItems)
	{
		if (Item && Item->StatusTag.MatchesTagExact(InTag))
		{
			return Item;
		}
	}
	return nullptr;
}

bool UFCStatusToggleContainerViewModel::SetStatusActive(FName InStatusId, bool bActive)
{
	if (UFCStatusToggleItemViewModel* Item = FindStatusById(InStatusId))
	{
		Item->SetIsActive(bActive);
		return true;
	}
	return false;
}

bool UFCStatusToggleContainerViewModel::SetStatusStack(FName InStatusId, int32 InStack)
{
	if (UFCStatusToggleItemViewModel* Item = FindStatusById(InStatusId))
	{
		Item->SetStackCount(InStack);
		return true;
	}
	return false;
}

bool UFCStatusToggleContainerViewModel::SetStatusDuration(FName InStatusId, float InRemaining, float InTotal)
{
	if (UFCStatusToggleItemViewModel* Item = FindStatusById(InStatusId))
	{
		Item->SetDuration(InRemaining, InTotal);
		return true;
	}
	return false;
}
