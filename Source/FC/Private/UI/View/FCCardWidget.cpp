#include "UI/View/FCCardWidget.h"
#include "UI/ViewModel/FCCardViewModel.h"
#include "UI/View/FCHandWidget.h"
#include "Controller/Player/FCPlayerController.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "View/MVVMView.h"

UFCCardWidget::UFCCardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CardViewModel(nullptr)
{
	SetIsFocusable(true);
}

void UFCCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyDescriptionWrapping();

	if (CardViewModel)
	{
		if (UMVVMView* View = GetExtension<UMVVMView>())
		{
			if (!View->SetViewModel(TEXT("FCCardViewModel"), CardViewModel))
			{
				View->SetViewModelByClass(CardViewModel);
			}

			if (View->IsConstructed())
			{
				View->ExecuteViewModelBindings(TEXT("FCCardViewModel"));
			}
		}
	}
}

void UFCCardWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	ApplyDescriptionWrapping();
}

void UFCCardWidget::SetCardViewModel(UFCCardViewModel* InViewModel)
{
	CardViewModel = InViewModel;

	ApplyDescriptionWrapping();

	if (UMVVMView* View = GetExtension<UMVVMView>())
	{
		if (!View->SetViewModel(TEXT("FCCardViewModel"), InViewModel))
		{
			View->SetViewModelByClass(InViewModel);
		}

		if (View->IsConstructed())
		{
			View->ExecuteViewModelBindings(TEXT("FCCardViewModel"));
		}
	}

	OnCardViewModelAssigned(InViewModel);
}

void UFCCardWidget::ApplyDescriptionWrapping()
{
	if (Text_Description)
	{
		Text_Description->SetAutoWrapText(bAutoWrapDescription);
		if (bAutoWrapDescription && DescriptionWrapWidth > 0.0f)
		{
			Text_Description->SetWrapTextAt(DescriptionWrapWidth);
		}
	}
}

void UFCCardWidget::SetAutoWrapDescription(bool bInAutoWrap)
{
	bAutoWrapDescription = bInAutoWrap;
	ApplyDescriptionWrapping();
}

void UFCCardWidget::SetDescriptionWrapWidth(float InWrapWidth)
{
	DescriptionWrapWidth = InWrapWidth;
	ApplyDescriptionWrapping();
}

float UFCCardWidget::GetDescriptionWrapWidth() const
{
	if (DescriptionWrapWidth > 0.0f)
	{
		return DescriptionWrapWidth;
	}
	if (Text_Description)
	{
		return Text_Description->GetWrapTextAt();
	}
	return 0.0f;
}

void UFCCardWidget::SetIsHeldByHotKey(bool bInHeld)
{
	bIsHeldByHotKey = bInHeld;
	if (bInHeld)
	{
		TargetAngle = 0.0f;
		TargetZOrder = 1000;
	}
}

void UFCCardWidget::SetCardTargetTransform(
	const FVector2D& InTranslation,
	float InAngle,
	const FVector2D& InScale,
	const FVector2D& InPivot,
	int32 InZOrder,
	bool bImmediate)
{
	TargetTranslation = InTranslation;
	TargetAngle = InAngle;
	TargetScale = InScale;
	TargetPivot = InPivot;
	TargetZOrder = InZOrder;

	if (bImmediate)
	{
		CurrentTranslation = InTranslation;
		CurrentAngle = InAngle;
		CurrentScale = InScale;
		ApplyCurrentTransform();
	}
}

void UFCCardWidget::UpdateCardTransform(float DeltaTime, float InterpSpeed)
{
	if (InterpSpeed <= 0.0f || DeltaTime <= 0.0f)
	{
		CurrentTranslation = TargetTranslation;
		CurrentAngle = TargetAngle;
		CurrentScale = TargetScale;
	}
	else
	{
		CurrentTranslation = FMath::Vector2DInterpTo(CurrentTranslation, TargetTranslation, DeltaTime, InterpSpeed);
		CurrentAngle = FMath::FInterpTo(CurrentAngle, TargetAngle, DeltaTime, InterpSpeed);
		CurrentScale = FMath::Vector2DInterpTo(CurrentScale, TargetScale, DeltaTime, InterpSpeed);
	}

	ApplyCurrentTransform();
}

void UFCCardWidget::ApplyCurrentTransform()
{
	SetRenderTranslation(CurrentTranslation);
	SetRenderTransformAngle(CurrentAngle);
	SetRenderScale(CurrentScale);
	SetRenderTransformPivot(TargetPivot);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetZOrder(TargetZOrder);
	}
}

void UFCCardWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (CardViewModel)
	{
		CardViewModel->SetIsHovered(true);
	}

	OnCardHovered.Broadcast(this, true);
	OnCardHoverStateChanged(true);
}

void UFCCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (CardViewModel)
	{
		CardViewModel->SetIsHovered(false);
	}

	OnCardHovered.Broadcast(this, false);
	OnCardHoverStateChanged(false);
}

FReply UFCCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (bIsHeldByHotKey)
		{
			if (APlayerController* PC = GetOwningPlayer())
			{
				if (AFCPlayerController* FCPC = Cast<AFCPlayerController>(PC))
				{
					FCPC->OnLeftMouseButtonPressed();
					return FReply::Handled();
				}
			}

			if (UFCHandWidget* HandWidget = GetTypedOuter<UFCHandWidget>())
			{
				HandWidget->PlayHeldCard();
				return FReply::Handled();
			}
		}

		bIsDragging = true;
		bHasMovedDuringDrag = false;
		DragStartScreenPos = InMouseEvent.GetScreenSpacePosition();
		DragStartCardTranslation = CurrentTranslation;

		// Straighten angle and bring to highest Z-Order while dragging
		TargetAngle = 0.0f;
		CurrentAngle = 0.0f;
		TargetZOrder = 1000;
		ApplyCurrentTransform();

		OnCardDragStarted.Broadcast(this);

		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	else if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (bIsHeldByHotKey)
		{
			if (APlayerController* PC = GetOwningPlayer())
			{
				if (AFCPlayerController* FCPC = Cast<AFCPlayerController>(PC))
				{
					FCPC->OnRightMouseButtonPressed();
					return FReply::Handled();
				}
			}

			if (UFCHandWidget* HandWidget = GetTypedOuter<UFCHandWidget>())
			{
				HandWidget->ClearHeldCard();
				return FReply::Handled();
			}
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UFCCardWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging && HasMouseCapture())
	{
		const FVector2D CurrentScreenPos = InMouseEvent.GetScreenSpacePosition();
		const float DragDistance = FVector2D::Distance(CurrentScreenPos, DragStartScreenPos);
		if (DragDistance > 3.0f)
		{
			bHasMovedDuringDrag = true;
		}

		const FVector2D StartLocalPos = InGeometry.AbsoluteToLocal(DragStartScreenPos);
		const FVector2D CurrentLocalPos = InGeometry.AbsoluteToLocal(CurrentScreenPos);
		const FVector2D LocalDelta = CurrentLocalPos - StartLocalPos;

		CurrentTranslation = DragStartCardTranslation + LocalDelta;
		TargetTranslation = CurrentTranslation;
		CurrentAngle = 0.0f;
		TargetAngle = 0.0f;
		TargetZOrder = 1000;

		ApplyCurrentTransform();

		OnCardDragged.Broadcast(this, CurrentTranslation);
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UFCCardWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDragging)
	{
		bIsDragging = false;
		const bool bWasDragged = bHasMovedDuringDrag;

		OnCardDragEnded.Broadcast(this, bWasDragged);

		if (!bWasDragged)
		{
			OnCardClicked.Broadcast(this);
		}

		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UFCCardWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);

	if (bIsDragging)
	{
		bIsDragging = false;
		OnCardDragEnded.Broadcast(this, bHasMovedDuringDrag);
	}
}
