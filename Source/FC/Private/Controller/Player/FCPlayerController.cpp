#include "Controller/Player/FCPlayerController.h"
#include "Controller/Player/FCCardTargetingComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Card/FCCardDeckComponent.h"
#include "Game/FCPlayerState.h"
#include "UI/View/FCHUDWidget.h"
#include "UI/View/FCHandWidget.h"
#include "UI/ViewModel/FCHUDViewModel.h"
#include "UI/ViewModel/FCStatusToggleContainerViewModel.h"
#include "UI/Data/FCStatusToggleTypes.h"
#include "UI/Presenter/FCStatusPresenterComponent.h"
#include "UI/Presenter/FCHUDPresenterComponent.h"
#include "Character/FCCharacterBase.h"
#include "Gameplay/FCCardPickupActor.h"
#include "Engine/World.h"

AFCPlayerController::AFCPlayerController()
{
	bShowMouseCursor = false;
	DefaultMouseCursor = EMouseCursor::Default;
	bAutoEnableCardCombatInputMode = true;

	StatusPresenterComponent = CreateDefaultSubobject<UFCStatusPresenterComponent>(TEXT("StatusPresenterComponent"));
	HUDPresenterComponent = CreateDefaultSubobject<UFCHUDPresenterComponent>(TEXT("HUDPresenterComponent"));
	CardTargetingComponent = CreateDefaultSubobject<UFCCardTargetingComponent>(TEXT("CardTargetingComponent"));
}

void AFCPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (CardTargetingComponent)
	{
		CardTargetingComponent->ConfigureTargetingParams(
			bAutoTargetAttackablesOnProjectileCards,
			ProjectileTargetMaxRange,
			ProjectileTargetHalfAngleDegrees,
			ProjectileTargetProximityRadius);
	}

	InitializeDefaultMappingContext();

	if (IsLocalPlayerController())
	{
		SetupHUD();
	}
}

void AFCPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AFCPlayerController::OnNumberKey1Pressed);
		InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AFCPlayerController::OnNumberKey2Pressed);
		InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AFCPlayerController::OnNumberKey3Pressed);
		InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AFCPlayerController::OnNumberKey4Pressed);
		InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AFCPlayerController::OnNumberKey5Pressed);
		InputComponent->BindKey(EKeys::Six, IE_Pressed, this, &AFCPlayerController::OnNumberKey6Pressed);
		InputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &AFCPlayerController::OnNumberKey7Pressed);
		InputComponent->BindKey(EKeys::Eight, IE_Pressed, this, &AFCPlayerController::OnNumberKey8Pressed);
		InputComponent->BindKey(EKeys::Nine, IE_Pressed, this, &AFCPlayerController::OnNumberKey9Pressed);
		InputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &AFCPlayerController::OnNumberKey0Pressed);

		InputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &AFCPlayerController::OnNumberKey1Pressed);
		InputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &AFCPlayerController::OnNumberKey2Pressed);
		InputComponent->BindKey(EKeys::NumPadThree, IE_Pressed, this, &AFCPlayerController::OnNumberKey3Pressed);
		InputComponent->BindKey(EKeys::NumPadFour, IE_Pressed, this, &AFCPlayerController::OnNumberKey4Pressed);
		InputComponent->BindKey(EKeys::NumPadFive, IE_Pressed, this, &AFCPlayerController::OnNumberKey5Pressed);
		InputComponent->BindKey(EKeys::NumPadSix, IE_Pressed, this, &AFCPlayerController::OnNumberKey6Pressed);
		InputComponent->BindKey(EKeys::NumPadSeven, IE_Pressed, this, &AFCPlayerController::OnNumberKey7Pressed);
		InputComponent->BindKey(EKeys::NumPadEight, IE_Pressed, this, &AFCPlayerController::OnNumberKey8Pressed);
		InputComponent->BindKey(EKeys::NumPadNine, IE_Pressed, this, &AFCPlayerController::OnNumberKey9Pressed);
		InputComponent->BindKey(EKeys::NumPadZero, IE_Pressed, this, &AFCPlayerController::OnNumberKey0Pressed);

		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AFCPlayerController::OnLeftMouseButtonPressed);
		InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AFCPlayerController::OnRightMouseButtonPressed);
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AFCPlayerController::OnRightMouseButtonPressed);
	}
}

void AFCPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (IsLocalPlayerController())
	{
		SetupHUD();
	}
}

void AFCPlayerController::OnUnPossess()
{
	if (HUDPresenterComponent)
	{
		HUDPresenterComponent->UnbindAttributeListeners(StatusPresenterComponent);
	}
	Super::OnUnPossess();
}

void AFCPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HUDPresenterComponent)
	{
		HUDPresenterComponent->UnbindAttributeListeners(StatusPresenterComponent);
	}
	Super::EndPlay(EndPlayReason);
}

void AFCPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	if (IsLocalPlayerController())
	{
		SetupHUD();
	}
}

void AFCPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (IsLocalPlayerController())
	{
		SetupHUD();
	}
}

void AFCPlayerController::SetupHUD()
{
	if (!IsLocalPlayerController() || !HUDWidgetClass || !HUDPresenterComponent)
	{
		return;
	}

	HUDPresenterComponent->SetupHUD(HUDWidgetClass, StatusPresenterComponent);
	HUDWidget = HUDPresenterComponent->GetHUDWidget();
	HUDViewModel = HUDPresenterComponent->GetHUDViewModel();

	if (bAutoEnableCardCombatInputMode)
	{
		SetCardCombatInputMode(true);
	}
}

UFCHUDWidget* AFCPlayerController::GetHUDWidget() const
{
	return HUDPresenterComponent ? HUDPresenterComponent->GetHUDWidget() : HUDWidget.Get();
}

UFCHUDViewModel* AFCPlayerController::GetHUDViewModel() const
{
	return HUDPresenterComponent ? HUDPresenterComponent->GetHUDViewModel() : HUDViewModel.Get();
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
	if (IsLocalPlayerController())
	{
		if (AFCCharacterBase* Char = Cast<AFCCharacterBase>(GetPawn()))
		{
			const FVector AimPos = TargetInfo.TargetActor.IsValid()
				? TargetInfo.TargetActor->GetActorLocation()
				: static_cast<FVector>(TargetInfo.TargetLocation);

			if (!AimPos.IsZero())
			{
				Char->RotateTowardsTarget(AimPos);
			}
		}
	}

	if (AFCPlayerState* FCPS = GetPlayerState<AFCPlayerState>())
	{
		if (UFCCardDeckComponent* DeckComp = FCPS->GetCardDeckComponent())
		{
			DeckComp->Server_PlayCard(CardGuid, TargetInfo);
		}
	}
}

FFCCardTargetInfo AFCPlayerController::ResolveCardTargetUnderCursor(const UFCCardDataAsset* CardAsset)
{
	if (CardTargetingComponent)
	{
		CardTargetingComponent->ConfigureTargetingParams(
			bAutoTargetAttackablesOnProjectileCards,
			ProjectileTargetMaxRange,
			ProjectileTargetHalfAngleDegrees,
			ProjectileTargetProximityRadius);
		return CardTargetingComponent->ResolveCardTargetUnderCursor(CardAsset);
	}
	return FFCCardTargetInfo();
}

FFCCardTargetInfo AFCPlayerController::ResolveCardTargetAtScreenPosition(const FVector2D& ScreenPos, const UFCCardDataAsset* CardAsset)
{
	if (CardTargetingComponent)
	{
		CardTargetingComponent->ConfigureTargetingParams(
			bAutoTargetAttackablesOnProjectileCards,
			ProjectileTargetMaxRange,
			ProjectileTargetHalfAngleDegrees,
			ProjectileTargetProximityRadius);
		return CardTargetingComponent->ResolveCardTargetAtScreenPosition(ScreenPos, CardAsset);
	}
	return FFCCardTargetInfo();
}

void AFCPlayerController::HandleNumberKeyInput(int32 SlotIndex)
{
	UFCHUDWidget* ActiveHUD = GetHUDWidget();
	UFCHUDViewModel* ActiveVM = GetHUDViewModel();
	if (!IsLocalPlayerController() || !ActiveHUD)
	{
		return;
	}

	if (ActiveVM && ActiveVM->IsPileViewerOpen())
	{
		return;
	}

	if (UFCHandWidget* HandWidget = ActiveHUD->GetHandWidget())
	{
		HandWidget->SelectAndHoldCardByIndex(SlotIndex);
	}
}

void AFCPlayerController::OnLeftMouseButtonPressed()
{
	UFCHUDWidget* ActiveHUD = GetHUDWidget();
	UFCHUDViewModel* ActiveVM = GetHUDViewModel();
	if (!IsLocalPlayerController() || !ActiveHUD)
	{
		return;
	}

	if (ActiveVM && ActiveVM->IsPileViewerOpen())
	{
		return;
	}

	if (UFCHandWidget* HandWidget = ActiveHUD->GetHandWidget())
	{
		if (HandWidget->HasHeldCard())
		{
			HandWidget->PlayHeldCard();
		}
	}
}

void AFCPlayerController::OnRightMouseButtonPressed()
{
	UFCHUDWidget* ActiveHUD = GetHUDWidget();
	UFCHUDViewModel* ActiveVM = GetHUDViewModel();
	if (!IsLocalPlayerController() || !ActiveHUD)
	{
		return;
	}

	if (ActiveVM && ActiveVM->IsPileViewerOpen())
	{
		ActiveVM->ClosePileViewer();
		return;
	}

	if (UFCHandWidget* HandWidget = ActiveHUD->GetHandWidget())
	{
		if (HandWidget->HasHeldCard())
		{
			HandWidget->ClearHeldCard();
		}
	}
}

bool AFCPlayerController::Server_ClaimCardPickup_Validate(AFCCardPickupActor* PickupActor, EFCCardZone Destination)
{
	return StaticEnum<EFCCardZone>()->IsValidEnumValue(static_cast<int64>(Destination));
}

void AFCPlayerController::Server_ClaimCardPickup_Implementation(AFCCardPickupActor* PickupActor, EFCCardZone Destination)
{
	if (!HasAuthority() || !IsValid(PickupActor))
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		const double Now = World->GetTimeSeconds();
		if (Now - LastClaimPickupServerTime < 0.1)
		{
			return;
		}
		LastClaimPickupServerTime = Now;
	}

	AFCCharacterBase* Char = Cast<AFCCharacterBase>(GetPawn());
	if (!Char || Char->IsDead())
	{
		return;
	}

	const float DistSq = FVector::DistSquared(Char->GetActorLocation(), PickupActor->GetActorLocation());
	if (DistSq > FMath::Square(300.0f))
	{
		return;
	}

	const EFCCardZone AuthoritativeDestination = PickupActor->GetDefaultDestination();
	PickupActor->ClaimPickup(Char, AuthoritativeDestination);
}

void AFCPlayerController::SetStatusToggle(const FFCStatusToggleData& InData)
{
	if (StatusPresenterComponent)
	{
		StatusPresenterComponent->PushStatusData(InData);
	}
	else if (UFCHUDViewModel* ActiveVM = GetHUDViewModel())
	{
		if (UFCStatusToggleContainerViewModel* StatusVM = ActiveVM->GetOrCreateStatusContainerViewModel())
		{
			StatusVM->AddOrUpdateStatus(InData);
		}
	}
}

void AFCPlayerController::RemoveStatusToggle(FName InStatusId)
{
	if (StatusPresenterComponent)
	{
		StatusPresenterComponent->RemoveStatusById(InStatusId);
	}
	else if (UFCHUDViewModel* ActiveVM = GetHUDViewModel())
	{
		if (UFCStatusToggleContainerViewModel* StatusVM = ActiveVM->GetOrCreateStatusContainerViewModel())
		{
			StatusVM->RemoveStatusById(InStatusId);
		}
	}
}
