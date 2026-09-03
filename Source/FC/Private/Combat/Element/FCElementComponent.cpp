#include "Combat/Element/FCElementComponent.h"
#include "Net/UnrealNetwork.h"

UFCElementComponent::UFCElementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	StackContainer.OwnerComponent = this;
}

void UFCElementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UFCElementComponent, StackContainer, COND_None);
}

TArray<EFCElement> UFCElementComponent::GetAllElementStacks() const
{
	TArray<EFCElement> Result;
	Result.Reserve(StackContainer.Items.Num());
	for (const FFCElementStackItem& Item : StackContainer.Items)
	{
		Result.Add(Item.Element);
	}
	return Result;
}

int32 UFCElementComponent::GetElementCount(EFCElement InElement) const
{
	if (InElement == EFCElement::None)
	{
		return 0;
	}

	int32 Count = 0;
	for (const FFCElementStackItem& Item : StackContainer.Items)
	{
		if (Item.Element == InElement)
		{
			Count++;
		}
	}
	return Count;
}

bool UFCElementComponent::HasElementStack(EFCElement InElement, int32 RequiredCount) const
{
	if (RequiredCount <= 0)
	{
		return true;
	}

	return GetElementCount(InElement) >= RequiredCount;
}

bool UFCElementComponent::HasElementStacks(const TArray<EFCElement>& RequiredElements) const
{
	if (RequiredElements.Num() == 0)
	{
		return true;
	}

	// Count required quantities per element
	TMap<EFCElement, int32> RequiredMap;
	for (EFCElement Elem : RequiredElements)
	{
		if (Elem != EFCElement::None)
		{
			RequiredMap.FindOrAdd(Elem, 0)++;
		}
	}

	// Check if this component has enough for every required element
	for (const auto& Pair : RequiredMap)
	{
		if (GetElementCount(Pair.Key) < Pair.Value)
		{
			return false;
		}
	}

	return true;
}

bool UFCElementComponent::InternalAddSingleElement(EFCElement InElement)
{
	if (InElement == EFCElement::None)
	{
		return false;
	}

	if (StackContainer.Items.Num() >= MaxTotalStacks)
	{
		if (OverflowPolicy == EFCElementOverflowPolicy::Clamp)
		{
			return false;
		}

		// FIFO: Discard the oldest stack to maintain strictly capped total
		StackContainer.Items.RemoveAt(0);
	}

	FFCElementStackItem& NewItem = StackContainer.Items.AddDefaulted_GetRef();
	NewItem.Element = InElement;
	StackContainer.MarkItemDirty(NewItem);

	OnElementStackAdded.Broadcast(InElement, StackContainer.Items.Num());
	return true;
}

bool UFCElementComponent::AddElementStack(EFCElement InElement, int32 Count)
{
	if (Count <= 0 || InElement == EFCElement::None)
	{
		return false;
	}

	bool bAnyAdded = false;
	for (int32 i = 0; i < Count; ++i)
	{
		if (InternalAddSingleElement(InElement))
		{
			bAnyAdded = true;
		}
	}

	if (bAnyAdded)
	{
		NotifyStacksChanged();
	}

	return bAnyAdded;
}

void UFCElementComponent::AddElementStacks(const TArray<EFCElement>& InElements)
{
	if (InElements.Num() == 0)
	{
		return;
	}

	bool bAnyAdded = false;
	for (EFCElement Elem : InElements)
	{
		if (InternalAddSingleElement(Elem))
		{
			bAnyAdded = true;
		}
	}

	if (bAnyAdded)
	{
		NotifyStacksChanged();
	}
}

bool UFCElementComponent::ConsumeElementStack(EFCElement InElement, int32 InCount)
{
	if (InCount <= 0)
	{
		return true;
	}

	if (!HasElementStack(InElement, InCount))
	{
		return false;
	}

	int32 Removed = 0;
	// Remove instances from oldest to newest
	for (int32 i = 0; i < StackContainer.Items.Num() && Removed < InCount;)
	{
		if (StackContainer.Items[i].Element == InElement)
		{
			StackContainer.Items.RemoveAt(i);
			Removed++;
		}
		else
		{
			++i;
		}
	}

	StackContainer.MarkArrayDirty();
	OnElementStackConsumed.Broadcast(InElement, StackContainer.Items.Num());
	NotifyStacksChanged();
	return true;
}

bool UFCElementComponent::ConsumeElementStacks(const TArray<EFCElement>& RequiredElements)
{
	if (RequiredElements.Num() == 0)
	{
		return true;
	}

	// Atomic validation check
	if (!HasElementStacks(RequiredElements))
	{
		return false;
	}

	// Remove each required element instance
	for (EFCElement ReqElem : RequiredElements)
	{
		int32 Index = INDEX_NONE;
		for (int32 i = 0; i < StackContainer.Items.Num(); ++i)
		{
			if (StackContainer.Items[i].Element == ReqElem)
			{
				Index = i;
				break;
			}
		}

		if (Index != INDEX_NONE)
		{
			StackContainer.Items.RemoveAt(Index);
			OnElementStackConsumed.Broadcast(ReqElem, StackContainer.Items.Num());
		}
	}

	StackContainer.MarkArrayDirty();
	NotifyStacksChanged();
	return true;
}

int32 UFCElementComponent::RemoveAllStacksOfElement(EFCElement InElement)
{
	if (InElement == EFCElement::None)
	{
		return 0;
	}

	int32 RemovedCount = 0;
	for (int32 i = StackContainer.Items.Num() - 1; i >= 0; --i)
	{
		if (StackContainer.Items[i].Element == InElement)
		{
			StackContainer.Items.RemoveAt(i);
			RemovedCount++;
		}
	}

	if (RemovedCount > 0)
	{
		StackContainer.MarkArrayDirty();
		OnElementStackConsumed.Broadcast(InElement, StackContainer.Items.Num());
		NotifyStacksChanged();
	}
	return RemovedCount;
}

void UFCElementComponent::ClearAllElementStacks()
{
	if (StackContainer.Items.Num() > 0)
	{
		StackContainer.Items.Empty();
		StackContainer.MarkArrayDirty();
		NotifyStacksChanged();
	}
}

void UFCElementComponent::NotifyStacksChanged()
{
	OnElementStacksChanged.Broadcast(GetAllElementStacks());
}

void UFCElementComponent::NotifyStackItemAdded(EFCElement InElement)
{
	OnElementStackAdded.Broadcast(InElement, StackContainer.Items.Num());
	NotifyStacksChanged();
}

void UFCElementComponent::NotifyStackItemRemoved(EFCElement InElement)
{
	OnElementStackConsumed.Broadcast(InElement, StackContainer.Items.Num());
	NotifyStacksChanged();
}
