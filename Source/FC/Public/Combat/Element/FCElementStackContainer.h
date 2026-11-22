#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "FCElementStackContainer.generated.h"

struct FFCElementStackContainer;
class UFCElementComponent;

/**
 * FFCElementStackItem
 *
 * Replicated runtime instance of a single element stack token on an actor.
 * Uses FGameplayTag (compact NetIndex bit-packed) + FastArray delta serialization.
 */
USTRUCT(BlueprintType)
struct FC_API FFCElementStackItem : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FFCElementStackItem() = default;

	explicit FFCElementStackItem(FGameplayTag InElementTag)
		: ElementTag(InElementTag)
	{
	}

	/** Element GameplayTag of this stack (e.g. Element.Fire, Element.Earth) */
	UPROPERTY(BlueprintReadOnly, Category = "Element")
	FGameplayTag ElementTag;

	void PostReplicatedAdd(const FFCElementStackContainer& InArraySerializer);
	void PostReplicatedChange(const FFCElementStackContainer& InArraySerializer);
	void PreReplicatedRemove(const FFCElementStackContainer& InArraySerializer);
};

/**
 * FFCElementStackContainer
 *
 * Fast Array Serializer container holding active element stack tags up to MaxTotalStacks.
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
