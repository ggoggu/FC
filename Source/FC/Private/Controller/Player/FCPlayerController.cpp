#include "Controller/Player/FCPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

AFCPlayerController::AFCPlayerController()
{
	bShowMouseCursor = false;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AFCPlayerController::BeginPlay()
{
	Super::BeginPlay();

	InitializeDefaultMappingContext();
}

void AFCPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
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
