#include "UI/View/FCCardRewardWidget.h"
#include "UI/ViewModel/FCCardRewardViewModel.h"
#include "Gameplay/FCCardPickupActor.h"
#include "Data/Card/FCCardSubsystem.h"
#include "GameFramework/PlayerController.h"

UFCCardRewardWidget::UFCCardRewardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, RewardViewModel(nullptr)
	, SourcePickupActor(nullptr)
{
	SetIsFocusable(true);
}

void UFCCardRewardWidget::SetupRewardWidget(FName InCardId, AFCCardPickupActor* InSourceActor)
{
	SourcePickupActor = InSourceActor;

	if (!RewardViewModel)
	{
		RewardViewModel = NewObject<UFCCardRewardViewModel>(this);
	}

	UFCCardSubsystem* Subsystem = UFCCardSubsystem::GetCardSubsystem(this);
	RewardViewModel->SetupReward(InCardId, Subsystem, AcquireKey, DiscardKey);

	OnRewardViewModelAssigned(RewardViewModel);

	// Set input mode to UI/Game and take keyboard focus for instant shortcut response
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}

	SetKeyboardFocus();
}

FReply UFCCardRewardWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey PressedKey = InKeyEvent.GetKey();

	if (AcquireKey.IsValid() && PressedKey == AcquireKey)
	{
		OnAcquireClicked();
		return FReply::Handled();
	}

	if (DiscardKey.IsValid() && PressedKey == DiscardKey)
	{
		OnDiscardClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UFCCardRewardWidget::OnAcquireClicked()
{
	if (SourcePickupActor.IsValid())
	{
		SourcePickupActor->Server_ClaimPickup(GetOwningPlayerPawn());
	}

	CloseRewardWidget();
}

void UFCCardRewardWidget::OnDiscardClicked()
{
	CloseRewardWidget();
}

void UFCCardRewardWidget::CloseRewardWidget()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}

	RemoveFromParent();
}
