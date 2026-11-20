#include "Combat/Element/FCElementComponent.h"
#include "Card/FCCardExecutionContext.h"
#include "Card/Effects/FCCardEffectBase.h"
#include "Data/Class/FCClassSubsystem.h"
#include "Data/FCGameplayTags.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
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

bool UFCElementComponent::IsValidActiveElementTag(FGameplayTag Tag)
{
	return Tag.IsValid() && Tag != FCGameplayTags::Element_None && Tag != FCGameplayTags::Element_Root;
}

TArray<FGameplayTag> UFCElementComponent::GetAllElementStacks() const
{
	TArray<FGameplayTag> Result;
	Result.Reserve(StackContainer.Items.Num());
	for (const FFCElementStackItem& Item : StackContainer.Items)
	{
		Result.Add(Item.ElementTag);
	}
	return Result;
}

int32 UFCElementComponent::GetElementCount(FGameplayTag ElementTag) const
{
	if (!IsValidActiveElementTag(ElementTag))
	{
		return 0;
	}

	int32 Count = 0;
	for (const FFCElementStackItem& Item : StackContainer.Items)
	{
		if (Item.ElementTag == ElementTag)
		{
			Count++;
		}
	}
	return Count;
}

bool UFCElementComponent::HasElementStack(FGameplayTag ElementTag, int32 RequiredCount) const
{
	if (RequiredCount <= 0)
	{
		return true;
	}
	return GetElementCount(ElementTag) >= RequiredCount;
}

bool UFCElementComponent::HasElementStacks(const TArray<FGameplayTag>& RequiredElementTags) const
{
	if (RequiredElementTags.Num() == 0)
	{
		return true;
	}

	TMap<FGameplayTag, int32> RequiredMap;
	for (const FGameplayTag& ElemTag : RequiredElementTags)
	{
		if (IsValidActiveElementTag(ElemTag))
		{
			RequiredMap.FindOrAdd(ElemTag, 0)++;
		}
	}

	for (const auto& Pair : RequiredMap)
	{
		if (GetElementCount(Pair.Key) < Pair.Value)
		{
			return false;
		}
	}

	return true;
}

bool UFCElementComponent::InternalAddSingleElement(FGameplayTag InElementTag)
{
	if (!IsValidActiveElementTag(InElementTag))
	{
		return false;
	}

	if (StackContainer.Items.Num() >= MaxTotalStacks)
	{
		if (OverflowPolicy == EFCElementOverflowPolicy::Clamp)
		{
			return false;
		}

		StackContainer.Items.RemoveAt(0);
		StackContainer.MarkArrayDirty();
	}

	FFCElementStackItem& NewItem = StackContainer.Items.AddDefaulted_GetRef();
	NewItem.ElementTag = InElementTag;
	StackContainer.MarkItemDirty(NewItem);

	OnElementStackAdded.Broadcast(InElementTag, StackContainer.Items.Num());
	return true;
}

bool UFCElementComponent::AddElementStack(FGameplayTag ElementTag, int32 Count, AActor* InstigatorActor)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (Count <= 0 || !IsValidActiveElementTag(ElementTag))
	{
		return false;
	}

	bool bAnyAdded = false;
	for (int32 i = 0; i < Count; ++i)
	{
		if (InternalAddSingleElement(ElementTag))
		{
			bAnyAdded = true;
		}
	}

	if (bAnyAdded)
	{
		NotifyStacksChanged();
		EvaluateElementReactions(InstigatorActor);
	}

	return bAnyAdded;
}

void UFCElementComponent::AddElementStacks(const TArray<FGameplayTag>& InElementTags, AActor* InstigatorActor)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (InElementTags.Num() == 0)
	{
		return;
	}

	bool bAnyAdded = false;
	for (const FGameplayTag& ElemTag : InElementTags)
	{
		if (InternalAddSingleElement(ElemTag))
		{
			bAnyAdded = true;
		}
	}

	if (bAnyAdded)
	{
		NotifyStacksChanged();
		EvaluateElementReactions(InstigatorActor);
	}
}

bool UFCElementComponent::ConsumeElementStack(FGameplayTag ElementTag, int32 InCount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (InCount <= 0)
	{
		return true;
	}

	if (!HasElementStack(ElementTag, InCount))
	{
		return false;
	}

	int32 Removed = 0;
	for (int32 i = 0; i < StackContainer.Items.Num() && Removed < InCount;)
	{
		if (StackContainer.Items[i].ElementTag == ElementTag)
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
	OnElementStackConsumed.Broadcast(ElementTag, StackContainer.Items.Num());
	NotifyStacksChanged();
	return true;
}

bool UFCElementComponent::ConsumeElementStacks(const TArray<FGameplayTag>& RequiredElementTags)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (RequiredElementTags.Num() == 0)
	{
		return true;
	}

	if (!HasElementStacks(RequiredElementTags))
	{
		return false;
	}

	for (const FGameplayTag& ReqElem : RequiredElementTags)
	{
		int32 Index = INDEX_NONE;
		for (int32 i = 0; i < StackContainer.Items.Num(); ++i)
		{
			if (StackContainer.Items[i].ElementTag == ReqElem)
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

int32 UFCElementComponent::RemoveAllStacksOfElement(FGameplayTag ElementTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return 0;
	}

	if (!IsValidActiveElementTag(ElementTag))
	{
		return 0;
	}

	int32 RemovedCount = 0;
	for (int32 i = StackContainer.Items.Num() - 1; i >= 0; --i)
	{
		if (StackContainer.Items[i].ElementTag == ElementTag)
		{
			StackContainer.Items.RemoveAt(i);
			RemovedCount++;
		}
	}

	if (RemovedCount > 0)
	{
		StackContainer.MarkArrayDirty();
		OnElementStackConsumed.Broadcast(ElementTag, StackContainer.Items.Num());
		NotifyStacksChanged();
	}
	return RemovedCount;
}

void UFCElementComponent::ClearAllElementStacks()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (StackContainer.Items.Num() > 0)
	{
		StackContainer.Items.Empty();
		StackContainer.MarkArrayDirty();
		NotifyStacksChanged();
	}
}

bool UFCElementComponent::EvaluateElementReactions(AActor* InstigatorActor)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || bEvaluatingReactions || StackContainer.Items.Num() == 0)
	{
		return false;
	}

	const UFCClassSubsystem* ClassSubsystem = UFCClassSubsystem::GetClassSubsystem(this);
	if (!ClassSubsystem)
	{
		return false;
	}

	const TArray<FFCElementReactionRow>& Rules = ClassSubsystem->GetElementReactionRules();
	if (Rules.Num() == 0)
	{
		return false;
	}

	bEvaluatingReactions = true;
	bool bTriggeredAny = false;

	for (const FFCElementReactionRow& Rule : Rules)
	{
		if (Rule.RequiredElementTags.Num() == 0 || !HasElementStacks(Rule.RequiredElementTags))
		{
			continue;
		}

		if (Rule.bConsumeTriggerElements)
		{
			if (!ConsumeElementStacks(Rule.RequiredElementTags))
			{
				continue;
			}
		}

		FFCCardExecutionContext ReactionContext;
		ReactionContext.CasterActor = InstigatorActor ? InstigatorActor : OwnerActor;
		if (const IAbilitySystemInterface* CasterASI = Cast<IAbilitySystemInterface>(ReactionContext.CasterActor.Get()))
		{
			ReactionContext.CasterASC = CasterASI->GetAbilitySystemComponent();
		}
		ReactionContext.VerifiedTargets.Add(OwnerActor);
		ReactionContext.VerifiedAimLocation = OwnerActor->GetActorLocation();

		for (const TInstancedStruct<FFCCardEffectBase>& EffectStruct : Rule.ReactionEffects)
		{
			if (const FFCCardEffectBase* Effect = EffectStruct.GetPtr())
			{
				Effect->Execute(ReactionContext);
			}
		}

		OnElementReactionTriggered.Broadcast(Rule.ReactionId, OwnerActor);
		bTriggeredAny = true;
		break;
	}

	bEvaluatingReactions = false;
	return bTriggeredAny;
}

void UFCElementComponent::NotifyStacksChanged()
{
	OnElementStacksChanged.Broadcast(GetAllElementStacks());
}

void UFCElementComponent::NotifyStackItemAdded(FGameplayTag InElementTag)
{
	OnElementStackAdded.Broadcast(InElementTag, StackContainer.Items.Num());
	NotifyStacksChanged();
}

void UFCElementComponent::NotifyStackItemRemoved(FGameplayTag InElementTag)
{
	OnElementStackConsumed.Broadcast(InElementTag, StackContainer.Items.Num());
	NotifyStacksChanged();
}
