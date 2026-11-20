#include "UI/Presenter/FCHUDPresenterComponent.h"
#include "UI/Presenter/FCStatusPresenterComponent.h"
#include "UI/View/FCHUDWidget.h"
#include "UI/ViewModel/FCHUDViewModel.h"
#include "UI/ViewModel/FCHandViewModel.h"
#include "UI/ViewModel/FCCardPileViewModel.h"
#include "Card/FCCardDeckComponent.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Game/FCPlayerState.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

UFCHUDPresenterComponent::UFCHUDPresenterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFCHUDPresenterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CycleHUDUpdateTimerHandle);
	}

	UnbindDeckComponentEvents();
	UnbindAttributeListeners(CachedStatusPresenter.Get());
	Super::EndPlay(EndPlayReason);
}

void UFCHUDPresenterComponent::SetupHUD(TSubclassOf<UFCHUDWidget> InHUDWidgetClass, UFCStatusPresenterComponent* StatusPresenter)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalPlayerController() || !InHUDWidgetClass)
	{
		return;
	}

	if (StatusPresenter)
	{
		CachedStatusPresenter = StatusPresenter;
	}

	if (!HUDViewModel)
	{
		HUDViewModel = NewObject<UFCHUDViewModel>(this);
	}

	if (!HUDWidget)
	{
		HUDWidget = CreateWidget<UFCHUDWidget>(PC, InHUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
			HUDWidget->SetHUDViewModel(HUDViewModel);
		}
	}
	else
	{
		HUDWidget->SetHUDViewModel(HUDViewModel);
	}

	RefreshBindings(StatusPresenter);
}

void UFCHUDPresenterComponent::RefreshBindings(UFCStatusPresenterComponent* StatusPresenter)
{
	if (StatusPresenter)
	{
		CachedStatusPresenter = StatusPresenter;
	}

	BindDeckComponentEvents();
	BindAttributeListeners(CachedStatusPresenter.Get());
}

void UFCHUDPresenterComponent::UnbindDeckComponentEvents()
{
	if (UFCCardDeckComponent* DeckComp = BoundDeckComponent.Get())
	{
		DeckComp->OnCardHandUpdated.RemoveAll(this);
		DeckComp->OnPileCountsChanged.RemoveAll(this);
		DeckComp->OnPileSummariesUpdated.RemoveAll(this);
		DeckComp->OnCardCycleTriggered.RemoveAll(this);
		DeckComp->OnCycleSettingsChanged.RemoveAll(this);
		DeckComp->OnCycleTimerUpdated.RemoveAll(this);
	}
	BoundDeckComponent.Reset();
}

void UFCHUDPresenterComponent::BindDeckComponentEvents()
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalPlayerController() || !HUDViewModel)
	{
		return;
	}

	AFCPlayerState* FCPS = PC->GetPlayerState<AFCPlayerState>();
	UFCCardDeckComponent* DeckComp = FCPS ? FCPS->GetCardDeckComponent() : nullptr;
	if (!DeckComp)
	{
		return;
	}

	UnbindDeckComponentEvents();
	BoundDeckComponent = DeckComp;

	DeckComp->OnCardHandUpdated.AddDynamic(this, &UFCHUDPresenterComponent::SyncHandToViewModel);
	DeckComp->OnPileCountsChanged.AddDynamic(this, &UFCHUDPresenterComponent::OnDeckPileCountsChanged);
	DeckComp->OnPileSummariesUpdated.AddDynamic(this, &UFCHUDPresenterComponent::SyncPileSummariesToViewModel);
	DeckComp->OnCardCycleTriggered.AddDynamic(this, &UFCHUDPresenterComponent::OnDeckCycleTriggered);
	DeckComp->OnCycleSettingsChanged.AddDynamic(this, &UFCHUDPresenterComponent::OnDeckCycleSettingsChanged);
	DeckComp->OnCycleTimerUpdated.AddDynamic(this, &UFCHUDPresenterComponent::UpdateCycleHUD);

	SyncHandToViewModel();
	SyncPileSummariesToViewModel();
	HUDViewModel->SetDrawPileCount(DeckComp->GetDrawPileCount());
	HUDViewModel->SetDiscardPileCount(DeckComp->GetDiscardPileCount());
	HUDViewModel->SetExhaustPileCount(DeckComp->GetExhaustPileCount());
	UpdateCycleHUD();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CycleHUDUpdateTimerHandle,
			this,
			&UFCHUDPresenterComponent::UpdateCycleHUD,
			0.05f,
			true);
	}
}

void UFCHUDPresenterComponent::UnbindAttributeListeners(UFCStatusPresenterComponent* StatusPresenter)
{
	if (StatusPresenter)
	{
		StatusPresenter->UnbindFromAbilitySystem();
	}

	if (UAbilitySystemComponent* CurrentASC = BoundASC.Get())
	{
		for (const auto& Entry : BoundAttributeHandles)
		{
			CurrentASC->GetGameplayAttributeValueChangeDelegate(Entry.Key).Remove(Entry.Value);
		}
	}
	BoundAttributeHandles.Empty();
	BoundASC.Reset();
}

void UFCHUDPresenterComponent::BindAttributeListeners(UFCStatusPresenterComponent* StatusPresenter)
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalPlayerController() || !HUDViewModel)
	{
		return;
	}

	UnbindAttributeListeners(StatusPresenter);

	APawn* ControlledPawn = PC->GetPawn();
	UAbilitySystemComponent* ASC = nullptr;

	if (const IAbilitySystemInterface* PawnASI = Cast<IAbilitySystemInterface>(ControlledPawn))
	{
		ASC = PawnASI->GetAbilitySystemComponent();
	}
	else if (const IAbilitySystemInterface* PSASI = Cast<IAbilitySystemInterface>(PC->GetPlayerState<APlayerState>()))
	{
		ASC = PSASI->GetAbilitySystemComponent();
	}

	if (!ASC)
	{
		return;
	}

	BoundASC = ASC;

	if (StatusPresenter)
	{
		StatusPresenter->BindToAbilitySystem(ASC, HUDViewModel->GetOrCreateStatusContainerViewModel());
	}

	if (const UFCAttributeSet* AttrSet = ASC->GetSet<UFCAttributeSet>())
	{
		HUDViewModel->SetCurrentHealth(FMath::RoundToInt(AttrSet->GetHealth()));
		HUDViewModel->SetMaxHealth(FMath::RoundToInt(AttrSet->GetMaxHealth()));
		HUDViewModel->SetCurrentMana(FMath::RoundToInt(AttrSet->GetMana()));
		HUDViewModel->SetMaxMana(FMath::RoundToInt(AttrSet->GetMaxMana()));
		HUDViewModel->SetCurrentShield(FMath::RoundToInt(AttrSet->GetShield()));
		HUDViewModel->SetMaxShield(FMath::RoundToInt(AttrSet->GetMaxShield()));
	}

	TWeakObjectPtr<UFCHUDPresenterComponent> WeakSelf(this);
	auto RegisterAttrListener = [this, WeakSelf, ASC](const FGameplayAttribute& Attribute, TFunction<void(UFCHUDViewModel*, float)> Updater)
	{
		FDelegateHandle Handle = ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddWeakLambda(
			this,
			[WeakSelf, Updater = MoveTemp(Updater)](const FOnAttributeChangeData& Data)
			{
				if (const UFCHUDPresenterComponent* Self = WeakSelf.Get())
				{
					if (Self->HUDViewModel)
					{
						Updater(Self->HUDViewModel, Data.NewValue);
					}
				}
			});
		BoundAttributeHandles.Add(MakeTuple(Attribute, Handle));
	};

	RegisterAttrListener(UFCAttributeSet::GetHealthAttribute(), [](UFCHUDViewModel* VM, float NewVal) {
		VM->SetCurrentHealth(FMath::RoundToInt(NewVal));
	});
	RegisterAttrListener(UFCAttributeSet::GetMaxHealthAttribute(), [](UFCHUDViewModel* VM, float NewVal) {
		VM->SetMaxHealth(FMath::RoundToInt(NewVal));
	});
	RegisterAttrListener(UFCAttributeSet::GetManaAttribute(), [](UFCHUDViewModel* VM, float NewVal) {
		VM->SetCurrentMana(FMath::RoundToInt(NewVal));
	});
	RegisterAttrListener(UFCAttributeSet::GetMaxManaAttribute(), [](UFCHUDViewModel* VM, float NewVal) {
		VM->SetMaxMana(FMath::RoundToInt(NewVal));
	});
	RegisterAttrListener(UFCAttributeSet::GetShieldAttribute(), [](UFCHUDViewModel* VM, float NewVal) {
		VM->SetCurrentShield(FMath::RoundToInt(NewVal));
	});
	RegisterAttrListener(UFCAttributeSet::GetMaxShieldAttribute(), [](UFCHUDViewModel* VM, float NewVal) {
		VM->SetMaxShield(FMath::RoundToInt(NewVal));
	});
}

void UFCHUDPresenterComponent::SyncHandToViewModel()
{
	if (!HUDViewModel)
	{
		return;
	}

	UFCCardDeckComponent* DeckComp = BoundDeckComponent.Get();
	if (!DeckComp)
	{
		return;
	}

	if (UFCHandViewModel* HandVM = HUDViewModel->GetOrCreateHandViewModel())
	{
		UFCCardSubsystem* CardSubsystem = UFCCardSubsystem::GetCardSubsystem(this);
		HandVM->SyncFromHandContainer(DeckComp->GetHandContainer(), CardSubsystem);
	}
}

void UFCHUDPresenterComponent::SyncPileSummariesToViewModel()
{
	if (!HUDViewModel)
	{
		return;
	}

	UFCCardDeckComponent* DeckComp = BoundDeckComponent.Get();
	if (!DeckComp)
	{
		return;
	}

	if (UFCCardPileViewModel* PileVM = HUDViewModel->GetOrCreatePileViewerViewModel())
	{
		UFCCardSubsystem* CardSubsystem = UFCCardSubsystem::GetCardSubsystem(this);
		PileVM->UpdatePileSummaries(DeckComp->GetDrawPileSummary(), DeckComp->GetDiscardPileSummary(), CardSubsystem);
	}
}

void UFCHUDPresenterComponent::OnDeckPileCountsChanged(int32 DrawCount, int32 DiscardCount, int32 ExhaustCount)
{
	if (HUDViewModel)
	{
		HUDViewModel->SetDrawPileCount(DrawCount);
		HUDViewModel->SetDiscardPileCount(DiscardCount);
		HUDViewModel->SetExhaustPileCount(ExhaustCount);
	}
}

void UFCHUDPresenterComponent::OnDeckCycleTriggered()
{
	UpdateCycleHUD();
}

void UFCHUDPresenterComponent::OnDeckCycleSettingsChanged(float NewInterval, int32 NewDrawCount)
{
	UpdateCycleHUD();
}

void UFCHUDPresenterComponent::UpdateCycleHUD()
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalPlayerController() || !HUDViewModel)
	{
		return;
	}

	if (UFCCardDeckComponent* DeckComp = BoundDeckComponent.Get())
	{
		HUDViewModel->SetCycleInterval(DeckComp->GetCycleInterval());
		HUDViewModel->SetCycleRemainingTime(DeckComp->GetCycleRemainingTime());
		HUDViewModel->SetCycleProgress(DeckComp->GetCycleProgress());
	}
}
