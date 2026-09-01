#include "Controller/Player/FCPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Data/Card/FCCardTypes.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Card/FCCardDeckComponent.h"
#include "Game/FCPlayerState.h"
#include "UI/View/FCHUDWidget.h"
#include "UI/ViewModel/FCHUDViewModel.h"
#include "UI/ViewModel/FCHandViewModel.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"

AFCPlayerController::AFCPlayerController()
{
	bShowMouseCursor = false;
	DefaultMouseCursor = EMouseCursor::Default;
	bAutoEnableCardCombatInputMode = true;
}

void AFCPlayerController::BeginPlay()
{
	Super::BeginPlay();

	InitializeDefaultMappingContext();

	if (IsLocalPlayerController())
	{
		SetupHUD();
	}
}

void AFCPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}

void AFCPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (IsLocalPlayerController())
	{
		SetupHUD();
	}
}

void AFCPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	if (IsLocalPlayerController())
	{
		SetupHUD();
	}
}

void AFCPlayerController::SetupHUD()
{
	if (!IsLocalPlayerController() || !HUDWidgetClass)
	{
		return;
	}

	if (!HUDViewModel)
	{
		HUDViewModel = NewObject<UFCHUDViewModel>(this);
	}

	if (!HUDWidget)
	{
		HUDWidget = CreateWidget<UFCHUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->SetHUDViewModel(HUDViewModel);
			HUDWidget->AddToViewport();
		}
	}
	else
	{
		HUDWidget->SetHUDViewModel(HUDViewModel);
	}

	BindDeckComponentEvents();
	BindAttributeListeners();

	if (bAutoEnableCardCombatInputMode)
	{
		SetCardCombatInputMode(true);
	}
}

void AFCPlayerController::BindDeckComponentEvents()
{
	if (!IsLocalPlayerController() || !HUDViewModel)
	{
		return;
	}

	AFCPlayerState* FCPS = GetPlayerState<AFCPlayerState>();
	if (!FCPS)
	{
		return;
	}

	UFCCardDeckComponent* DeckComp = FCPS->GetCardDeckComponent();
	if (!DeckComp)
	{
		return;
	}

	// Avoid duplicate delegate bindings
	DeckComp->OnCardHandUpdated.RemoveAll(this);
	DeckComp->OnPileCountsChanged.RemoveAll(this);

	DeckComp->OnCardHandUpdated.AddDynamic(this, &AFCPlayerController::SyncHandToViewModel);
	DeckComp->OnPileCountsChanged.AddDynamic(this, &AFCPlayerController::OnDeckPileCountsChanged);

	// Perform initial sync
	SyncHandToViewModel();
	HUDViewModel->SetDrawPileCount(DeckComp->GetDrawPileCount());
	HUDViewModel->SetDiscardPileCount(DeckComp->GetDiscardPileCount());
	HUDViewModel->SetExhaustPileCount(DeckComp->GetExhaustPileCount());
}

void AFCPlayerController::BindAttributeListeners()
{
	if (!IsLocalPlayerController() || !HUDViewModel)
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	UAbilitySystemComponent* ASC = nullptr;

	if (const IAbilitySystemInterface* PawnASI = Cast<IAbilitySystemInterface>(ControlledPawn))
	{
		ASC = PawnASI->GetAbilitySystemComponent();
	}
	else if (const IAbilitySystemInterface* PSASI = Cast<IAbilitySystemInterface>(GetPlayerState<APlayerState>()))
	{
		ASC = PSASI->GetAbilitySystemComponent();
	}

	if (!ASC)
	{
		return;
	}

	const UFCAttributeSet* AttrSet = ASC->GetSet<UFCAttributeSet>();
	if (AttrSet)
	{
		HUDViewModel->SetCurrentHealth(FMath::RoundToInt(AttrSet->GetHealth()));
		HUDViewModel->SetMaxHealth(FMath::RoundToInt(AttrSet->GetMaxHealth()));
		HUDViewModel->SetCurrentMana(FMath::RoundToInt(AttrSet->GetMana()));
		HUDViewModel->SetMaxMana(FMath::RoundToInt(AttrSet->GetMaxMana()));
	}

	ASC->GetGameplayAttributeValueChangeDelegate(UFCAttributeSet::GetHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			if (HUDViewModel)
			{
				HUDViewModel->SetCurrentHealth(FMath::RoundToInt(Data.NewValue));
			}
		});

	ASC->GetGameplayAttributeValueChangeDelegate(UFCAttributeSet::GetMaxHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			if (HUDViewModel)
			{
				HUDViewModel->SetMaxHealth(FMath::RoundToInt(Data.NewValue));
			}
		});

	ASC->GetGameplayAttributeValueChangeDelegate(UFCAttributeSet::GetManaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			if (HUDViewModel)
			{
				HUDViewModel->SetCurrentMana(FMath::RoundToInt(Data.NewValue));
			}
		});

	ASC->GetGameplayAttributeValueChangeDelegate(UFCAttributeSet::GetMaxManaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			if (HUDViewModel)
			{
				HUDViewModel->SetMaxMana(FMath::RoundToInt(Data.NewValue));
			}
		});
}

void AFCPlayerController::SyncHandToViewModel()
{
	if (!HUDViewModel)
	{
		return;
	}

	AFCPlayerState* FCPS = GetPlayerState<AFCPlayerState>();
	if (!FCPS)
	{
		return;
	}

	UFCCardDeckComponent* DeckComp = FCPS->GetCardDeckComponent();
	if (!DeckComp)
	{
		return;
	}

	UFCHandViewModel* HandVM = HUDViewModel->GetOrCreateHandViewModel();
	if (HandVM)
	{
		UFCCardSubsystem* CardSubsystem = UFCCardSubsystem::GetCardSubsystem(this);
		HandVM->SyncFromHandContainer(DeckComp->GetHandContainer(), CardSubsystem);
	}
}

void AFCPlayerController::OnDeckPileCountsChanged(int32 DrawCount, int32 DiscardCount, int32 ExhaustCount)
{
	if (HUDViewModel)
	{
		HUDViewModel->SetDrawPileCount(DrawCount);
		HUDViewModel->SetDiscardPileCount(DiscardCount);
		HUDViewModel->SetExhaustPileCount(ExhaustCount);
	}
}

void AFCPlayerController::InitializeDefaultMappingContext()
{
	if (IsLocalPlayerController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, DefaultMappingPriority);
			}
		}
	}
}

void AFCPlayerController::SwitchMappingContext(UInputMappingContext* NewContext, int32 Priority)
{
	if (!IsLocalPlayerController() || !NewContext)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
		Subsystem->AddMappingContext(NewContext, Priority);
	}
}

void AFCPlayerController::SetCardCombatInputMode(bool bEnableCardMode)
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (bEnableCardMode)
	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);

		if (CardCombatMappingContext)
		{
			SwitchMappingContext(CardCombatMappingContext, CardCombatMappingPriority);
		}
	}
	else
	{
		bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);

		if (DefaultMappingContext)
		{
			SwitchMappingContext(DefaultMappingContext, DefaultMappingPriority);
		}
	}
}

void AFCPlayerController::RequestPlayCard(const FGuid& CardGuid, const FFCCardTargetInfo& TargetInfo)
{
	if (AFCPlayerState* FCPS = GetPlayerState<AFCPlayerState>())
	{
		if (UFCCardDeckComponent* DeckComp = FCPS->GetCardDeckComponent())
		{
			DeckComp->Server_PlayCard(CardGuid, TargetInfo);
		}
	}
}
