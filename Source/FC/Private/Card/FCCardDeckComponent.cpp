#include "Card/FCCardDeckComponent.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Class/FCClassSubsystem.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "AbilitySystem/Abilities/FCGA_SpawnProjectile.h"
#include "Combat/FCCombatUtils.h"
#include "Combat/Element/FCElementComponent.h"
#include "Character/FCCharacterBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UFCCardDeckComponent::UFCCardDeckComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	HandContainer.OwnerComponent = this;

	CycleInterval = 30.0f;
	CycleDrawCount = 5;
	bAutoCycleEnabled = true;
	CycleEndTime = 0.0f;
}

void UFCCardDeckComponent::BeginPlay()
{
	Super::BeginPlay();
	HandContainer.OwnerComponent = this;

	if (GetOwner() && GetOwner()->HasAuthority() && bAutoCycleEnabled && ServerDrawPile.Num() > 0)
	{
		StartCycleTimer();
	}
}

void UFCCardDeckComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopCycleTimer();
	Super::EndPlay(EndPlayReason);
}

void UFCCardDeckComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, HandContainer, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, DrawPileCount, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, DiscardPileCount, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UFCCardDeckComponent, ExhaustPileCount, COND_OwnerOnly);

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
		int32 SwapIdx = FMath::RandRange(0, NumCards - 1);
		ServerDrawPile.Swap(i, SwapIdx);
	}

	DrawPileCount = ServerDrawPile.Num();
	DiscardPileCount = 0;
	ExhaustPileCount = 0;

	if (bAutoCycleEnabled)
	{
		StartCycleTimer();
	}
}

void UFCCardDeckComponent::InitializeDeckForClass(EFCCharacterClass InClass)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	TArray<FName> ClassStarterDeck;
	if (UFCClassSubsystem* ClassSubsystem = UFCClassSubsystem::GetClassSubsystem(this))
	{
		ClassStarterDeck = ClassSubsystem->GetStartingDeckForClass(InClass);
	}
	else
	{
		ClassStarterDeck = {
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_AttackBuff"),
			FName("Card_AttackBuff"),
			FName("Card_AttackBuff"),
			FName("Card_AttackBuff"),
			FName("Card_AttackBuff")
		};
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

bool UFCCardDeckComponent::RetrieveCardFromExhaust(FName CardId, EFCCardAddDestination Destination)
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
		if (HandContainer.Num() >= MaxHandSize)
		{
			return false; // Hand is full
		}
		HandContainer.AddCard(CardId);
		OnCardHandUpdated.Broadcast();
		break;

	case EFCCardAddDestination::ExhaustPile:
		ServerExhaustPile.Add(CardId);
		ExhaustPileCount = ServerExhaustPile.Num();
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

		// Deduct mana (unconditionally, even if element condition fails)
		if (AttributeSet)
		{
			const_cast<UFCAttributeSet*>(AttributeSet)->SetMana(AttributeSet->GetMana() - ManaCost);
		}

		// Handle target element consumption for conditional cards:
		// If the card requires consuming elements, check & consume from TargetActor.
		// If condition is not met, card is still played and mana deducted, but effect is skipped.
		bool bCanApplyConditionalEffects = true;
		if (DataAsset && DataAsset->GameplayData.RequiresElementConsumption())
		{
			if (TargetInfo.TargetActor.IsValid())
			{
				bCanApplyConditionalEffects = UFCCombatUtils::TryConsumeTargetElements(TargetInfo.TargetActor.Get(), DataAsset->GameplayData.ConsumedElements);
			}
			else
			{
				bCanApplyConditionalEffects = false;
			}
		}

		// If this is a projectile card or targeted card, set target and rotate character towards target on server
		if (DataAsset && DataAsset->GameplayData.SpawnsProjectile())
		{
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

			if (Char)
			{
				AActor* ValidTarget = TargetInfo.TargetActor.Get();
				if (ValidTarget && !UFCCombatUtils::IsAttackableTarget(Char, ValidTarget))
				{
					ValidTarget = nullptr;
				}

				Char->SetCombatTarget(ValidTarget);
				Char->SetTargetAimLocation(TargetInfo.TargetLocation);

				// Rotate character towards target actor or target location
				const FVector AimPos = ValidTarget ? ValidTarget->GetActorLocation() : (FVector)TargetInfo.TargetLocation;
				if (!AimPos.IsZero())
				{
					Char->RotateTowardsTarget(AimPos);
				}
			}
		}

		// Trigger GameplayAbility if assigned or if projectile data asset is specified
		TSubclassOf<UGameplayAbility> AbilityToActivate = DataAsset ? DataAsset->GameplayData.CardAbilityClass : nullptr;
		if (!AbilityToActivate && DataAsset && DataAsset->GameplayData.SpawnsProjectile() && DataAsset->GameplayData.ProjectileDataAsset)
		{
			AbilityToActivate = UFCGA_SpawnProjectile::StaticClass();
		}

		if (AbilityToActivate && bCanApplyConditionalEffects)
		{
			FGameplayAbilitySpec Spec(AbilityToActivate, Item->UpgradeLevel + 1);
			Spec.SourceObject = DataAsset;
			FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
			ASC->TryActivateAbility(Handle);
		}

		// Apply direct Gameplay Effects if assigned and condition met
		if (DataAsset && bCanApplyConditionalEffects)
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

		// If this is a direct single-target attack card with elements, apply element stacks to target
		if (DataAsset && DataAsset->GameplayData.CardType == EFCCardType::Attack && DataAsset->GameplayData.Elements.Num() > 0 && TargetInfo.TargetActor.IsValid())
		{
			UFCCombatUtils::ApplyAttackCardHitTraits(GetOwner(), TargetInfo.TargetActor.Get(), DataAsset->GameplayData.RequiredClass, DataAsset->GameplayData.CardType, DataAsset->GameplayData.Elements);
		}
	}

	// Move played card to Exhaust Pile or Discard Pile based on card exhaust keyword/property
	const bool bShouldExhaust = DataAsset && DataAsset->GameplayData.DoesExhaustOnPlay();
	if (bShouldExhaust)
	{
		ServerExhaustPile.Add(CardId);
		ExhaustPileCount = ServerExhaustPile.Num();
	}
	else
	{
		ServerDiscardPile.Add(CardId);
		DiscardPileCount = ServerDiscardPile.Num();
	}

	HandContainer.RemoveCard(CardGuid);

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
		CycleEndTime = World->GetTimeSeconds() + CycleInterval;
		World->GetTimerManager().SetTimer(CycleTimerHandle, this, &UFCCardDeckComponent::ExecuteHandCycle, CycleInterval, true);
	}
}

void UFCCardDeckComponent::StopCycleTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CycleTimerHandle);
	}
	CycleEndTime = 0.0f;
}

void UFCCardDeckComponent::ResetCycleTimer()
{
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

	// 1. Process hand cards: Retain stays in hand, Ethereal exhausts, others discard
	TArray<FGuid> CardsToExhaust;
	TArray<FGuid> CardsToDiscard;

	for (const FFCCardItem& Item : HandContainer.Items)
	{
		const UFCCardDataAsset* DataAsset = CardSubsystem ? CardSubsystem->GetCardDataAsset(Item.CardId) : nullptr;
		if (DataAsset && DataAsset->GameplayData.DoesRetain())
		{
			// Preserve in hand
			continue;
		}
		else if (DataAsset && DataAsset->GameplayData.IsEthereal())
		{
			CardsToExhaust.Add(Item.CardGuid);
		}
		else
		{
			CardsToDiscard.Add(Item.CardGuid);
		}
	}

	for (const FGuid& Guid : CardsToExhaust)
	{
		if (const FFCCardItem* Item = HandContainer.FindCard(Guid))
		{
			ServerExhaustPile.Add(Item->CardId);
			HandContainer.RemoveCard(Guid);
		}
	}

	for (const FGuid& Guid : CardsToDiscard)
	{
		if (const FFCCardItem* Item = HandContainer.FindCard(Guid))
		{
			ServerDiscardPile.Add(Item->CardId);
			HandContainer.RemoveCard(Guid);
		}
	}

	DiscardPileCount = ServerDiscardPile.Num();
	ExhaustPileCount = ServerExhaustPile.Num();

	// 2. Refresh owner's mana
	RefreshOwnerMana();

	// 3. Draw configured cards from deck
	DrawCards(CycleDrawCount);

	// 4. Update cycle timestamp for next interval
	if (UWorld* World = GetWorld())
	{
		CycleEndTime = World->GetTimeSeconds() + CycleInterval;
	}

	OnCardHandUpdated.Broadcast();
	OnPileCountsChanged.Broadcast(DrawPileCount, DiscardPileCount, ExhaustPileCount);
	OnCardCycleTriggered.Broadcast();
	Client_OnCycleTriggered();
}

void UFCCardDeckComponent::Client_OnCycleTriggered_Implementation()
{
	OnCardCycleTriggered.Broadcast();
}

float UFCCardDeckComponent::GetCycleRemainingTime() const
{
	if (!bAutoCycleEnabled)
	{
		return 0.0f;
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (const UWorld* World = GetWorld())
		{
			if (World->GetTimerManager().IsTimerActive(CycleTimerHandle))
			{
				return World->GetTimerManager().GetTimerRemaining(CycleTimerHandle);
			}
		}
	}

	if (const UWorld* World = GetWorld())
	{
		return FMath::Max(0.0f, CycleEndTime - World->GetTimeSeconds());
	}
	return 0.0f;
}

float UFCCardDeckComponent::GetCycleProgress() const
{
	if (CycleInterval <= 0.0f || !bAutoCycleEnabled)
	{
		return 0.0f;
	}
	const float Remaining = GetCycleRemainingTime();
	return FMath::Clamp(1.0f - (Remaining / CycleInterval), 0.0f, 1.0f);
}

void UFCCardDeckComponent::OnRep_CycleSettings()
{
	OnCycleSettingsChanged.Broadcast(CycleInterval, CycleDrawCount);
}

void UFCCardDeckComponent::OnRep_CycleEndTime()
{
}

