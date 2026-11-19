#include "UI/ClassSelect/FCClassSelectPlayerController.h"
#include "UI/ClassSelect/FCClassSelectWidget.h"
#include "UI/ClassSelect/FCClassSelectViewModel.h"
#include "UI/ClassSelect/FCClassSelectGameMode.h"
#include "Data/Class/FCClassSubsystem.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"

AFCClassSelectPlayerController::AFCClassSelectPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AFCClassSelectPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	// Set UI Only input mode
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	// Focus camera on Studio CameraActor if present
	AActor* StudioCamera = UGameplayStatics::GetActorOfClass(this, ACameraActor::StaticClass());
	if (StudioCamera)
	{
		SetViewTarget(StudioCamera);
	}

	// Instantiate ViewModel
	ClassSelectViewModel = NewObject<UFCClassSelectViewModel>(this);
	if (ClassSelectViewModel)
	{
		ClassSelectViewModel->InitializeClasses(UFCClassSubsystem::GetClassSubsystem(this));
		ClassSelectViewModel->OnClassConfirmed.AddDynamic(this, &AFCClassSelectPlayerController::HandleClassConfirmed);
	}

	// Instantiate Widget
	if (ClassSelectWidgetClass)
	{
		ClassSelectWidget = CreateWidget<UFCClassSelectWidget>(this, ClassSelectWidgetClass);
		if (ClassSelectWidget)
		{
			ClassSelectWidget->InitializeWidget(ClassSelectViewModel);
			ClassSelectWidget->AddToViewport(10);
		}
	}
}

void AFCClassSelectPlayerController::HandleClassConfirmed(EFCCharacterClass ConfirmedClass, const TArray<FName>& StartingDeck)
{
	// Trigger level transition via GameMode if server/authority
	if (AFCClassSelectGameMode* GameMode = GetWorld()->GetAuthGameMode<AFCClassSelectGameMode>())
	{
		GameMode->TransitionToNextLevel();
	}
}
