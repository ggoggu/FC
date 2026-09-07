#include "Character/Player/FCPlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Data/FCPlayerPersistenceSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/Mob/FCMobCharacter.h"

AFCPlayerCharacter::AFCPlayerCharacter()
{
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Create a camera boom
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 400.0f; // Shoulder view distance
	SpringArmComponent->bUsePawnControlRotation = true;
	SpringArmComponent->SocketOffset = FVector(0.f, 50.f, 50.f); // Over-the-shoulder offset

	// Create a follow camera
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false; 

	// GAS is typically mixed mode for player
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	}

	// Load all directional death animations from Character/Mannequins/Anims/Death
	static ConstructorHelpers::FObjectFinder<UAnimSequence> FrontDeathFinder1(
		TEXT("/Game/Character/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01"));
	if (FrontDeathFinder1.Succeeded())
	{
		DeathAnim_Front = FrontDeathFinder1.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> FrontDeathFinder2(
		TEXT("/Game/Character/Mannequins/Anims/Death/MM_Death_Front_02.MM_Death_Front_02"));
	if (FrontDeathFinder2.Succeeded())
	{
		DeathAnim_Front_02 = FrontDeathFinder2.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> FrontDeathFinder3(
		TEXT("/Game/Character/Mannequins/Anims/Death/MM_Death_Front_03.MM_Death_Front_03"));
	if (FrontDeathFinder3.Succeeded())
	{
		DeathAnim_Front_03 = FrontDeathFinder3.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> BackDeathFinder(
		TEXT("/Game/Character/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01"));
	if (BackDeathFinder.Succeeded())
	{
		DeathAnim_Back = BackDeathFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> LeftDeathFinder(
		TEXT("/Game/Character/Mannequins/Anims/Death/MM_Death_Left_01.MM_Death_Left_01"));
	if (LeftDeathFinder.Succeeded())
	{
		DeathAnim_Left = LeftDeathFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> RightDeathFinder(
		TEXT("/Game/Character/Mannequins/Anims/Death/MM_Death_Right_01.MM_Death_Right_01"));
	if (RightDeathFinder.Succeeded())
	{
		DeathAnim_Right = RightDeathFinder.Object;
	}

	// Hit reaction animations default to the animations in Character/Mannequins/Anims/Death
	HitAnim_Front = DeathAnim_Front;
	HitAnim_Back = DeathAnim_Back;
	HitAnim_Left = DeathAnim_Left;
	HitAnim_Right = DeathAnim_Right;

	HitPlayRate = 1.5f;
	HitReactionCooldown = 0.25f;

	bEnableHealthDrain = true;
	InitialDrainInterval = 10.0f;
	HealthDrainAmount = 1.0f;
	MinDrainInterval = 1.0f;
	DrainAccelerationStep = 0.15f;
	DrainDecayMultiplier = 0.98f;
	CurrentDrainInterval = 10.0f;
	HealOnKillAmount = 5.0f;
}

void AFCPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && Controller != nullptr)
	{
		StartHealthDrain();
	}
}

void AFCPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFCPlayerCharacter::Move);
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFCPlayerCharacter::Look);
		}
	}
}

void AFCPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Find forward direction based on Controller Yaw
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement in forward/backward and right/left directions
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AFCPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AFCPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	// Init info on the server
	InitAbilityActorInfo();

	if (HasAuthority())
	{
		if (APlayerController* PC = Cast<APlayerController>(NewController))
		{
			if (UFCPlayerPersistenceSubsystem* PersistenceSubsystem = UFCPlayerPersistenceSubsystem::Get(this))
			{
				const FString Key = PersistenceSubsystem->GetPlayerKey(PC);
				FFCCardDeckSaveData DeckData;
				FFCPlayerStatSaveData StatData;
				if (PersistenceSubsystem->LoadPlayerData(Key, DeckData, StatData))
				{
					if (AttributeSet)
					{
						AttributeSet->RestoreFromStatSaveData(StatData);
					}
				}
			}
		}

		StartHealthDrain();
	}
}

void AFCPlayerCharacter::UnPossessed()
{
	StopHealthDrain();
	Super::UnPossessed();
}

void AFCPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopHealthDrain();
	Super::EndPlay(EndPlayReason);
}

void AFCPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	// Init info on the client
	InitAbilityActorInfo();
}

void AFCPlayerCharacter::InitAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AFCPlayerCharacter::Die(AActor* Killer)
{
	Super::Die(Killer);

	StopHealthDrain();

	const EFCDeathDirection DeathDir = CalculateHitDirection(Killer);
	Multicast_PlayDeathAnimation(DeathDir);
}

void AFCPlayerCharacter::PlayDeathAnimation(EFCDeathDirection Direction)
{
	UAnimSequence* AnimToPlay = GetDeathAnimationForDirection(Direction);
	if (!AnimToPlay)
	{
		return;
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->Stop();
		MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		MeshComp->PlayAnimation(AnimToPlay, /*bLooping=*/false);
	}
}

void AFCPlayerCharacter::Multicast_PlayDeathAnimation_Implementation(EFCDeathDirection Direction)
{
	PlayDeathAnimation(Direction);
}

UAnimSequence* AFCPlayerCharacter::GetDeathAnimationForDirection(EFCDeathDirection Direction) const
{
	switch (Direction)
	{
	case EFCDeathDirection::Front:
	{
		TArray<UAnimSequence*, TInlineAllocator<3>> FrontAnims;
		if (DeathAnim_Front)
		{
			FrontAnims.Add(DeathAnim_Front);
		}
		if (DeathAnim_Front_02)
		{
			FrontAnims.Add(DeathAnim_Front_02);
		}
		if (DeathAnim_Front_03)
		{
			FrontAnims.Add(DeathAnim_Front_03);
		}

		if (FrontAnims.Num() > 0)
		{
			const int32 RandIndex = FMath::RandRange(0, FrontAnims.Num() - 1);
			return FrontAnims[RandIndex];
		}
		return DeathAnim_Front;
	}
	case EFCDeathDirection::Back:
		return DeathAnim_Back;
	case EFCDeathDirection::Left:
		return DeathAnim_Left;
	case EFCDeathDirection::Right:
		return DeathAnim_Right;
	default:
		return DeathAnim_Front;
	}
}

void AFCPlayerCharacter::HandleDamageTaken(float DamageAmount, AActor* DamageCauser, const FHitResult& HitResult)
{
	Super::HandleDamageTaken(DamageAmount, DamageCauser, HitResult);

	if (bIsDead)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (HitReactionCooldown > 0.0f && (CurrentTime - LastHitReactTime < HitReactionCooldown))
	{
		return;
	}
	LastHitReactTime = CurrentTime;

	const EFCDeathDirection HitDir = CalculateHitDirection(DamageCauser);
	Multicast_PlayHitAnimation(HitDir);
}

void AFCPlayerCharacter::PlayHitAnimation(EFCDeathDirection Direction)
{
	if (bIsDead)
	{
		return;
	}

	if (HitMontage)
	{
		PlayAnimMontage(HitMontage, HitPlayRate);
		return;
	}

	UAnimSequence* AnimToPlay = GetHitAnimationForDirection(Direction);
	if (!AnimToPlay)
	{
		return;
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
		{
			AnimInstance->PlaySlotAnimationAsDynamicMontage(
				AnimToPlay,
				FName(TEXT("DefaultSlot")),
				/*BlendInTime=*/0.05f,
				/*BlendOutTime=*/0.15f,
				HitPlayRate
			);
		}
	}
}

void AFCPlayerCharacter::Multicast_PlayHitAnimation_Implementation(EFCDeathDirection Direction)
{
	PlayHitAnimation(Direction);
}

UAnimSequence* AFCPlayerCharacter::GetHitAnimationForDirection(EFCDeathDirection Direction) const
{
	switch (Direction)
	{
	case EFCDeathDirection::Front:
		return HitAnim_Front ? HitAnim_Front.Get() : DeathAnim_Front.Get();
	case EFCDeathDirection::Back:
		return HitAnim_Back ? HitAnim_Back.Get() : DeathAnim_Back.Get();
	case EFCDeathDirection::Left:
		return HitAnim_Left ? HitAnim_Left.Get() : DeathAnim_Left.Get();
	case EFCDeathDirection::Right:
		return HitAnim_Right ? HitAnim_Right.Get() : DeathAnim_Right.Get();
	default:
		return HitAnim_Front ? HitAnim_Front.Get() : DeathAnim_Front.Get();
	}
}

void AFCPlayerCharacter::StartHealthDrain()
{
	if (!HasAuthority() || bIsDead || !bEnableHealthDrain)
	{
		return;
	}

	if (CurrentDrainInterval <= 0.0f)
	{
		CurrentDrainInterval = InitialDrainInterval;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HealthDrainTimerHandle);
		World->GetTimerManager().SetTimer(
			HealthDrainTimerHandle,
			this,
			&AFCPlayerCharacter::HandleHealthDrainTick,
			CurrentDrainInterval,
			false
		);
	}
}

void AFCPlayerCharacter::StopHealthDrain()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HealthDrainTimerHandle);
	}
}

void AFCPlayerCharacter::ResetHealthDrain()
{
	CurrentDrainInterval = InitialDrainInterval;
	if (HasAuthority() && !bIsDead && bEnableHealthDrain)
	{
		StartHealthDrain();
	}
}

void AFCPlayerCharacter::HandleHealthDrainTick()
{
	if (!HasAuthority() || bIsDead)
	{
		StopHealthDrain();
		return;
	}

	if (AttributeSet)
	{
		const float CurrentHealth = AttributeSet->GetHealth();
		const float NewHealth = FMath::Clamp(CurrentHealth - HealthDrainAmount, 0.0f, AttributeSet->GetMaxHealth());
		AttributeSet->SetHealth(NewHealth);

		if (NewHealth <= 0.0f)
		{
			StopHealthDrain();
			Die(nullptr);
			return;
		}
	}

	// Calculate accelerated interval for next tick
	// Slowly accelerate: decrement step, and gently scale by decay multiplier
	float NextInterval = CurrentDrainInterval - DrainAccelerationStep;
	if (DrainDecayMultiplier > 0.0f && DrainDecayMultiplier < 1.0f)
	{
		NextInterval *= DrainDecayMultiplier;
	}
	CurrentDrainInterval = FMath::Max(MinDrainInterval, NextInterval);

	// Schedule next drain tick
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			HealthDrainTimerHandle,
			this,
			&AFCPlayerCharacter::HandleHealthDrainTick,
			CurrentDrainInterval,
			false
		);
	}
}

void AFCPlayerCharacter::OnKilledEnemy(AFCMobCharacter* VictimMob)
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	const float RewardAmount = (VictimMob && VictimMob->GetHealthRewardOnKill() > 0.0f)
		? VictimMob->GetHealthRewardOnKill()
		: HealOnKillAmount;

	if (RewardAmount > 0.0f)
	{
		ApplyHeal(RewardAmount);
	}
}
