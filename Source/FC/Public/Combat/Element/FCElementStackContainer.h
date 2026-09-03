#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Data/Class/FCClassTypes.h"
#include "FCElementStackContainer.generated.h"

struct FFCElementStackContainer;
class UFCElementComponent;

/**
 * FFCElementStackItem
 * 
 * Replicated runtime instance of a single element stack token on a character.
 * Uses Fast Array Serialization for optimal network delta bandwidth.
 */
USTRUCT(BlueprintType)
struct FC_API FFCElementStackItem : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FFCElementStackItem()
		: Element(EFCElement::None)
	{
	}

	FFCElementStackItem(EFCElement InElement)
		: Element(InElement)
	{
	}

	/** Element type of this stack */
	UPROPERTY(BlueprintReadOnly, Category = "Element")
	EFCElement Element = EFCElement::None;

	// Fast Array replication lifecycle callbacks
	void PostReplicatedAdd(const FFCElementStackContainer& InArraySerializer);
	void PostReplicatedChange(const FFCElementStackContainer& InArraySerializer);
	void PreReplicatedRemove(const FFCElementStackContainer& InArraySerializer);
};

/**
 * FFCElementStackContainer
 * 
 * Fast Array Serializer container holding active element stacks up to 7 items.
 */
USTRUCT(BlueprintType)
struct FC_API FFCElementStackContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	FFCElementStackContainer()
		: OwnerComponent(nullptr)
	{
	}

	UPROPERTY()
	TArray<FFCElementStackItem> Items;

	UPROPERTY(NotReplicated)
	TObjectPtr<UFCElementComponent> OwnerComponent = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FFCElementStackItem, FFCElementStackContainer>(Items, DeltaParms, *this);
	}

	int32 Num() const { return Items.Num(); }
};

template<>
struct TStructOpsTypeTraits<FFCElementStackContainer> : public TStructOpsTypeTraitsBase2<FFCElementStackContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
