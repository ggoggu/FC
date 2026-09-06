#include "Controller/Player/FCPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Data/Card/FCCardTypes.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Card/FCCardDeckComponent.h"
#include "Game/FCPlayerState.h"
#include "UI/View/FCHUDWidget.h"
#include "UI/View/FCHandWidget.h"
#include "UI/ViewModel/FCHUDViewModel.h"
#include "UI/ViewModel/FCHandViewModel.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Combat/FCCombatUtils.h"
#include "Data/Card/FCCardDataAsset.h"
#include "AbilitySystem/Abilities/FCGA_SpawnProjectile.h"
#include "Character/FCCharacterBase.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

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

	if (InputComponent)
	{
		// Keyboard number keys 1~0
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

		// Keypad number keys 1~0
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

		// Left Mouse Button (play held card)
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AFCPlayerController::OnLeftMouseButtonPressed);

		// Right Mouse Button & Escape (cancel held card)
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
			HUDWidget->AddToViewport();
			HUDWidget->SetHUDViewModel(HUDViewModel);
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
	DeckComp->OnCardCycleTriggered.RemoveAll(this);
	DeckComp->OnCycleSettingsChanged.RemoveAll(this);

	DeckComp->OnCardHandUpdated.AddDynamic(this, &AFCPlayerController::SyncHandToViewModel);
	DeckComp->OnPileCountsChanged.AddDynamic(this, &AFCPlayerController::OnDeckPileCountsChanged);
	DeckComp->OnCardCycleTriggered.AddDynamic(this, &AFCPlayerController::OnDeckCycleTriggered);
	DeckComp->OnCycleSettingsChanged.AddDynamic(this, &AFCPlayerController::OnDeckCycleSettingsChanged);

	// Perform initial sync
	SyncHandToViewModel();
	HUDViewModel->SetDrawPileCount(DeckComp->GetDrawPileCount());
	HUDViewModel->SetDiscardPileCount(DeckComp->GetDiscardPileCount());
	HUDViewModel->SetExhaustPileCount(DeckComp->GetExhaustPileCount());
	HUDViewModel->SetCycleInterval(DeckComp->GetCycleInterval());
	HUDViewModel->SetCycleRemainingTime(DeckComp->GetCycleRemainingTime());
	HUDViewModel->SetCycleProgress(DeckComp->GetCycleProgress());
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
		HUDViewModel->SetCurrentShield(FMath::RoundToInt(AttrSet->GetShield()));
		HUDViewModel->SetMaxShield(FMath::RoundToInt(AttrSet->GetMaxShield()));
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

	ASC->GetGameplayAttributeValueChangeDelegate(UFCAttributeSet::GetShieldAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			if (HUDViewModel)
			{
				HUDViewModel->SetCurrentShield(FMath::RoundToInt(Data.NewValue));
			}
		});

	ASC->GetGameplayAttributeValueChangeDelegate(UFCAttributeSet::GetMaxShieldAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			if (HUDViewModel)
			{
				HUDViewModel->SetMaxShield(FMath::RoundToInt(Data.NewValue));
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

void AFCPlayerController::OnDeckCycleTriggered()
{
	if (HUDViewModel)
	{
		if (AFCPlayerState* FCPS = GetPlayerState<AFCPlayerState>())
		{
			if (UFCCardDeckComponent* DeckComp = FCPS->GetCardDeckComponent())
			{
				HUDViewModel->SetCycleInterval(DeckComp->GetCycleInterval());
				HUDViewModel->SetCycleRemainingTime(DeckComp->GetCycleRemainingTime());
				HUDViewModel->SetCycleProgress(DeckComp->GetCycleProgress());
			}
		}
	}
}

void AFCPlayerController::OnDeckCycleSettingsChanged(float NewInterval, int32 NewDrawCount)
{
	if (HUDViewModel)
	{
		HUDViewModel->SetCycleInterval(NewInterval);
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
	// Client-side prediction: immediately rotate local player pawn towards target
	if (IsLocalPlayerController())
	{
		if (AFCCharacterBase* Char = Cast<AFCCharacterBase>(GetPawn()))
		{
			const FVector AimPos = TargetInfo.TargetActor.IsValid() 
				? TargetInfo.TargetActor->GetActorLocation() 
				: (FVector)TargetInfo.TargetLocation;

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
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (GetMousePosition(MouseX, MouseY))
	{
		return ResolveCardTargetAtScreenPosition(FVector2D(MouseX, MouseY), CardAsset);
	}

	// Fallback if no mouse cursor position (e.g. headless test or gamepad mode)
	if (APawn* MyPawn = GetPawn())
	{
		FFCCardTargetInfo FallbackInfo;
		FallbackInfo.TargetLocation = MyPawn->GetActorLocation() + MyPawn->GetActorForwardVector() * 1000.0f;
		return FallbackInfo;
	}

	return FFCCardTargetInfo();
}

FFCCardTargetInfo AFCPlayerController::ResolveCardTargetAtScreenPosition(const FVector2D& ScreenPos, const UFCCardDataAsset* CardAsset)
{
	FFCCardTargetInfo TargetInfo;

	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return TargetInfo;
	}

	const FVector PawnLocation = MyPawn->GetActorLocation();

	// 1. Deproject Screen Position to 3D World Ray
	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = MyPawn->GetActorForwardVector();

	if (!DeprojectScreenPositionToWorld(ScreenPos.X, ScreenPos.Y, WorldOrigin, WorldDirection))
	{
		if (PlayerCameraManager)
		{
			WorldOrigin = PlayerCameraManager->GetCameraLocation();
			WorldDirection = PlayerCameraManager->GetCameraRotation().Vector();
		}
	}

	// 2. Line trace along WorldDirection to find 3D Impact / Drop Location
	const float TraceDistance = 15000.0f;
	const FVector TraceEnd = WorldOrigin + WorldDirection * TraceDistance;

	FHitResult HitResult;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(CardTargetDeprojectTrace), true);
	TraceParams.AddIgnoredActor(MyPawn);

	UWorld* World = GetWorld();
	bool bHit = false;
	if (World)
	{
		bHit = World->LineTraceSingleByChannel(
			HitResult,
			WorldOrigin,
			TraceEnd,
			ECC_Visibility,
			TraceParams
		);
	}

	FVector DropWorldLocation = FVector::ZeroVector;
	AActor* DirectHitActor = nullptr;

	if (bHit && HitResult.bBlockingHit)
	{
		DropWorldLocation = HitResult.ImpactPoint;
		DirectHitActor = HitResult.GetActor();
		TargetInfo.TargetHitComponent = HitResult.GetComponent();
	}
	else
	{
		// Intersect ray with ground plane at player's foot level
		const FPlane GroundPlane(PawnLocation, FVector::UpVector);
		DropWorldLocation = FMath::LinePlaneIntersection(WorldOrigin, TraceEnd, GroundPlane);
	}

	TargetInfo.TargetLocation = DropWorldLocation;

	// Check if this card shoots/spawns projectiles and whether auto-targeting is enabled
	const bool bIsProjectileCard = CardAsset && (CardAsset->GameplayData.SpawnsProjectile() || 
		(CardAsset->GameplayData.CardAbilityClass && CardAsset->GameplayData.CardAbilityClass->IsChildOf(UFCGA_SpawnProjectile::StaticClass())));

	// If single target or direct hit on attackable target
	if (DirectHitActor && UFCCombatUtils::IsAttackableTarget(MyPawn, DirectHitActor))
	{
		TargetInfo.TargetActor = DirectHitActor;
		TargetInfo.TargetLocation = DirectHitActor->GetActorLocation();
		return TargetInfo;
	}

	// If projectile auto-targeting is active, search for best attackable target in that direction
	if (bAutoTargetAttackablesOnProjectileCards && bIsProjectileCard)
	{
		const FVector AimDirection2D = (DropWorldLocation - PawnLocation).GetSafeNormal2D();

		AActor* BestTarget = UFCCombatUtils::FindBestAttackableTargetInDirection(
			MyPawn,
			AimDirection2D,
			DropWorldLocation,
			ProjectileTargetMaxRange,
			ProjectileTargetHalfAngleDegrees,
			ProjectileTargetProximityRadius,
			true // Check Line of Sight
		);

		if (BestTarget)
		{
			TargetInfo.TargetActor = BestTarget;
			TargetInfo.TargetLocation = BestTarget->GetActorLocation();
		}
	}

	return TargetInfo;
}

void AFCPlayerController::HandleNumberKeyInput(int32 SlotIndex)
{
	if (!IsLocalPlayerController() || !HUDWidget)
	{
		return;
	}

	if (UFCHandWidget* HandWidget = HUDWidget->GetHandWidget())
	{
		HandWidget->SelectAndHoldCardByIndex(SlotIndex);
	}
}

void AFCPlayerController::OnLeftMouseButtonPressed()
{
	if (!IsLocalPlayerController() || !HUDWidget)
	{
		return;
	}

	if (UFCHandWidget* HandWidget = HUDWidget->GetHandWidget())
	{
		if (HandWidget->HasHeldCard())
		{
			HandWidget->PlayHeldCard();
		}
	}
}

void AFCPlayerController::OnRightMouseButtonPressed()
{
	if (!IsLocalPlayerController() || !HUDWidget)
	{
		return;
	}

	if (UFCHandWidget* HandWidget = HUDWidget->GetHandWidget())
	{
		if (HandWidget->HasHeldCard())
		{
			HandWidget->ClearHeldCard();
		}
	}
}
