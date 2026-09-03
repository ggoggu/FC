#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Class/FCClassTypes.h"
#include "Combat/Element/FCElementStackContainer.h"
#include "FCElementComponent.generated.h"

/**
 * Overflow policy when adding element stacks beyond MaxTotalStacks (7)
 */
UENUM(BlueprintType)
enum class EFCElementOverflowPolicy : uint8
{
	FIFO        UMETA(DisplayName = "FIFO / 오래된 스택 밀어내기"),
	Clamp       UMETA(DisplayName = "Clamp / 상한선 도달 시 추가 불가")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnElementStacksChangedSignature, const TArray<EFCElement>&, CurrentStacks);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnElementStackAddedSignature, EFCElement, AddedElement, int32, NewTotalCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnElementStackConsumedSignature, EFCElement, ConsumedElement, int32, RemainingTotalCount);

/**
 * UFCElementComponent
 * 
 * Server-authoritative, replicated component managing element stacks (Fire, Earth, Water, etc.)
 * placed on characters/targets by Mage attack cards.
 * 
 * Rules:
 * - Stacks accumulate up to MaxTotalStacks (default: 7) regardless of element type.
 * - Supports FIFO (oldest pushed out) or Clamp overflow policies.
 * - FastArray serialization for optimal bandwidth and zero raw-TArray replication overhead.
 * - Provides atomic query, check, and consumption APIs for single-target and AoE cards.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FC_API UFCElementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFCElementComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Public Delegates (Client Presentation / UI / Niagara VFX) ---
	UPROPERTY(BlueprintAssignable, Category = "Element|Events")
	FOnElementStacksChangedSignature OnElementStacksChanged;

	UPROPERTY(BlueprintAssignable, Category = "Element|Events")
	FOnElementStackAddedSignature OnElementStackAdded;

	UPROPERTY(BlueprintAssignable, Category = "Element|Events")
	FOnElementStackConsumedSignature OnElementStackConsumed;

	// --- Stack Query APIs ---
	UFUNCTION(BlueprintPure, Category = "Element|Query")
	TArray<EFCElement> GetAllElementStacks() const;

	UFUNCTION(BlueprintPure, Category = "Element|Query")
	int32 GetTotalElementStacks() const { return StackContainer.Items.Num(); }

	UFUNCTION(BlueprintPure, Category = "Element|Query")
	int32 GetElementCount(EFCElement InElement) const;

	UFUNCTION(BlueprintPure, Category = "Element|Query")
	bool HasElementStack(EFCElement InElement, int32 RequiredCount = 1) const;

	/** Checks whether this component contains all the required elements (atomic check) */
	UFUNCTION(BlueprintPure, Category = "Element|Query")
	bool HasElementStacks(const TArray<EFCElement>& RequiredElements) const;

	UFUNCTION(BlueprintPure, Category = "Element|Query")
	int32 GetMaxTotalStacks() const { return MaxTotalStacks; }

	UFUNCTION(BlueprintPure, Category = "Element|Query")
	EFCElementOverflowPolicy GetOverflowPolicy() const { return OverflowPolicy; }

	// --- Server-Authoritative Stack Mutation APIs ---
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	bool AddElementStack(EFCElement InElement, int32 Count = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	void AddElementStacks(const TArray<EFCElement>& InElements);

	/**
	 * Consumes InCount stacks of InElement from the target.
	 * Returns true if successful, false if insufficient stacks (atomic: no stacks consumed on failure).
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	bool ConsumeElementStack(EFCElement InElement, int32 InCount = 1);

	/**
	 * Consumes all elements in RequiredElements from the target.
	 * Returns true if all were consumed, false if any were missing (atomic: no stacks consumed on failure).
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	bool ConsumeElementStacks(const TArray<EFCElement>& RequiredElements);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	int32 RemoveAllStacksOfElement(EFCElement InElement);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	void ClearAllElementStacks();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Configuration")
	void SetMaxTotalStacks(int32 InMaxStacks) { MaxTotalStacks = FMath::Max(1, InMaxStacks); }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Configuration")
	void SetOverflowPolicy(EFCElementOverflowPolicy InPolicy) { OverflowPolicy = InPolicy; }

	// Client callbacks from FastArraySerializer
	void NotifyStacksChanged();
	void NotifyStackItemAdded(EFCElement InElement);
	void NotifyStackItemRemoved(EFCElement InElement);

protected:
	/** Replicated FastArray container of active element stacks */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Element|Replication")
	FFCElementStackContainer StackContainer;

	/** Maximum total element stacks regardless of element type (default: 7) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|Rules", meta = (ClampMin = "1"))
	int32 MaxTotalStacks = 7;

	/** Overflow policy when adding element stacks beyond MaxTotalStacks */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|Rules")
	EFCElementOverflowPolicy OverflowPolicy = EFCElementOverflowPolicy::FIFO;

private:
	/** Internal single stack adder enforcing cap & policy */
	bool InternalAddSingleElement(EFCElement InElement);
};
