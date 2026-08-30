#include "Card/FCCardDeckComponent.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardSubsystem.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

UFCCardDeckComponent::UFCCardDeckComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	HandContainer.OwnerComponent = this;
}

void UFCCardDeckComponent::BeginPlay()
{
	Super::BeginPlay();
	HandContainer.OwnerComponent = this;
}

void UFCCardDeckComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, HandContainer, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, DrawPileCount, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, DiscardPileCount, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, ExhaustPileCount, COND_OwnerOnly);
}

void UFCCardDeckComponent::InitializeDeck(const TArray<FName>& StartingDeck)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	ServerDrawPile = StartingDeck;
	ServerDiscardPile.Empty();
	ServerExhaustPile.Empty();
	HandContainer.ClearCards();

	// Shuffle starting draw pile
	const int32 NumCards = ServerDrawPile.Num();
	for (int32 i = 0; i < NumCards; ++i)
	{
		int32 SwapIdx = FMath::RandRange(0, NumCards - 1);
		ServerDrawPile.Swap(i, SwapIdx);
	}

	DrawPileCount = ServerDrawPile.Num();
	DiscardPileCount = 0;
	ExhaustPileCount = 0;
}

void UFCCardDeckComponent::DrawCards(int32 Count)
{
	if (!GetOwner()->HasAuthority() || Count <= 0)
	{
		return;
	}

	for (int32 i = 0; i < Count; ++i)
	{
		if (ServerDrawPile.Num() == 0)
		{
			ReshuffleDiscardIntoDraw();
		}

		if (ServerDrawPile.Num() == 0)
		{
			break; // No cards left in draw or discard pile
		}

		FName DrawnCardId = ServerDrawPile.Pop();
		HandContainer.AddCard(DrawnCardId);
	}

	DrawPileCount = ServerDrawPile.Num();
	DiscardPileCount = ServerDiscardPile.Num();
	ExhaustPileCount = ServerExhaustPile.Num();

	OnCardHandUpdated.Broadcast();
	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);
}

bool UFCCardDeckComponent::DiscardCard(const FGuid& CardGuid)
{
	if (!GetOwner()->HasAuthority())
	{
		return false;
	}

	const FFCCardItem* FoundItem = HandContainer.FindCard(CardGuid);
	if (!FoundItem)
	{
		return false;
	}

	ServerDiscardPile.Add(FoundItem->CardId);
	HandContainer.RemoveCard(CardGuid);

	DiscardPileCount = ServerDiscardPile.Num();
	OnCardHandUpdated.Broadcast();
	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);
	return true;
}

bool UFCCardDeckComponent::ExhaustCard(const FGuid& CardGuid)
{
	if (!GetOwner()->HasAuthority())
	{
		return false;
	}

	const FFCCardItem* FoundItem = HandContainer.FindCard(CardGuid);
	if (!FoundItem)
	{
		return false;
	}

	ServerExhaustPile.Add(FoundItem->CardId);
	HandContainer.RemoveCard(CardGuid);

	ExhaustPileCount = ServerExhaustPile.Num();
	OnCardHandUpdated.Broadcast();
	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);
	return true;
}

bool UFCCardDeckComponent::UpgradeCardInHand(const FGuid& CardGuid, int32 NewUpgradeLevel)
{
	if (!GetOwner()->HasAuthority())
	{
		return false;
	}

	bool bResult = HandContainer.UpgradeCard(CardGuid, NewUpgradeLevel);
	if (bResult)
	{
		if (const FFCCardItem* Item = HandContainer.FindCard(CardGuid))
		{
			OnCardItemChanged.Broadcast(*Item);
		}
		OnCardHandUpdated.Broadcast();
	}
	return bResult;
}

bool UFCCardDeckComponent::SetCardLockedInHand(const FGuid& CardGuid, bool bLocked)
{
	if (!GetOwner()->HasAuthority())
	{
		return false;
	}

	bool bResult = HandContainer.SetCardLocked(CardGuid, bLocked);
	if (bResult)
	{
		if (const FFCCardItem* Item = HandContainer.FindCard(CardGuid))
		{
			OnCardItemChanged.Broadcast(*Item);
		}
		OnCardHandUpdated.Broadcast();
	}
	return bResult;
}

void UFCCardDeckComponent::ReshuffleDiscardIntoDraw()
{
	if (!GetOwner()->HasAuthority() || ServerDiscardPile.Num() == 0)
	{
		return;
	}

	ServerDrawPile.Append(ServerDiscardPile);
	ServerDiscardPile.Empty();

	const int32 NumCards = ServerDrawPile.Num();
	for (int32 i = 0; i < NumCards; ++i)
	{
		int32 SwapIdx = FMath::RandRange(0, NumCards - 1);
		ServerDrawPile.Swap(i, SwapIdx);
	}

	DrawPileCount = ServerDrawPile.Num();
	DiscardPileCount = ServerDiscardPile.Num();
}

void UFCCardDeckComponent::DiscardEntireHand()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	for (const FFCCardItem& Item : HandContainer.Items)
	{
		ServerDiscardPile.Add(Item.CardId);
	}

	HandContainer.ClearCards();
	DiscardPileCount = ServerDiscardPile.Num();

	OnCardHandUpdated.Broadcast();
	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);
}

bool UFCCardDeckComponent::AddCardToDeck(FName CardId, EFCCardAddDestination Destination, bool bShuffleIfDrawPile)
{
	if (!GetOwner()->HasAuthority() || CardId.IsNone())
	{
		return false;
	}

	UFCCardSubsystem* Subsystem = UFCCardSubsystem::GetCardSubsystem(this);
	if (!Subsystem || !Subsystem->GetCardDataAsset(CardId))
	{
		return false;
	}

	switch (Destination)
	{
	case EFCCardAddDestination::DrawPile:
		ServerDrawPile.Add(CardId);
		if (bShuffleIfDrawPile)
		{
			const int32 NumCards = ServerDrawPile.Num();
			for (int32 i = 0; i < NumCards; ++i)
			{
				int32 SwapIdx = FMath::RandRange(0, NumCards - 1);
				ServerDrawPile.Swap(i, SwapIdx);
			}
		}
		DrawPileCount = ServerDrawPile.Num();
		break;

	case EFCCardAddDestination::DiscardPile:
		ServerDiscardPile.Add(CardId);
		DiscardPileCount = ServerDiscardPile.Num();
		break;

	case EFCCardAddDestination::Hand:
		HandContainer.AddCard(CardId);
		OnCardHandUpdated.Broadcast();
		break;
	}

	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);
	return true;
}

FFCCardDeckSaveData UFCCardDeckComponent::ExportDeckSaveData() const
{
	FFCCardDeckSaveData SaveData;
	SaveData.HandCards = HandContainer.Items;
	SaveData.DrawPile = ServerDrawPile;
	SaveData.DiscardPile = ServerDiscardPile;
	SaveData.ExhaustPile = ServerExhaustPile;
	return SaveData;
}

void UFCCardDeckComponent::RestoreFromDeckSaveData(const FFCCardDeckSaveData& SaveData)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ServerDrawPile = SaveData.DrawPile;
	ServerDiscardPile = SaveData.DiscardPile;
	ServerExhaustPile = SaveData.ExhaustPile;

	HandContainer.Items = SaveData.HandCards;
	HandContainer.MarkArrayDirty();

	DrawPileCount = ServerDrawPile.Num();
	DiscardPileCount = ServerDiscardPile.Num();
	ExhaustPileCount = ServerExhaustPile.Num();

	OnCardHandUpdated.Broadcast();
	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);
}

bool UFCCardDeckComponent::Server_PlayCard_Validate(const FGuid& CardGuid, const FFCCardTargetInfo& TargetInfo)
{
	if (!CardGuid.IsValid())
	{
		return false;
	}

	const FFCCardItem* Item = HandContainer.FindCard(CardGuid);
	if (!Item)
	{
		return false;
	}

	if (Item->bIsLocked)
	{
		return false;
	}

	return true;
}

void UFCCardDeckComponent::Server_PlayCard_Implementation(const FGuid& CardGuid, const FFCCardTargetInfo& TargetInfo)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	const FFCCardItem* Item = HandContainer.FindCard(CardGuid);
	if (!Item || Item->bIsLocked)
	{
		return;
	}

	FName CardId = Item->CardId;
	UFCCardSubsystem* Subsystem = UFCCardSubsystem::GetCardSubsystem(this);
	UFCCardDataAsset* DataAsset = Subsystem ? Subsystem->GetCardDataAsset(CardId) : nullptr;

	int32 ManaCost = DataAsset ? DataAsset->GameplayData.BaseManaCost : 1;

	// Check & deduct mana from ASC AttributeSet if available
	UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();
	if (ASC)
	{
		const UFCAttributeSet* AttributeSet = ASC->GetSet<UFCAttributeSet>();
		if (AttributeSet && AttributeSet->GetMana() < ManaCost)
		{
			return; // Insufficient mana
		}

		// Deduct mana
		if (AttributeSet)
		{
			const_cast<UFCAttributeSet*>(AttributeSet)->SetMana(AttributeSet->GetMana() - ManaCost);
		}

		// Trigger GameplayAbility if assigned
		if (DataAsset && DataAsset->GameplayData.CardAbilityClass)
		{
			FGameplayAbilitySpec Spec(DataAsset->GameplayData.CardAbilityClass, Item->UpgradeLevel + 1);
			FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
			ASC->TryActivateAbility(Handle);
		}

		// Apply direct Gameplay Effects if assigned
		if (DataAsset)
		{
			for (const TSubclassOf<UGameplayEffect>& EffectClass : DataAsset->GameplayData.CardEffectClasses)
			{
				if (EffectClass)
				{
					FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
					Context.AddInstigator(GetOwner(), GetOwner());
					FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, Item->UpgradeLevel + 1, Context);
					if (SpecHandle.IsValid())
					{
						if (DataAsset->GameplayData.TargetType == EFCCardTargetType::Self || !TargetInfo.TargetActor.IsValid())
						{
							ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
						}
						else if (const IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(TargetInfo.TargetActor.Get()))
						{
							if (UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent())
							{
								TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
							}
						}
					}
				}
			}
		}
	}

	// Move played card to discard pile and remove from hand
	ServerDiscardPile.Add(CardId);
	HandContainer.RemoveCard(CardGuid);

	DiscardPileCount = ServerDiscardPile.Num();

	OnCardPlayed.Broadcast(CardGuid, CardId);
	OnCardHandUpdated.Broadcast();
	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);
}

bool UFCCardDeckComponent::Server_DrawCards_Validate(int32 Count)
{
	return Count > 0 && Count <= 10;
}

void UFCCardDeckComponent::Server_DrawCards_Implementation(int32 Count)
{
	DrawCards(Count);
}

bool UFCCardDeckComponent::Server_EndTurn_Validate()
{
	return true;
}

void UFCCardDeckComponent::Server_EndTurn_Implementation()
{
	DiscardEntireHand();
}

void UFCCardDeckComponent::NotifyHandChanged()
{
	OnCardHandUpdated.Broadcast();
}

void UFCCardDeckComponent::NotifyItemChanged(const FFCCardItem& Item)
{
	OnCardItemChanged.Broadcast(Item);
}

void UFCCardDeckComponent::OnRep_HandContainer()
{
	OnCardHandUpdated.Broadcast();
}

void UFCCardDeckComponent::OnRep_PileCounts()
{
	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);
}

UAbilitySystemComponent* UFCCardDeckComponent::GetOwnerAbilitySystemComponent() const
{
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return ASI->GetAbilitySystemComponent();
	}

	if (const APawn* PawnOwner = Cast<APawn>(GetOwner()))
	{
		if (const IAbilitySystemInterface* PawnASI = Cast<IAbilitySystemInterface>(PawnOwner->GetPlayerState()))
		{
			return PawnASI->GetAbilitySystemComponent();
		}
	}

	if (const APlayerState* PSOwner = Cast<APlayerState>(GetOwner()))
	{
		if (const APawn* Pawn = PSOwner->GetPawn())
		{
			if (const IAbilitySystemInterface* PawnASI = Cast<IAbilitySystemInterface>(Pawn))
			{
				return PawnASI->GetAbilitySystemComponent();
			}
		}
	}

	return nullptr;
}
