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
}

void AFCPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
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
	}
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
