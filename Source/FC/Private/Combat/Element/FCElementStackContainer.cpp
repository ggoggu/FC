#include "Combat/Element/FCElementStackContainer.h"
#include "Combat/Element/FCElementComponent.h"

void FFCElementStackItem::PostReplicatedAdd(const FFCElementStackContainer& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->NotifyStackItemAdded(Element);
	}
}

void FFCElementStackItem::PostReplicatedChange(const FFCElementStackContainer& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->NotifyStacksChanged();
	}
}

void FFCElementStackItem::PreReplicatedRemove(const FFCElementStackContainer& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->NotifyStackItemRemoved(Element);
	}
}
