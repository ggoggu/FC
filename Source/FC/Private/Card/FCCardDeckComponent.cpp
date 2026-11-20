#include "Card/FCCardDeckComponent.h"
#include "Card/FCCardExecutionContext.h"
#include "Card/Policies/FCCardTargetPolicyBase.h"
#include "Card/Conditions/FCCardConditionBase.h"
#include "Card/Effects/FCCardEffectBase.h"
#include "Card/Keywords/FCCardKeywordRuleBase.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Class/FCClassSubsystem.h"
#include "Data/FCGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Character/FCCharacterBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/StreamableManager.h"

UFCCardDeckComponent::UFCCardDeckComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	HandContainer.OwnerComponent = this;
	DrawPileSummary.OwnerComponent = this;
	DiscardPileSummary.OwnerComponent = this;

	CycleInterval = 30.0f;
	CycleDrawCount = 5;
	bAutoCycleEnabled = true;
	CycleEndTime = 0.0;
}

void UFCCardDeckComponent::BeginPlay()
{
	Super::BeginPlay();
	HandContainer.OwnerComponent = this;
	DrawPileSummary.OwnerComponent = this;
	DiscardPileSummary.OwnerComponent = this;

	if (GetOwner() && GetOwner()->HasAuthority() && bAutoCycleEnabled && ServerDrawPile.Num() > 0)
	{
		StartCycleTimer();
	}
}

void UFCCardDeckComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopCycleTimer();
	ActiveDeckPreloadHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void UFCCardDeckComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, HandContainer, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, DrawPileCount, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, DiscardPileCount, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, ExhaustPileCount, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, DrawPileSummary, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, DiscardPileSummary, COND_OwnerOnly);

	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, CycleInterval, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, CycleDrawCount, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, bAutoCycleEnabled, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, CycleEndTime, COND_OwnerOnly);
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
		const int32 SwapIdx = FMath::RandRange(0, NumCards - 1);
		ServerDrawPile.Swap(i, SwapIdx);
	}

	// Prioritize cards with Innate-like keyword rules at the top (end) of ServerDrawPile for opening draw
	if (UFCCardSubsystem* Subsystem = UFCCardSubsystem::GetCardSubsystem(this))
	{
		TArray<FName> NormalCards;
		TArray<FName> PrioritizedCards;
		NormalCards.Reserve(ServerDrawPile.Num());

		for (const FName& CardId : ServerDrawPile)
		{
			bool bPrioritize = false;
			if (const FFCCardTableRow* Row = Subsystem->FindCardRowPtr(CardId))
			{
				for (const FGameplayTag& Tag : Row->GameplayData.CardTags)
				{
					if (const FFCCardKeywordRuleBase* Rule = Subsystem->FindKeywordRule(Tag))
					{
						if (Rule->ShouldPrioritizeInStartingHand())
						{
							bPrioritize = true;
							break;
						}
					}
				}
			}

			if (bPrioritize)
			{
				PrioritizedCards.Add(CardId);
			}
			else
			{
				NormalCards.Add(CardId);
			}
		}

		if (PrioritizedCards.Num() > 0)
		{
			ServerDrawPile = MoveTemp(NormalCards);
			ServerDrawPile.Append(PrioritizedCards);
		}
	}

	DrawPileCount = ServerDrawPile.Num();
	DiscardPileCount = 0;
	ExhaustPileCount = 0;
	SyncReplicatedPileSummaries();

	if (bAutoCycleEnabled)
	{
		StartCycleTimer();
	}

	// Trigger on-demand async preloading for active combat deck
	if (UFCCardSubsystem* Subsystem = UFCCardSubsystem::GetCardSubsystem(this))
	{
		TSet<FName> UniqueDeckCards(ServerDrawPile);
		ActiveDeckPreloadHandle = Subsystem->PreloadCardAssetsAsync(UniqueDeckCards.Array());
	}
}

void UFCCardDeckComponent::InitializeDeckForClass(FGameplayTag InClassTag)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	TArray<FName> ClassStarterDeck;
	if (UFCClassSubsystem* ClassSubsystem = UFCClassSubsystem::GetClassSubsystem(this))
	{
		ClassStarterDeck = ClassSubsystem->GetStartingDeckForClass(InClassTag);
	}
	else if (const FFCCharacterClassData* BuiltinRow = UFCClassSubsystem::FindBuiltinClassDefault(InClassTag))
	{
		ClassStarterDeck = BuiltinRow->StartingDeck;
	}

	InitializeDeck(ClassStarterDeck);
}

void UFCCardDeckComponent::DrawCards(int32 Count)
{
	if (!GetOwner()->HasAuthority() || Count <= 0)
	{
		return;
	}

	for (int32 i = 0; i < Count; ++i)
	{
		if (HandContainer.Num() >= MaxHandSize)
		{
			break; // Hand reached maximum capacity
		}

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
	SyncReplicatedPileSummaries();

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
	SyncReplicatedPileSummaries();
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

bool UFCCardDeckComponent::RetrieveCardFromExhaust(FName CardId, EFCCardZone Destination)
{
	if (!GetOwner()->HasAuthority() || CardId.IsNone())
	{
		return false;
	}

	int32 FoundIdx = ServerExhaustPile.IndexOfByKey(CardId);
	if (FoundIdx == INDEX_NONE)
	{
		return false;
	}

	ServerExhaustPile.RemoveAt(FoundIdx);
	ExhaustPileCount = ServerExhaustPile.Num();

	AddCardToDeck(CardId, Destination, true);
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
	SyncReplicatedPileSummaries();
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
	SyncReplicatedPileSummaries();

	OnCardHandUpdated.Broadcast();
	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);
}

bool UFCCardDeckComponent::AddCardToDeck(FName CardId, EFCCardZone Destination, bool bShuffleIfDrawPile)
{
	if (!GetOwner()->HasAuthority() || CardId.IsNone())
	{
		return false;
	}

	UFCCardSubsystem* Subsystem = UFCCardSubsystem::GetCardSubsystem(this);
	if (!Subsystem || !Subsystem->HasCard(CardId))
	{
		return false;
	}

	switch (Destination)
	{
	case EFCCardZone::DrawPile:
		ServerDrawPile.Add(CardId);
		if (bShuffleIfDrawPile)
		{
			const int32 NumCards = ServerDrawPile.Num();
			for (int32 i = 0; i < NumCards; ++i)
			{
				const int32 SwapIdx = FMath::RandRange(0, NumCards - 1);
				ServerDrawPile.Swap(i, SwapIdx);
			}
		}
		DrawPileCount = ServerDrawPile.Num();
		break;

	case EFCCardZone::DiscardPile:
		ServerDiscardPile.Add(CardId);
		DiscardPileCount = ServerDiscardPile.Num();
		break;

	case EFCCardZone::Hand:
		if (HandContainer.Num() >= MaxHandSize)
		{
			return false;
		}
		HandContainer.AddCard(CardId);
		OnCardHandUpdated.Broadcast();
		break;

	case EFCCardZone::ExhaustPile:
		ServerExhaustPile.Add(CardId);
		ExhaustPileCount = ServerExhaustPile.Num();
		break;
	}

	SyncReplicatedPileSummaries();
	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);

	if (Subsystem)
	{
		Subsystem->PreloadCardAssetsAsync({ CardId });
	}

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
	SyncReplicatedPileSummaries();

	OnCardHandUpdated.Broadcast();
	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);
}

bool UFCCardDeckComponent::Server_PlayCard_Validate(const FGuid& CardGuid, const FFCCardTargetInfo& TargetInfo)
{
	return CardGuid.IsValid() && !TargetInfo.TargetLocation.ContainsNaN();
}

void UFCCardDeckComponent::Server_PlayCard_Implementation(const FGuid& CardGuid, const FFCCardTargetInfo& TargetInfo)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (TargetInfo.TargetLocation.ContainsNaN())
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		const double Now = World->GetTimeSeconds();
		if (Now - LastPlayCardServerTime < 0.05)
		{
			return;
		}
		LastPlayCardServerTime = Now;
	}

	// 1. Resolve and validate caster Pawn before mutating mana/hand state
	AFCCharacterBase* Char = Cast<AFCCharacterBase>(GetOwner());
	if (!Char)
	{
		if (const APlayerState* PS = Cast<APlayerState>(GetOwner()))
		{
			Char = Cast<AFCCharacterBase>(PS->GetPawn());
		}
		else if (const AController* Ctrl = Cast<AController>(GetOwner()))
		{
			Char = Cast<AFCCharacterBase>(Ctrl->GetPawn());
		}
	}

	if (!Char || Char->IsDead())
	{
		return;
	}

	const FFCCardItem* Item = HandContainer.FindCard(CardGuid);
	if (!Item || Item->bIsLocked)
	{
		return;
	}

	const FName CardId = Item->CardId;
	const int32 UpgradeLevel = Item->UpgradeLevel;
	UFCCardSubsystem* Subsystem = UFCCardSubsystem::GetCardSubsystem(this);
	const FFCCardTableRow* CardRow = Subsystem ? Subsystem->FindCardRowPtr(CardId) : nullptr;
	UFCCardDataAsset* DataAsset = Subsystem ? Subsystem->GetCardDataAsset(CardId) : nullptr;
	const FFCCardGameplayData* CardData = CardRow ? &CardRow->GameplayData : (DataAsset ? &DataAsset->GameplayData : nullptr);

	// 2. Build immutable per-invocation execution context
	UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();
	FFCCardExecutionContext Context;
	Context.CasterActor = Char;
	Context.CasterASC = ASC;
	Context.DeckComponent = this;
	Context.CardDataAsset = DataAsset;
	Context.CardData = CardData;
	Context.CardGuid = CardGuid;
	Context.CardId = CardId;
	Context.UpgradeLevel = UpgradeLevel;
	Context.RawTargetInfo = TargetInfo;

	// 3. Keyword playability gate (e.g. Card.Keyword.Unplayable)
	if (CardData && Subsystem)
	{
		for (const FGameplayTag& Tag : CardData->CardTags)
		{
			if (const FFCCardKeywordRuleBase* Rule = Subsystem->FindKeywordRule(Tag))
			{
				if (!Rule->CanPlayCard(Context))
				{
					return;
				}
			}
		}
	}

	// 4. Authoritative target resolution via modular TargetPolicy (TInstancedStruct)
	const FFCCardTargetPolicyBase* TargetPolicy = CardData ? CardData->TargetPolicy.GetPtr() : nullptr;
	if (TargetPolicy)
	{
		if (!TargetPolicy->VerifyServerTargets(Context, Context.VerifiedTargets, Context.VerifiedAimLocation))
		{
			return;
		}
	}

	// 5. Validate mana & hard preconditions before mutating state
	const int32 ManaCost = CardData ? CardData->BaseManaCost : 1;
	const UFCAttributeSet* AttributeSet = ASC ? ASC->GetSet<UFCAttributeSet>() : nullptr;
	if (AttributeSet && AttributeSet->GetMana() < ManaCost)
	{
		return;
	}

	if (CardData)
	{
		for (const TInstancedStruct<FFCCardConditionBase>& CondStruct : CardData->Conditions)
		{
			if (const FFCCardConditionBase* Cond = CondStruct.GetPtr())
			{
				if (Cond->BlocksCardPlayWhenUnmet() && !Cond->CanExecute(Context))
				{
					return;
				}
			}
		}
	}

	// 6. Commit costs (mana + hard/soft conditions) and remove card from hand
	if (AttributeSet && ManaCost > 0)
	{
		const float NewMana = FMath::Max(0.0f, AttributeSet->GetMana() - static_cast<float>(ManaCost));
		if (ASC && ASC->GetOwnerActor())
		{
			const_cast<UFCAttributeSet*>(AttributeSet)->SetMana(NewMana);
		}
		else
		{
			const_cast<UFCAttributeSet*>(AttributeSet)->InitMana(NewMana);
		}
	}

	HandContainer.RemoveCard(CardGuid);

	if (CardData)
	{
		for (const TInstancedStruct<FFCCardConditionBase>& CondStruct : CardData->Conditions)
		{
			if (const FFCCardConditionBase* Cond = CondStruct.GetPtr())
			{
				if (!Cond->ConsumeCost(Context))
				{
					Context.bConditionalEffectsAllowed = false;
				}
			}
		}
	}

	// 7. Orient caster if requested by TargetPolicy
	if (TargetPolicy && TargetPolicy->ShouldRotateCasterOnPlay())
	{
		const AActor* PrimaryTarget = Context.GetPrimaryTarget();
		const FVector AimPos = PrimaryTarget ? PrimaryTarget->GetActorLocation() : Context.VerifiedAimLocation;
		if (!AimPos.IsZero())
		{
			Char->RotateTowardsTarget(AimPos);
		}
	}

	// 8. Execute composite effect fragments sequentially
	if (CardData && Context.bConditionalEffectsAllowed)
	{
		for (const TInstancedStruct<FFCCardEffectBase>& EffectStruct : CardData->PlayEffects)
		{
			if (const FFCCardEffectBase* Effect = EffectStruct.GetPtr())
			{
				if (Effect->Execute(Context))
				{
					Context.bExecutedAnyEffect = true;
				}
			}
		}
	}

	// 9. Route played card to destination zone determined by KeywordRules (default: DiscardPile)
	EFCCardZone PostPlayZone = EFCCardZone::DiscardPile;
	if (CardData && Subsystem)
	{
		for (const FGameplayTag& Tag : CardData->CardTags)
		{
			if (const FFCCardKeywordRuleBase* Rule = Subsystem->FindKeywordRule(Tag))
			{
				if (Rule->OverridePostPlayZone(Context, PostPlayZone))
				{
					break;
				}
			}
		}
	}

	switch (PostPlayZone)
	{
	case EFCCardZone::ExhaustPile:
		ServerExhaustPile.Add(CardId);
		break;
	case EFCCardZone::DrawPile:
		ServerDrawPile.Add(CardId);
		break;
	case EFCCardZone::Hand:
		if (HandContainer.Num() < MaxHandSize)
		{
			HandContainer.AddCard(CardId, UpgradeLevel);
		}
		else
		{
			ServerDiscardPile.Add(CardId);
		}
		break;
	case EFCCardZone::DiscardPile:
	default:
		ServerDiscardPile.Add(CardId);
		break;
	}

	DrawPileCount = ServerDrawPile.Num();
	DiscardPileCount = ServerDiscardPile.Num();
	ExhaustPileCount = ServerExhaustPile.Num();
	SyncReplicatedPileSummaries();

	OnCardPlayed.Broadcast(CardGuid, CardId);
	OnCardHandUpdated.Broadcast();
	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);
}

void UFCCardDeckComponent::NotifyHandChanged()
{
	OnCardHandUpdated.Broadcast();
}

void UFCCardDeckComponent::NotifyItemChanged(const FFCCardItem& Item)
{
	OnCardItemChanged.Broadcast(Item);
}

void UFCCardDeckComponent::NotifyPileSummariesChanged()
{
	OnPileSummariesUpdated.Broadcast();
}

void UFCCardDeckComponent::OnRep_HandContainer()
{
	OnCardHandUpdated.Broadcast();
}

void UFCCardDeckComponent::OnRep_PileCounts()
{
	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);
}

void UFCCardDeckComponent::OnRep_PileSummaries()
{
	OnPileSummariesUpdated.Broadcast();
}

void UFCCardDeckComponent::SyncReplicatedPileSummaries()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const bool bDrawChanged = DrawPileSummary.SyncFromCardIds(ServerDrawPile);
	const bool bDiscardChanged = DiscardPileSummary.SyncFromCardIds(ServerDiscardPile);

	if (bDrawChanged || bDiscardChanged)
	{
		OnPileSummariesUpdated.Broadcast();
	}
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

void UFCCardDeckComponent::SetCycleInterval(float InInterval)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	CycleInterval = FMath::Max(1.0f, InInterval);
	OnCycleSettingsChanged.Broadcast(CycleInterval, CycleDrawCount);

	if (bAutoCycleEnabled)
	{
		ResetCycleTimer();
	}
}

void UFCCardDeckComponent::SetCycleDrawCount(int32 InDrawCount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	CycleDrawCount = FMath::Max(0, InDrawCount);
	OnCycleSettingsChanged.Broadcast(CycleInterval, CycleDrawCount);
}

void UFCCardDeckComponent::SetAutoCycleEnabled(bool bEnabled)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (bAutoCycleEnabled != bEnabled)
	{
		bAutoCycleEnabled = bEnabled;
		OnCycleSettingsChanged.Broadcast(CycleInterval, CycleDrawCount);

		if (bAutoCycleEnabled)
		{
			StartCycleTimer();
		}
		else
		{
			StopCycleTimer();
		}
	}
}

void UFCCardDeckComponent::StartCycleTimer()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (!bAutoCycleEnabled || CycleInterval <= 0.0f)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		const double ServerTime = World->GetGameState() ? World->GetGameState()->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
		CycleEndTime = ServerTime + static_cast<double>(CycleInterval);
		World->GetTimerManager().SetTimer(CycleTimerHandle, this, &UFCCardDeckComponent::ExecuteHandCycle, CycleInterval, true);
		OnCycleTimerUpdated.Broadcast();
	}
}

void UFCCardDeckComponent::StopCycleTimer()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CycleTimerHandle);
	}
	CycleEndTime = 0.0;
	OnCycleTimerUpdated.Broadcast();
}

void UFCCardDeckComponent::ResetCycleTimer()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	StopCycleTimer();
	StartCycleTimer();
}

void UFCCardDeckComponent::RefreshOwnerMana()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent())
	{
		if (const UFCAttributeSet* AttributeSet = ASC->GetSet<UFCAttributeSet>())
		{
			const_cast<UFCAttributeSet*>(AttributeSet)->RefreshMana();
		}
	}
}

void UFCCardDeckComponent::ExecuteHandCycle()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	UFCCardSubsystem* CardSubsystem = UFCCardSubsystem::GetCardSubsystem(this);

	// 1. Process hand cards through DataTable-driven KeywordRules (default: DiscardPile)
	TArray<TPair<FGuid, EFCCardZone>> Transitions;
	Transitions.Reserve(HandContainer.Items.Num());

	for (const FFCCardItem& Item : HandContainer.Items)
	{
		EFCCardZone DestZone = EFCCardZone::DiscardPile;
		if (CardSubsystem)
		{
			if (const FFCCardTableRow* CardRow = CardSubsystem->FindCardRowPtr(Item.CardId))
			{
				for (const FGameplayTag& Tag : CardRow->GameplayData.CardTags)
				{
					if (const FFCCardKeywordRuleBase* Rule = CardSubsystem->FindKeywordRule(Tag))
					{
						if (Rule->OverrideCycleEndZone(this, Item, DestZone))
						{
							break;
						}
					}
				}
			}
		}

		if (DestZone != EFCCardZone::Hand)
		{
			Transitions.Emplace(Item.CardGuid, DestZone);
		}
	}

	for (const TPair<FGuid, EFCCardZone>& Transition : Transitions)
	{
		if (const FFCCardItem* Item = HandContainer.FindCard(Transition.Key))
		{
			const FName MovingCardId = Item->CardId;
			switch (Transition.Value)
			{
			case EFCCardZone::ExhaustPile:
				ServerExhaustPile.Add(MovingCardId);
				break;
			case EFCCardZone::DrawPile:
				ServerDrawPile.Add(MovingCardId);
				break;
			case EFCCardZone::DiscardPile:
			default:
				ServerDiscardPile.Add(MovingCardId);
				break;
			}
			HandContainer.RemoveCard(Transition.Key);
		}
	}

	DrawPileCount = ServerDrawPile.Num();
	DiscardPileCount = ServerDiscardPile.Num();
	ExhaustPileCount = ServerExhaustPile.Num();

	// 2. Refresh owner's mana
	RefreshOwnerMana();

	// 3. Draw configured cards from deck
	DrawCards(CycleDrawCount);
	SyncReplicatedPileSummaries();

	// 4. Update cycle timestamp for next interval using synchronized server time
	if (bAutoCycleEnabled && CycleInterval > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			const double ServerTime = World->GetGameState() ? World->GetGameState()->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
			CycleEndTime = ServerTime + static_cast<double>(CycleInterval);
		}
	}
	else
	{
		CycleEndTime = 0.0;
	}

	OnCardHandUpdated.Broadcast();
	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);
	OnCardCycleTriggered.Broadcast();
	OnCycleTimerUpdated.Broadcast();
	Client_OnCycleTriggered(CycleEndTime);
}

void UFCCardDeckComponent::Client_OnCycleTriggered_Implementation(double InNextCycleEndTime)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		CycleEndTime = InNextCycleEndTime;
		OnCardCycleTriggered.Broadcast();
		OnCycleTimerUpdated.Broadcast();
	}
}

float UFCCardDeckComponent::GetCycleRemainingTime() const
{
	if (!bAutoCycleEnabled || CycleInterval <= 0.0f)
	{
		return 0.0f;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (World->GetTimerManager().IsTimerActive(CycleTimerHandle))
		{
			const float TimerRemaining = World->GetTimerManager().GetTimerRemaining(CycleTimerHandle);
			if (TimerRemaining > KINDA_SMALL_NUMBER)
			{
				return FMath::Clamp(TimerRemaining, 0.0f, CycleInterval);
			}
		}
	}
	else if (!World->GetGameState())
	{
		return 0.0f;
	}

	if (CycleEndTime <= 0.0)
	{
		return 0.0f;
	}

	const double ServerTime = World->GetGameState() ? World->GetGameState()->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
	return FMath::Clamp(static_cast<float>(CycleEndTime - ServerTime), 0.0f, CycleInterval);
}

float UFCCardDeckComponent::GetCycleProgress() const
{
	if (CycleInterval <= 0.0f || !bAutoCycleEnabled || CycleEndTime <= 0.0)
	{
		return 0.0f;
	}

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		const UWorld* World = GetWorld();
		if (!World || !World->GetGameState())
		{
			return 0.0f;
		}
	}

	const float Remaining = GetCycleRemainingTime();
	return FMath::Clamp(1.0f - (Remaining / CycleInterval), 0.0f, 1.0f);
}

void UFCCardDeckComponent::OnRep_CycleSettings()
{
	OnCycleSettingsChanged.Broadcast(CycleInterval, CycleDrawCount);
	OnCycleTimerUpdated.Broadcast();
}

void UFCCardDeckComponent::OnRep_CycleEndTime()
{
	OnCycleTimerUpdated.Broadcast();
}

