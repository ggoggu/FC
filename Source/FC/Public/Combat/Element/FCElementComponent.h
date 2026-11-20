#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnElementStacksChangedSignature, const TArray<FGameplayTag>&, CurrentStacks);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnElementStackAddedSignature, FGameplayTag, AddedElementTag, int32, NewTotalCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnElementStackConsumedSignature, FGameplayTag, ConsumedElementTag, int32, RemainingTotalCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnElementReactionTriggeredSignature, FName, ReactionId, AActor*, TargetActor);

/**
 * UFCElementComponent
 *
 * Server-authoritative, replicated component managing GameplayTag-based element stacks (Element.*)
 * with FastArray serialization, configurable overflow policy, and DataTable-driven elemental reaction evaluation.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FC_API UFCElementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFCElementComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Public Delegates ---
	UPROPERTY(BlueprintAssignable, Category = "Element|Events")
	FOnElementStacksChangedSignature OnElementStacksChanged;

	UPROPERTY(BlueprintAssignable, Category = "Element|Events")
	FOnElementStackAddedSignature OnElementStackAdded;

	UPROPERTY(BlueprintAssignable, Category = "Element|Events")
	FOnElementStackConsumedSignature OnElementStackConsumed;

	UPROPERTY(BlueprintAssignable, Category = "Element|Events")
	FOnElementReactionTriggeredSignature OnElementReactionTriggered;

	// --- Stack Query APIs (GameplayTag SSOT) ---
	UFUNCTION(BlueprintPure, Category = "Element|Query")
	TArray<FGameplayTag> GetAllElementStacks() const;

	UFUNCTION(BlueprintPure, Category = "Element|Query")
	int32 GetTotalElementStacks() const { return StackContainer.Items.Num(); }

	UFUNCTION(BlueprintPure, Category = "Element|Query")
	int32 GetElementCount(FGameplayTag ElementTag) const;

	UFUNCTION(BlueprintPure, Category = "Element|Query")
	int32 GetElementTagCount(FGameplayTag ElementTag) const { return GetElementCount(ElementTag); }

	UFUNCTION(BlueprintPure, Category = "Element|Query")
	bool HasElementStack(FGameplayTag ElementTag, int32 RequiredCount = 1) const;

	UFUNCTION(BlueprintPure, Category = "Element|Query")
	bool HasElementTagStack(FGameplayTag ElementTag, int32 RequiredCount = 1) const { return HasElementStack(ElementTag, RequiredCount); }

	UFUNCTION(BlueprintPure, Category = "Element|Query")
	bool HasElementStacks(const TArray<FGameplayTag>& RequiredElementTags) const;

	UFUNCTION(BlueprintPure, Category = "Element|Query")
	bool HasElementTagStacks(const TArray<FGameplayTag>& RequiredElementTags) const { return HasElementStacks(RequiredElementTags); }

	UFUNCTION(BlueprintPure, Category = "Element|Query")
	int32 GetMaxTotalStacks() const { return MaxTotalStacks; }

	UFUNCTION(BlueprintPure, Category = "Element|Query")
	EFCElementOverflowPolicy GetOverflowPolicy() const { return OverflowPolicy; }

	// --- Server-Authoritative Stack Mutation APIs ---
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	bool AddElementStack(FGameplayTag ElementTag, int32 Count = 1, AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	bool AddElementTagStack(FGameplayTag ElementTag, int32 Count = 1)
	{
		if (!GetOwner() || !GetOwner()->HasAuthority())
		{
			return false;
		}
		return AddElementStack(ElementTag, Count, nullptr);
	}

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	void AddElementStacks(const TArray<FGameplayTag>& InElementTags, AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	void AddElementTagStacks(const TArray<FGameplayTag>& InElementTags)
	{
		if (!GetOwner() || !GetOwner()->HasAuthority())
		{
			return;
		}
		AddElementStacks(InElementTags, nullptr);
	}

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	bool ConsumeElementStack(FGameplayTag ElementTag, int32 InCount = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	bool ConsumeElementStacks(const TArray<FGameplayTag>& RequiredElementTags);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	bool ConsumeElementTagStacks(const TArray<FGameplayTag>& RequiredElementTags)
	{
		if (!GetOwner() || !GetOwner()->HasAuthority())
		{
			return false;
		}
		return ConsumeElementStacks(RequiredElementTags);
	}

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	int32 RemoveAllStacksOfElement(FGameplayTag ElementTag);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	int32 RemoveAllStacksOfElementTag(FGameplayTag ElementTag)
	{
		if (!GetOwner() || !GetOwner()->HasAuthority())
		{
			return 0;
		}
		return RemoveAllStacksOfElement(ElementTag);
	}

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Mutation")
	void ClearAllElementStacks();

	/** Evaluates DataTable-registered elemental reactions (DT_ElementReactions) on the authoritative server */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Reaction")
	bool EvaluateElementReactions(AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Configuration")
	void SetMaxTotalStacks(int32 InMaxStacks)
	{
		if (!GetOwner() || !GetOwner()->HasAuthority())
		{
			return;
		}
		MaxTotalStacks = FMath::Max(1, InMaxStacks);
	}

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Element|Configuration")
	void SetOverflowPolicy(EFCElementOverflowPolicy InPolicy)
	{
		if (!GetOwner() || !GetOwner()->HasAuthority())
		{
			return;
		}
		OverflowPolicy = InPolicy;
	}

	// Client callbacks from FastArraySerializer
	void NotifyStacksChanged();
	void NotifyStackItemAdded(FGameplayTag InElementTag);
	void NotifyStackItemRemoved(FGameplayTag InElementTag);

protected:
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Element|Replication")
	FFCElementStackContainer StackContainer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|Rules", meta = (ClampMin = "1"))
	int32 MaxTotalStacks = 7;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|Rules")
	EFCElementOverflowPolicy OverflowPolicy = EFCElementOverflowPolicy::FIFO;

private:
	bool InternalAddSingleElement(FGameplayTag InElementTag);
	static bool IsValidActiveElementTag(FGameplayTag Tag);
	bool bEvaluatingReactions = false;
};
