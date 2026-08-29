#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "FCCardHandContainer.generated.h"

struct FFCCardHandContainer;
class UActorComponent;

/**
 * FFCCardItem
 * 
 * Replicated runtime instance of a single card in hand.
 * Utilizes Fast Array serialization for minimal network delta payloads.
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardItem : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FFCCardItem()
		: CardGuid(FGuid())
		, CardId(NAME_None)
		, UpgradeLevel(0)
		, bIsLocked(false)
	{
	}

	FFCCardItem(const FGuid& InGuid, FName InCardId, int32 InUpgradeLevel = 0, bool bInLocked = false)
		: CardGuid(InGuid)
		, CardId(InCardId)
		, UpgradeLevel(InUpgradeLevel)
		, bIsLocked(bInLocked)
	{
	}

	/** Unique runtime instance identifier */
	UPROPERTY(BlueprintReadOnly, Category = "Card|Instance")
	FGuid CardGuid;

	/** Static definition ID matching UFCCardDataAsset */
	UPROPERTY(BlueprintReadOnly, Category = "Card|Instance")
	FName CardId = NAME_None;

	/** Card upgrade tier (0 = Base, 1 = +1, etc.) */
	UPROPERTY(BlueprintReadOnly, Category = "Card|Instance")
	int32 UpgradeLevel = 0;

	/** Whether this card is temporarily sealed/locked from being played */
	UPROPERTY(BlueprintReadOnly, Category = "Card|Instance")
	bool bIsLocked = false;

	// Fast Array replication lifecycle callbacks
	void PostReplicatedAdd(const FFCCardHandContainer& InArraySerializer);
	void PostReplicatedChange(const FFCCardHandContainer& InArraySerializer);
	void PreReplicatedRemove(const FFCCardHandContainer& InArraySerializer);
};

/**
 * FFCCardHandContainer
 * 
 * Fast Array Serializer container managing active hand cards.
 * Replicated COND_OwnerOnly to ensure strict anti-cheat security.
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardHandContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	FFCCardHandContainer()
		: OwnerComponent(nullptr)
	{
	}

	UPROPERTY()
	TArray<FFCCardItem> Items;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FFCCardItem, FFCCardHandContainer>(Items, DeltaParms, *this);
	}

	// Server Mutation API
	FFCCardItem* AddCard(FName InCardId, int32 InUpgradeLevel = 0, bool bInLocked = false);
	bool RemoveCard(const FGuid& InCardGuid);
	FFCCardItem* FindCard(const FGuid& InCardGuid);
	const FFCCardItem* FindCard(const FGuid& InCardGuid) const;
	bool UpgradeCard(const FGuid& InCardGuid, int32 NewUpgradeLevel);
	bool SetCardLocked(const FGuid& InCardGuid, bool bLocked);
	void ClearCards();

	int32 Num() const { return Items.Num(); }
};

template<>
struct TStructOpsTypeTraits<FFCCardHandContainer> : public TStructOpsTypeTraitsBase2<FFCCardHandContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
