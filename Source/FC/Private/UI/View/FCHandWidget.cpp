#include "UI/View/FCHandWidget.h"
#include "UI/View/FCCardWidget.h"
#include "UI/ViewModel/FCHandViewModel.h"
#include "UI/ViewModel/FCCardViewModel.h"
#include "Controller/Player/FCPlayerController.h"
#include "Card/FCCardDeckComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Components/PanelWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Framework/Application/SlateApplication.h"

UFCHandWidget::UFCHandWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, HandViewModel(nullptr)
	, HoveredCardWidget(nullptr)
	, DraggedCardWidget(nullptr)
	, HeldCardWidget(nullptr)
{
	bHasScriptImplementedTick = true;
	SetIsFocusable(true);
}

void UFCHandWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Update held card target position to follow mouse cursor precisely
	if (HeldCardWidget.IsValid())
	{
		FVector2D AbsoluteCursorPos = FVector2D::ZeroVector;
		bool bGotCursor = false;

		if (FSlateApplication::IsInitialized())
		{
			AbsoluteCursorPos = FSlateApplication::Get().GetCursorPos();
			bGotCursor = true;
		}
		else if (APlayerController* PC = GetOwningPlayer())
		{
			float MouseX = 0.0f, MouseY = 0.0f;
			if (PC->GetMousePosition(MouseX, MouseY))
			{
				AbsoluteCursorPos = FVector2D(MouseX, MouseY);
				bGotCursor = true;
			}
		}

		if (bGotCursor)
		{
			const FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(AbsoluteCursorPos);
			const FVector2D ContainerSize = MyGeometry.GetLocalSize();

			// Determine card dimensions to accurately center card onto cursor
			FVector2D CardSize = HeldCardWidget->GetCachedGeometry().GetLocalSize();
			if (CardSize.Y <= 0.0f)
			{
				CardSize = HeldCardWidget->GetDesiredSize();
			}
			if (CardSize.Y <= 0.0f)
			{
				CardSize = FVector2D(150.0f, 220.0f);
			}

			// Center of card relative to default un-transformed position
			const FVector2D AnchorPos = FVector2D(ContainerSize.X * 0.5f, ContainerSize.Y - CardSize.Y * 0.5f);
			const FVector2D TargetTranslation = LocalMousePos - AnchorPos;

			HeldCardWidget->SetCardTargetTransform(
				TargetTranslation,
				0.0f,
				FVector2D(HoverScale, HoverScale),
				FVector2D(0.5f, 0.5f),
				1000,
				true);
		}
	}

	if (bEnableInterp)
	{
		for (UFCCardWidget* CardWidget : ActiveCardWidgets)
		{
			if (CardWidget && !CardWidget->IsDragging() && !CardWidget->IsHeldByHotKey() && CardWidget != HeldCardWidget.Get())
			{
				CardWidget->UpdateCardTransform(InDeltaTime, LayoutInterpSpeed);
			}
		}
	}
}

FReply UFCHandWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (HeldCardWidget.IsValid())
		{
			PlayHeldCard();
			return FReply::Handled();
		}
	}
	else if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (HeldCardWidget.IsValid())
		{
			ClearHeldCard();
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UFCHandWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::One || Key == EKeys::NumPadOne) { SelectAndHoldCardByIndex(0); return FReply::Handled(); }
	if (Key == EKeys::Two || Key == EKeys::NumPadTwo) { SelectAndHoldCardByIndex(1); return FReply::Handled(); }
	if (Key == EKeys::Three || Key == EKeys::NumPadThree) { SelectAndHoldCardByIndex(2); return FReply::Handled(); }
	if (Key == EKeys::Four || Key == EKeys::NumPadFour) { SelectAndHoldCardByIndex(3); return FReply::Handled(); }
	if (Key == EKeys::Five || Key == EKeys::NumPadFive) { SelectAndHoldCardByIndex(4); return FReply::Handled(); }
	if (Key == EKeys::Six || Key == EKeys::NumPadSix) { SelectAndHoldCardByIndex(5); return FReply::Handled(); }
	if (Key == EKeys::Seven || Key == EKeys::NumPadSeven) { SelectAndHoldCardByIndex(6); return FReply::Handled(); }
	if (Key == EKeys::Eight || Key == EKeys::NumPadEight) { SelectAndHoldCardByIndex(7); return FReply::Handled(); }
	if (Key == EKeys::Nine || Key == EKeys::NumPadNine) { SelectAndHoldCardByIndex(8); return FReply::Handled(); }
	if (Key == EKeys::Zero || Key == EKeys::NumPadZero) { SelectAndHoldCardByIndex(9); return FReply::Handled(); }
	if (Key == EKeys::Escape && HeldCardWidget.IsValid()) { ClearHeldCard(); return FReply::Handled(); }

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UFCHandWidget::SetHandViewModel(UFCHandViewModel* InViewModel)
{
	if (HandViewModel)
	{
		HandViewModel->OnCardsUpdated.RemoveAll(this);
	}

	HandViewModel = InViewModel;

	if (HandViewModel)
	{
		HandViewModel->OnCardsUpdated.AddDynamic(this, &UFCHandWidget::OnHandCardsUpdated);
	}

	OnHandViewModelAssigned(InViewModel);

	if (CardContainer)
	{
		RefreshCardWidgets(CardContainer);
	}
}

void UFCHandWidget::OnHandCardsUpdated()
{
	if (CardContainer)
	{
		RefreshCardWidgets(CardContainer);
	}

	OnHandCardsChanged();
}

void UFCHandWidget::HandleCardClicked(UFCCardWidget* ClickedCardWidget)
{
	if (!ClickedCardWidget)
	{
		return;
	}

	// If this card is currently held, clicking it immediately plays it!
	if (HeldCardWidget.Get() == ClickedCardWidget || ClickedCardWidget->IsHeldByHotKey())
	{
		PlayHeldCard();
		return;
	}

	if (UFCCardViewModel* CardVM = ClickedCardWidget->GetCardViewModel())
	{
		if (HandViewModel)
		{
			HandViewModel->SelectCardByGuid(CardVM->CardGuid);
		}
	}

	UpdateFanLayout(false);
	OnHandCardSelected.Broadcast(ClickedCardWidget);
}

void UFCHandWidget::HandleCardHovered(UFCCardWidget* InHoveredCardWidget, bool bIsHovered)
{
	if (!InHoveredCardWidget)
	{
		return;
	}

	if (bIsHovered)
	{
		HoveredCardWidget = InHoveredCardWidget;
		if (UFCCardViewModel* CardVM = InHoveredCardWidget->GetCardViewModel())
		{
			if (HandViewModel)
			{
				HandViewModel->HoverCardByGuid(CardVM->CardGuid);
			}
		}
	}
	else
	{
		if (HoveredCardWidget.Get() == InHoveredCardWidget)
		{
			HoveredCardWidget = nullptr;
			if (HandViewModel)
			{
				HandViewModel->ClearHover();
			}
		}
	}

	UpdateFanLayout(false);
}

void UFCHandWidget::HandleCardDragStarted(UFCCardWidget* DraggedCard)
{
	if (!DraggedCard)
	{
		return;
	}

	DraggedCardWidget = DraggedCard;

	if (UFCCardViewModel* CardVM = DraggedCard->GetCardViewModel())
	{
		if (HandViewModel)
		{
			HandViewModel->SelectCardByGuid(CardVM->CardGuid);
		}
	}

	OnHandCardDragStarted.Broadcast(DraggedCard);
}

void UFCHandWidget::HandleCardDragged(UFCCardWidget* DraggedCard, const FVector2D& DragTranslation)
{
	if (!DraggedCard)
	{
		return;
	}

	OnHandCardDragged.Broadcast(DraggedCard, DragTranslation);
}

void UFCHandWidget::HandleCardDragEnded(UFCCardWidget* DraggedCard, bool bWasDragged)
{
	if (DraggedCardWidget.Get() == DraggedCard)
	{
		DraggedCardWidget = nullptr;
	}

	OnHandCardDragEnded.Broadcast(DraggedCard, bWasDragged);

	if (bWasDragged && DraggedCard)
	{
		const FVector2D DragOffset = DraggedCard->GetCurrentTranslation() - DraggedCard->GetDragStartCardTranslation();
		const bool bTriggerPlay = bAutoPlayOnDragDrop || 
			(DragOffset.Y <= PlayDragDropThresholdY) || 
			(DragOffset.Size() >= MinDragDropDistance);

		if (bTriggerPlay)
		{
			PlayCardFromHand(DraggedCard);
		}
	}

	// Return card smoothly back into fan hand layout if not consumed
	UpdateFanLayout(false);
}

void UFCHandWidget::PlayCardFromHand(UFCCardWidget* CardWidget)
{
	if (!CardWidget)
	{
		return;
	}

	if (HeldCardWidget.Get() == CardWidget)
	{
		HeldCardWidget->SetIsHeldByHotKey(false);
		HeldCardWidget = nullptr;
	}

	UFCCardViewModel* CardVM = CardWidget->GetCardViewModel();
	if (!CardVM || !CardVM->CardGuid.IsValid())
	{
		return;
	}

	if (!CardVM->bIsPlayable || CardVM->bIsLocked)
	{
		// Cannot play card due to insufficient mana or locked state
		UpdateFanLayout(false);
		return;
	}

	FFCCardTargetInfo TargetInfo;

	// 1. Submit play request via AFCPlayerController
	if (AFCPlayerController* FCPC = Cast<AFCPlayerController>(GetOwningPlayer()))
	{
		FCPC->RequestPlayCard(CardVM->CardGuid, TargetInfo);
		return;
	}

	// 2. Fallback: Directly through PlayerState or Pawn's DeckComponent
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APlayerState* PS = PC->PlayerState)
		{
			if (UFCCardDeckComponent* DeckComp = PS->FindComponentByClass<UFCCardDeckComponent>())
			{
				DeckComp->Server_PlayCard(CardVM->CardGuid, TargetInfo);
				return;
			}
		}

		if (APawn* Pawn = PC->GetPawn())
		{
			if (UFCCardDeckComponent* DeckComp = Pawn->FindComponentByClass<UFCCardDeckComponent>())
			{
				DeckComp->Server_PlayCard(CardVM->CardGuid, TargetInfo);
				return;
			}
		}
	}
}

bool UFCHandWidget::SelectAndHoldCardByIndex(int32 Index)
{
	if (!ActiveCardWidgets.IsValidIndex(Index))
	{
		return false;
	}

	UFCCardWidget* TargetCard = ActiveCardWidgets[Index];
	if (!TargetCard)
	{
		return false;
	}

	// If already holding this card, toggle/cancel hold
	if (HeldCardWidget.Get() == TargetCard)
	{
		ClearHeldCard();
		return true;
	}

	// Clear previous held card if any
	if (HeldCardWidget.IsValid())
	{
		HeldCardWidget->SetIsHeldByHotKey(false);
	}

	HeldCardWidget = TargetCard;
	TargetCard->SetIsHeldByHotKey(true);

	if (UFCCardViewModel* CardVM = TargetCard->GetCardViewModel())
	{
		if (HandViewModel)
		{
			HandViewModel->SelectCardByGuid(CardVM->CardGuid);
		}
	}

	UpdateFanLayout(false);
	OnHandCardSelected.Broadcast(TargetCard);
	return true;
}

bool UFCHandWidget::SelectAndHoldCardByGuid(const FGuid& CardGuid)
{
	if (!CardGuid.IsValid())
	{
		return false;
	}

	for (int32 Index = 0; Index < ActiveCardWidgets.Num(); ++Index)
	{
		UFCCardWidget* CardWidget = ActiveCardWidgets[Index];
		if (CardWidget)
		{
			UFCCardViewModel* CardVM = CardWidget->GetCardViewModel();
			if (CardVM && CardVM->CardGuid == CardGuid)
			{
				return SelectAndHoldCardByIndex(Index);
			}
		}
	}
	return false;
}

void UFCHandWidget::ClearHeldCard()
{
	if (HeldCardWidget.IsValid())
	{
		HeldCardWidget->SetIsHeldByHotKey(false);
		HeldCardWidget = nullptr;
	}

	if (HandViewModel)
	{
		HandViewModel->ClearSelection();
	}

	UpdateFanLayout(false);
}

bool UFCHandWidget::PlayHeldCard()
{
	if (!HeldCardWidget.IsValid())
	{
		return false;
	}

	UFCCardWidget* CardToPlay = HeldCardWidget.Get();
	ClearHeldCard();

	PlayCardFromHand(CardToPlay);
	return true;
}

void UFCHandWidget::RefreshCardWidgets(UPanelWidget* TargetPanel)
{
	if (!TargetPanel || !HandViewModel || !CardWidgetClass)
	{
		return;
	}

	TargetPanel->ClearChildren();
	ActiveCardWidgets.Empty();
	HeldCardWidget = nullptr;

	const int32 TotalCards = HandViewModel->CardsInHand.Num();
	ActiveCardWidgets.Reserve(TotalCards);

	for (int32 Index = 0; Index < TotalCards; ++Index)
	{
		UFCCardViewModel* CardVM = HandViewModel->CardsInHand[Index];
		if (CardVM)
		{
			UFCCardWidget* CardWidget = CreateWidget<UFCCardWidget>(this, CardWidgetClass);
			if (CardWidget)
			{
				TargetPanel->AddChild(CardWidget);

				// Configure slot anchors and alignment for fan layout origin
				if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(CardWidget->Slot))
				{
					CanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
					CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
					CanvasSlot->SetAutoSize(true);
					CanvasSlot->SetPosition(FVector2D::ZeroVector);
				}
				else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(CardWidget->Slot))
				{
					OverlaySlot->SetHorizontalAlignment(HAlign_Center);
					OverlaySlot->SetVerticalAlignment(VAlign_Bottom);
				}

				CardWidget->SetCardViewModel(CardVM);
				CardWidget->OnCardClicked.AddDynamic(this, &UFCHandWidget::HandleCardClicked);
				CardWidget->OnCardHovered.AddDynamic(this, &UFCHandWidget::HandleCardHovered);
				CardWidget->OnCardDragStarted.AddDynamic(this, &UFCHandWidget::HandleCardDragStarted);
				CardWidget->OnCardDragged.AddDynamic(this, &UFCHandWidget::HandleCardDragged);
				CardWidget->OnCardDragEnded.AddDynamic(this, &UFCHandWidget::HandleCardDragEnded);

				ActiveCardWidgets.Add(CardWidget);
			}
		}
	}

	UpdateFanLayout(true);
}

void UFCHandWidget::CalculateFanTransformForIndex(
	int32 CardIndex,
	int32 TotalCards,
	bool bIsCardHovered,
	bool bIsCardSelected,
	FVector2D& OutTranslation,
	float& OutAngle,
	FVector2D& OutScale,
	int32& OutZOrder) const
{
	UFCHandViewModel::CalculateCardFanTransform(
		CardIndex,
		TotalCards,
		CardSpacing,
		MaxHandWidth,
		ArcHeight,
		MaxFanAngle,
		AngleStep,
		OutTranslation,
		OutAngle);

	OutScale = FVector2D(1.0f, 1.0f);
	OutZOrder = CardIndex;

	if (bIsCardSelected)
	{
		OutTranslation.Y += SelectedLiftY;
		OutScale = FVector2D(SelectedScale, SelectedScale);
		if (bStraightenOnHover)
		{
			OutAngle = 0.0f;
		}
		OutZOrder = 200 + CardIndex;
	}
	else if (bIsCardHovered)
	{
		OutTranslation.Y += HoverLiftY;
		OutScale = FVector2D(HoverScale, HoverScale);
		if (bStraightenOnHover)
		{
			OutAngle = 0.0f;
		}
		OutZOrder = 100 + CardIndex;
	}
}

void UFCHandWidget::UpdateFanLayout(bool bImmediate)
{
	const int32 TotalCards = ActiveCardWidgets.Num();
	if (TotalCards == 0)
	{
		return;
	}

	const int32 SelectedIdx = HandViewModel ? HandViewModel->SelectedCardIndex : INDEX_NONE;
	const int32 HoveredIdx = HandViewModel ? HandViewModel->HoveredCardIndex : INDEX_NONE;

	for (int32 Index = 0; Index < TotalCards; ++Index)
	{
		UFCCardWidget* CardWidget = ActiveCardWidgets[Index];
		if (!CardWidget || CardWidget->IsDragging() || CardWidget->IsHeldByHotKey() || CardWidget == HeldCardWidget.Get())
		{
			continue;
		}

		UFCCardViewModel* CardVM = CardWidget->GetCardViewModel();
		const bool bIsSelected = (Index == SelectedIdx) || (CardVM && CardVM->bIsSelected);
		const bool bIsHovered = (Index == HoveredIdx) || (CardVM && CardVM->bIsHovered);

		FVector2D TargetTranslation = FVector2D::ZeroVector;
		float TargetAngle = 0.0f;
		FVector2D TargetScale = FVector2D(1.0f, 1.0f);
		int32 TargetZOrder = Index;

		CalculateFanTransformForIndex(
			Index,
			TotalCards,
			bIsHovered,
			bIsSelected,
			TargetTranslation,
			TargetAngle,
			TargetScale,
			TargetZOrder);

		if (CardVM)
		{
			CardVM->SetTargetFanAngle(TargetAngle);
			CardVM->SetTargetFanOffset(TargetTranslation);
		}

		CardWidget->SetCardTargetTransform(
			TargetTranslation,
			TargetAngle,
			TargetScale,
			RenderPivot,
			TargetZOrder,
			bImmediate || !bEnableInterp);
	}
}
